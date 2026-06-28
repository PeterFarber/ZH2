# SVG Decal Assets Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let rink markings, center-ice logos, and board/ice ads be authored as SVG files and used by the existing material-backed `DecalComponent` renderer path.

**Architecture:** Treat SVG files as source texture art in the Phase 5 asset pipeline. The importer classifies `.svg` as `AssetType::Texture`, the cooker rasterizes SVG to RGBA8 cooked texture data, material YAML references the SVG through the existing `Textures.BaseColor` slot, and the renderer samples the cooked texture through the existing `DecalComponent::materialAsset` path. No Vulkan shader, ECS component, or gameplay change is required.

**Tech Stack:** C++20, CMake, vcpkg manifest mode, LunaSVG through vcpkg, existing `hockey_assets` import/cook/runtime loaders, existing `hockey_renderer` decal projection, `HockeyAssetTests`, `HockeyRendererTests`, root `justfile`.

---

## Repository Note

This repository ignores new files under `plans/` by default. When committing updates to this plan file, use `git add -f plans/svg-decal-assets.md` for this file only.

## Current Baseline

- `DecalComponent` already stores a `materialAsset` and renders material-backed base-color/normal decals in the forward PBR shader.
- `MaterialSource` already references raw texture paths in `Textures.BaseColor`, `Textures.Normal`, `Textures.MetallicRoughness`, `Textures.Occlusion`, and `Textures.Emissive`.
- `TextureImporter` and `TextureCooker` currently rely on `TextureLoader`/stb for raster image formats only.
- `AssetManager::ClassifyExtension` currently recognizes `.png`, `.jpg`, `.jpeg`, `.tga`, `.bmp`, `.hdr`, `.ktx`, and `.ktx2` as textures, but not `.svg`.
- `AssetType` does not need a new vector type for this feature; a cooked SVG becomes a normal cooked `TextureAsset`.
- The renderer should not parse SVG at runtime. Runtime loads only cooked `.tex.bin` texture data.

## Design Decisions

- Use LunaSVG as the SVG parser/rasterizer because it is a standalone C++ SVG rendering library available as a vcpkg package, and RmlUi's SVG feature already uses the same package in vcpkg.
- Keep SVG support private to `hockey_assets`; renderer, ECS, gameplay, physics, networking, and server code should not include LunaSVG headers.
- Rasterize SVGs during cooking with a transparent background and RGBA output.
- Preserve the SVG document aspect ratio. Use intrinsic SVG `width` and `height` when available; otherwise use a 1024x1024 fallback.
- Set SVG texture defaults to sRGB `BaseColor`, mipmap generation enabled, and a 2048-pixel largest dimension clamp. This gives rink logos and ads better fidelity than the current 1024 raster default without changing existing raster texture behavior.
- Material authors use `AlphaMode: Blend` for translucent logos/ads and `AlphaMode: Mask` for crisp hard-edged markings.

## Affected Files

Create:

- `libs/assets/include/Hockey/Assets/Runtime/SvgRasterizer.hpp`
- `libs/assets/src/Runtime/SvgRasterizer.cpp`
- `apps/asset_tests/src/SvgTextureImportTests.cpp`

Modify:

- `vcpkg.json`
- `libs/assets/CMakeLists.txt`
- `libs/assets/src/AssetManager.cpp`
- `libs/assets/include/Hockey/Assets/Importers/TextureImporter.hpp`
- `libs/assets/src/Importers/TextureImporter.cpp`
- `libs/assets/src/Cookers/TextureCooker.cpp`
- `apps/asset_tests/CMakeLists.txt`
- `apps/asset_tests/src/AssetTestsMain.cpp`
- `docs/phase-status/phase-05-asset-pipeline.md`

Do not modify:

- `libs/ecs/include/Hockey/ECS/RenderComponents.hpp`
- `libs/renderer/src/Renderer.cpp`
- `data/shaders/src/decals.glsl`
- `data/shaders/src/pbr.frag`
- `libs/gameplay/**`
- `libs/physics/**`
- `libs/networking/**`

## Authoring Workflow After Implementation

Raw SVG:

```text
data/raw/textures/rink_decals/center_ice_logo.svg
```

Raw material:

```yaml
Material:
  Name: Center Ice Logo Decal
  Type: PBR
  BaseColor: [1, 1, 1, 1]
  Metallic: 0
  Roughness: 0.65
  NormalStrength: 1
  OcclusionStrength: 1
  EmissiveColor: [0, 0, 0]
  EmissiveStrength: 0
  AlphaMode: Blend
  AlphaCutoff: 0.5
  Tiling: [1, 1]
  Offset: [0, 0]
  Textures:
    BaseColor: data/raw/textures/rink_decals/center_ice_logo.svg
```

Scene entity:

```yaml
DecalComponent:
  MaterialAsset: <asset id of the material above>
  Size: [6.0, 0.25, 6.0]
  AffectsBaseColor: true
  AffectsNormals: false
```

---

### Task 1: Add Failing SVG Texture And Decal Material Tests

**Files:**
- Create: `apps/asset_tests/src/SvgTextureImportTests.cpp`
- Modify: `apps/asset_tests/CMakeLists.txt`
- Modify: `apps/asset_tests/src/AssetTestsMain.cpp`

- [x] **Step 1: Create the SVG texture test source**

Create `apps/asset_tests/src/SvgTextureImportTests.cpp`:

```cpp
#include "Test.hpp"

#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Assets/Assets/MaterialAsset.hpp"
#include "Hockey/Assets/Assets/TextureAsset.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Paths.hpp"

#include <cstddef>
#include <cstdint>

using namespace Hockey;
namespace fs = std::filesystem;

namespace {

const char* kCenterLogoSvg = R"(<svg xmlns="http://www.w3.org/2000/svg" width="64" height="32" viewBox="0 0 64 32">
  <rect x="0" y="0" width="64" height="32" fill="#ff0000" fill-opacity="0.75"/>
  <circle cx="32" cy="16" r="8" fill="#ffffff" fill-opacity="1"/>
</svg>
)";

const char* kSvgDecalMaterial = R"(Material:
  Name: Center Ice Logo Decal
  Type: PBR
  BaseColor: [1, 1, 1, 1]
  Metallic: 0
  Roughness: 0.65
  NormalStrength: 1
  OcclusionStrength: 1
  EmissiveColor: [0, 0, 0]
  EmissiveStrength: 0
  AlphaMode: Blend
  AlphaCutoff: 0.5
  Tiling: [1, 1]
  Offset: [0, 0]
  Textures:
    BaseColor: data/raw/textures/rink_decals/center_logo.svg
)";

uint8_t ByteAt(const TextureAsset& texture, size_t offset) {
    return static_cast<uint8_t>(texture.data[offset]);
}

} // namespace

void RunSvgTextureImportTests() {
    HockeyTest::BeginSuite("SvgTextureImportTests");

    const fs::path workspace = Paths::TempFile("svg_texture_ws");
    FileSystem::Remove(workspace);

    const fs::path svgPath = workspace / "data" / "raw" / "textures" / "rink_decals" / "center_logo.svg";
    const fs::path materialPath = workspace / "data" / "raw" / "materials" / "rink_decals" /
                                  "center_logo_decal.material.yaml";
    HK_CHECK_MSG(static_cast<bool>(FileSystem::WriteTextFile(svgPath, kCenterLogoSvg)), "svg source written");
    HK_CHECK_MSG(static_cast<bool>(FileSystem::WriteTextFile(materialPath, kSvgDecalMaterial)), "material source written");

    AssetManager manager;
    HK_CHECK_MSG(static_cast<bool>(manager.Init(AssetManager::DefaultCreateInfo(workspace))), "manager init");
    HK_CHECK_MSG(static_cast<bool>(manager.DiscoverRawAssets()), "discover svg ok");
    HK_CHECK_MSG(static_cast<bool>(manager.ImportAll()), "import svg ok");

    AssetMetadata* svgMeta = manager.Database().FindByRawPath("data/raw/textures/rink_decals/center_logo.svg");
    HK_CHECK_MSG(svgMeta != nullptr, "svg metadata created");
    HK_CHECK_MSG(svgMeta != nullptr && svgMeta->type == AssetType::Texture, "svg is imported as texture");

    AssetMetadata* materialMeta =
        manager.Database().FindByRawPath("data/raw/materials/rink_decals/center_logo_decal.material.yaml");
    HK_CHECK_MSG(materialMeta != nullptr, "svg decal material metadata created");
    HK_CHECK_MSG(materialMeta != nullptr && materialMeta->type == AssetType::Material, "svg decal material is material");

    bool materialDependsOnSvg = false;
    if (materialMeta != nullptr && svgMeta != nullptr) {
        for (const AssetID dep : materialMeta->dependencies) {
            materialDependsOnSvg = materialDependsOnSvg || dep == svgMeta->id;
        }
    }
    HK_CHECK_MSG(materialDependsOnSvg, "material depends on svg texture");

    HK_CHECK_MSG(static_cast<bool>(manager.CookAllDirty()), "cook svg texture and material ok");
    svgMeta = manager.Database().FindByRawPath("data/raw/textures/rink_decals/center_logo.svg");
    HK_CHECK_MSG(svgMeta != nullptr && svgMeta->cooked, "svg texture cooked");
    HK_CHECK_MSG(svgMeta != nullptr && FileSystem::Exists(workspace / svgMeta->cookedPath), "svg cooked file exists");

    Result<std::shared_ptr<TextureAsset>> texture = manager.Load<TextureAsset>(svgMeta != nullptr ? svgMeta->id : AssetID{});
    HK_CHECK_MSG(static_cast<bool>(texture), "load cooked svg texture");
    if (texture) {
        HK_CHECK_EQ(texture.value->width, 64u);
        HK_CHECK_EQ(texture.value->height, 32u);
        HK_CHECK_MSG(texture.value->mipLevels >= 6u, "svg texture has mip chain");
        HK_CHECK_MSG(texture.value->colorSpace == TextureColorSpace::SRGB, "svg texture is sRGB");
        HK_CHECK_MSG(texture.value->semantic == TextureSemantic::BaseColor, "svg texture is base color semantic");
        HK_CHECK_MSG(texture.value->data.size() >= 64u * 32u * 4u, "svg texture has base mip data");
        HK_CHECK_MSG(ByteAt(*texture.value, 0) > 200u, "svg first pixel red channel rasterized");
        HK_CHECK_MSG(ByteAt(*texture.value, 3) > 180u, "svg first pixel alpha rasterized");
    }

    materialMeta = manager.Database().FindByRawPath("data/raw/materials/rink_decals/center_logo_decal.material.yaml");
    Result<std::shared_ptr<MaterialAsset>> material =
        manager.Load<MaterialAsset>(materialMeta != nullptr ? materialMeta->id : AssetID{});
    HK_CHECK_MSG(static_cast<bool>(material), "load cooked svg decal material");
    if (material && svgMeta != nullptr) {
        HK_CHECK_MSG(material.value->name == "Center Ice Logo Decal", "svg decal material name loaded");
        HK_CHECK_MSG(material.value->alphaMode == "Blend", "svg decal material alpha mode preserved");
        HK_CHECK_MSG(material.value->baseColorTexture == svgMeta->id, "svg texture id assigned to material base color");
    }

    manager.Shutdown();
    FileSystem::Remove(workspace);
}
```

- [x] **Step 2: Register the test in the asset test CMake target**

Modify `apps/asset_tests/CMakeLists.txt` so the executable source list contains:

```cmake
    src/SvgTextureImportTests.cpp
```

Place it near `src/TextureImportTests.cpp`.

- [x] **Step 3: Register the test in the asset test main**

Modify `apps/asset_tests/src/AssetTestsMain.cpp`:

```cpp
void RunSvgTextureImportTests();
```

Call it immediately after `RunTextureImportTests();`:

```cpp
    RunTextureImportTests();
    RunSvgTextureImportTests();
```

- [x] **Step 4: Run the failing test**

Run:

```powershell
.\scripts\windows\build_debug.ps1
.\build\windows-debug\apps\asset_tests\HockeyAssetTests.exe --root .
```

Expected result before implementation:

```text
[RUN ] SvgTextureImportTests
[FAIL] ... svg metadata created
[FAIL] ... svg is imported as texture
```

If the first command fails before compiling tests because dependencies are not installed, run the same focused target through the configured build preset after Task 2.

Actual red result:

```text
`.\scripts\windows\build_debug.ps1`: passed.
`.\build\windows-debug\apps\asset_tests\HockeyAssetTests.exe --root .`: failed as expected in `SvgTextureImportTests`; SVG metadata/dependency discovery reached cook, then `cook svg texture and material ok`, `svg texture cooked`, and `load cooked svg texture` failed because SVG rasterization is not implemented yet.
```

- [x] **Step 5: Commit the failing test**

```powershell
git add apps/asset_tests/src/SvgTextureImportTests.cpp apps/asset_tests/CMakeLists.txt apps/asset_tests/src/AssetTestsMain.cpp
git commit -m "test: cover svg decal texture assets"
```

Committed as `d3fffee`.

---

### Task 2: Add The SVG Rasterizer Dependency

**Files:**
- Modify: `vcpkg.json`
- Modify: `libs/assets/CMakeLists.txt`

- [x] **Step 1: Add LunaSVG to the vcpkg manifest**

Modify `vcpkg.json` and add the dependency string in the existing dependencies array near `stb`:

```json
        "stb",
        "lunasvg",
        "fastgltf",
```

- [x] **Step 2: Find and link LunaSVG privately in hockey_assets**

Modify `libs/assets/CMakeLists.txt`:

```cmake
find_package(lunasvg CONFIG REQUIRED)
```

Place it next to the other package discovery calls.

Add the source files that will be created in Task 3:

```cmake
        src/Runtime/SvgRasterizer.cpp
```

Place it near `src/Runtime/TextureLoader.cpp`.

Implementation note: source registration is deferred to Task 3 so configure does not reference a non-existent `SvgRasterizer.cpp`.

Add the private library link:

```cmake
        lunasvg::lunasvg
```

Place it in the existing `target_link_libraries(hockey_assets PRIVATE ...)` block.

- [x] **Step 3: Configure/build to verify dependency resolution**

Run:

```powershell
.\scripts\windows\configure_debug.ps1
cmake --build --preset build-windows-debug --target hockey_assets
```

Expected result:

```text
Build succeeded.
```

If CMake reports a different imported target name for the vcpkg port, inspect the generated package config under the vcpkg installed tree and replace `lunasvg::lunasvg` with the exported target name shown there.

Actual result:

```text
`.\scripts\windows\configure_debug.ps1`: initially failed while trying to remove stale generated `build/windows-debug` cache because `vcpkg-running.lock` was locked.
Approved cleanup removed generated `build/windows-debug/vcpkg_installed`.
`. .\scripts\windows\Env.ps1; cmake --preset windows-debug`: passed; vcpkg installed `lunasvg:x64-windows@3.5.0` and reported target `lunasvg::lunasvg`.
`. .\scripts\windows\Env.ps1; cmake --build --preset build-windows-debug --target hockey_assets`: passed.
```

- [x] **Step 4: Commit dependency wiring**

```powershell
git add vcpkg.json libs/assets/CMakeLists.txt
git commit -m "build: add svg rasterizer dependency"
```

Committed as `1562a27`.

---

### Task 3: Add A Focused SVG Rasterizer Helper

**Files:**
- Create: `libs/assets/include/Hockey/Assets/Runtime/SvgRasterizer.hpp`
- Create: `libs/assets/src/Runtime/SvgRasterizer.cpp`

- [x] **Step 1: Add the public helper header**

Create `libs/assets/include/Hockey/Assets/Runtime/SvgRasterizer.hpp`:

```cpp
#pragma once

#include <cstdint>
#include <filesystem>

#include "Hockey/Assets/Assets/TextureAsset.hpp"
#include "Hockey/Core/Result.hpp"

namespace Hockey {

struct SvgInfo {
    uint32_t width = 0;
    uint32_t height = 0;
};

class SvgRasterizer {
public:
    static constexpr int kFallbackSize = 1024;
    static constexpr int kDefaultMaxSize = 2048;

    static Result<SvgInfo> Inspect(const std::filesystem::path& rawPath);
    static Result<TextureAsset> Rasterize(const std::filesystem::path& rawPath,
                                          const TextureImportSettings& settings);
};

} // namespace Hockey
```

- [x] **Step 2: Add the implementation**

Create `libs/assets/src/Runtime/SvgRasterizer.cpp`:

```cpp
#include "Hockey/Assets/Runtime/SvgRasterizer.hpp"

#include "Hockey/Assets/Runtime/TextureLoader.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>

#include <lunasvg.h>

namespace Hockey {

namespace {

bool IsFinitePositive(float value) {
    return std::isfinite(value) && value > 0.0f;
}

SvgInfo ResolveRasterSize(float sourceWidth, float sourceHeight, int maxSize) {
    float width = IsFinitePositive(sourceWidth) ? sourceWidth : static_cast<float>(SvgRasterizer::kFallbackSize);
    float height = IsFinitePositive(sourceHeight) ? sourceHeight : static_cast<float>(SvgRasterizer::kFallbackSize);

    const float largest = std::max(width, height);
    if (maxSize > 0 && largest > static_cast<float>(maxSize)) {
        const float scale = static_cast<float>(maxSize) / largest;
        width *= scale;
        height *= scale;
    }

    SvgInfo info;
    info.width = std::max(1u, static_cast<uint32_t>(std::lround(width)));
    info.height = std::max(1u, static_cast<uint32_t>(std::lround(height)));
    return info;
}

std::unique_ptr<lunasvg::Document> LoadDocument(const std::filesystem::path& rawPath, std::string& error) {
    std::unique_ptr<lunasvg::Document> document = lunasvg::Document::loadFromFile(rawPath.string());
    if (document == nullptr) {
        error = "failed to parse svg: " + rawPath.string();
    }
    return document;
}

} // namespace

Result<SvgInfo> SvgRasterizer::Inspect(const std::filesystem::path& rawPath) {
    std::string error;
    std::unique_ptr<lunasvg::Document> document = LoadDocument(rawPath, error);
    if (document == nullptr) {
        return Result<SvgInfo>::Fail(error);
    }

    const SvgInfo info = ResolveRasterSize(document->width(), document->height(), kDefaultMaxSize);
    return Result<SvgInfo>::Ok(info);
}

Result<TextureAsset> SvgRasterizer::Rasterize(const std::filesystem::path& rawPath,
                                              const TextureImportSettings& settings) {
    std::string error;
    std::unique_ptr<lunasvg::Document> document = LoadDocument(rawPath, error);
    if (document == nullptr) {
        return Result<TextureAsset>::Fail(error);
    }

    const int maxSize = settings.maxSize > 0 ? settings.maxSize : kDefaultMaxSize;
    const SvgInfo size = ResolveRasterSize(document->width(), document->height(), maxSize);

    lunasvg::Bitmap bitmap =
        document->renderToBitmap(static_cast<int>(size.width), static_cast<int>(size.height), 0x00000000u);
    if (bitmap.isNull()) {
        return Result<TextureAsset>::Fail("failed to rasterize svg: " + rawPath.string());
    }
    bitmap.convertToRGBA();

    TextureAsset asset;
    asset.width = static_cast<uint32_t>(bitmap.width());
    asset.height = static_cast<uint32_t>(bitmap.height());
    asset.channels = 4;
    asset.mipLevels = 1;
    asset.colorSpace = settings.colorSpace;
    asset.semantic = settings.semantic;
    asset.data.resize(static_cast<size_t>(asset.width) * asset.height * 4u);

    const uint8_t* src = bitmap.data();
    std::byte* dst = asset.data.data();
    const size_t rowBytes = static_cast<size_t>(asset.width) * 4u;
    for (uint32_t y = 0; y < asset.height; ++y) {
        std::memcpy(dst + static_cast<size_t>(y) * rowBytes, src + static_cast<size_t>(y) * bitmap.stride(), rowBytes);
    }

    if (settings.generateMipmaps && asset.width > 0 && asset.height > 0) {
        TextureLoader::GenerateMipChain(asset);
    }
    return Result<TextureAsset>::Ok(std::move(asset));
}

} // namespace Hockey
```

- [x] **Step 3: Build the helper**

Run:

```powershell
cmake --build --preset build-windows-debug --target hockey_assets
```

Expected result:

```text
Build succeeded.
```

Actual result:

```text
`. .\scripts\windows\Env.ps1; cmake --build --preset build-windows-debug --target hockey_assets`: passed.
```

- [x] **Step 4: Commit the helper**

```powershell
git add libs/assets/include/Hockey/Assets/Runtime/SvgRasterizer.hpp libs/assets/src/Runtime/SvgRasterizer.cpp libs/assets/CMakeLists.txt
git commit -m "feat: add svg rasterizer helper"
```

Committed as `edeed4d`.

---

### Task 4: Integrate SVG With Texture Import And Cook

**Files:**
- Modify: `libs/assets/src/AssetManager.cpp`
- Modify: `libs/assets/include/Hockey/Assets/Importers/TextureImporter.hpp`
- Modify: `libs/assets/src/Importers/TextureImporter.cpp`
- Modify: `libs/assets/src/Cookers/TextureCooker.cpp`

- [x] **Step 1: Classify `.svg` as a texture asset**

Modify `AssetManager::ClassifyExtension` in `libs/assets/src/AssetManager.cpp`:

```cpp
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp" || ext == ".hdr" ||
        ext == ".ktx" || ext == ".ktx2" || ext == ".svg")
        return AssetType::Texture;
```

- [x] **Step 2: Add SVG helpers to TextureImporter**

Modify `libs/assets/include/Hockey/Assets/Importers/TextureImporter.hpp`:

```cpp
    static bool IsSvg(const std::filesystem::path& rawPath);
```

Place it near `InferSettings`.

- [x] **Step 3: Teach TextureImporter to support and inspect SVG**

Modify `libs/assets/src/Importers/TextureImporter.cpp`.

Add the include:

```cpp
#include "Hockey/Assets/Runtime/SvgRasterizer.hpp"
```

Add the helper:

```cpp
bool TextureImporter::IsSvg(const fs::path& rawPath) {
    std::string ext = rawPath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext == ".svg";
}
```

Modify `SupportsExtension`:

```cpp
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp" ||
           ext == ".hdr" || ext == ".ktx" || ext == ".ktx2" || ext == ".svg";
```

Modify `InferSettings` so SVGs default to higher-resolution decal/base-color behavior:

```cpp
    TextureImportSettings settings;
    if (IsSvg(rawPath)) {
        settings.semantic = TextureSemantic::BaseColor;
        settings.colorSpace = TextureColorSpace::SRGB;
        settings.normalMap = false;
        settings.generateMipmaps = true;
        settings.compress = true;
        settings.maxSize = SvgRasterizer::kDefaultMaxSize;
        return settings;
    }
```

Place that block immediately after the `TextureImportSettings settings;` declaration.

Modify `TextureImporter::Import` so SVGs use LunaSVG inspection instead of stb dimension inspection:

```cpp
    uint32_t width = 0, height = 0, channels = 0;
    if (IsSvg(context.rawPath)) {
        Result<SvgInfo> info = SvgRasterizer::Inspect(context.rawPath);
        if (!info) {
            result.success = false;
            result.error = info.error;
            return result;
        }
        width = info.value.width;
        height = info.value.height;
        channels = 4;
    } else if (!TextureLoader::ReadDimensions(context.rawPath, width, height, channels)) {
        result.success = false;
        result.error = "not a readable image: " + context.rawPath.string();
        return result;
    }
```

Keep the existing metadata assignment after this block.

- [x] **Step 4: Teach TextureCooker to rasterize SVGs**

Modify `libs/assets/src/Cookers/TextureCooker.cpp`.

Add the include:

```cpp
#include "Hockey/Assets/Runtime/SvgRasterizer.hpp"
```

Replace the raw load block:

```cpp
    Result<TextureAsset> loaded = TextureLoader::LoadRaw(rawAbsolute, settings);
```

with:

```cpp
    Result<TextureAsset> loaded = TextureImporter::IsSvg(metadata.rawPath)
                                      ? SvgRasterizer::Rasterize(rawAbsolute, settings)
                                      : TextureLoader::LoadRaw(rawAbsolute, settings);
```

Keep the existing error handling, `asset.id`, source hash, encode, and cooked output path logic unchanged.

- [x] **Step 5: Run the focused asset tests**

Run:

```powershell
cmake --build --preset build-windows-debug --target HockeyAssetTests
.\build\windows-debug\apps\asset_tests\HockeyAssetTests.exe --root .
```

Expected result:

```text
[RUN ] SvgTextureImportTests
==== Asset tests: <passed> passed, 0 failed ====
```

Actual result:

```text
`. .\scripts\windows\Env.ps1; cmake --build --preset build-windows-debug --target HockeyAssetTests`: passed.
`.\build\windows-debug\apps\asset_tests\HockeyAssetTests.exe --root .`: passed, 378 passed, 0 failed, includes `SvgTextureImportTests`.
```

- [x] **Step 6: Commit SVG import/cook integration**

```powershell
git add libs/assets/src/AssetManager.cpp libs/assets/include/Hockey/Assets/Importers/TextureImporter.hpp libs/assets/src/Importers/TextureImporter.cpp libs/assets/src/Cookers/TextureCooker.cpp
git commit -m "feat: cook svg files as textures"
```

Committed as `53a242a`.

---

### Task 5: Verify Existing Decal Renderer Compatibility

**Files:**
- Read: `libs/renderer/src/Renderer.cpp`
- Read: `data/shaders/src/decals.glsl`
- Read: `apps/renderer_tests/src/RendererDecalContractTests.cpp`
- No production renderer edits expected.

- [x] **Step 1: Confirm material-backed decals still consume texture assets**

Read `Renderer::Impl::ResolveAssetMaterial` and `Renderer::Impl::UpdateDecalSet` in `libs/renderer/src/Renderer.cpp`.

Expected facts:

```text
ResolveAssetMaterial loads MaterialAsset texture AssetIDs.
UpdateDecalSet binds frameDecals[i].material->desc.baseColorTexture.
decals.glsl samples uDecalBaseColorTex[i].
```

Confirmed in `libs/renderer/src/Renderer.cpp` and `data/shaders/src/decals.glsl`.

- [x] **Step 2: Run renderer no-GPU contracts**

Run:

```powershell
cmake --build --preset build-windows-debug --target HockeyRendererTests
.\build\windows-debug\apps\renderer_tests\HockeyRendererTests.exe --root . --no-gpu
```

Expected result:

```text
RendererDecalContractTests passes.
```

Actual result:

```text
`. .\scripts\windows\Env.ps1; cmake --build --preset build-windows-debug --target HockeyRendererTests`: passed.
`.\build\windows-debug\apps\renderer_tests\HockeyRendererTests.exe --root . --no-gpu`: passed, 378 passed, 0 failed, includes `RendererDecalContractTests`.
```

- [x] **Step 3: Commit only if renderer tests required a test-only update**

No commit is needed if the renderer tests pass unchanged. If a test assertion needs to be expanded to document SVG-as-texture compatibility, commit only the renderer test file:

```powershell
git add apps/renderer_tests/src/RendererDecalContractTests.cpp
git commit -m "test: document svg decal renderer compatibility"
```

No renderer commit needed; tests passed unchanged.

---

### Task 6: Update Phase Status

**Files:**
- Modify: `docs/phase-status/phase-05-asset-pipeline.md`
- Read: `docs/phase-status/phase-03-vulkan-renderer.md`

- [x] **Step 1: Update Phase 5 when SVG support starts**

If implementation is partially complete but not verified, add this under `Started / Partial` in `docs/phase-status/phase-05-asset-pipeline.md`:

```markdown
- [ ] SVG source textures are being added for rink decal materials; import/cook verification is still in progress.
```

Not needed; SVG support was completed and verified in the same pass.

- [x] **Step 2: Update Phase 5 when SVG support is complete**

When the asset tests pass, add this under `Finished - Content Pipelines`:

```markdown
- [x] SVG source textures can be rasterized during cooking and used by material-backed decal assets for rink markings, logos, and ads.
```

Remove the matching partial bullet from `Started / Partial` in the same edit.

Added the completed SVG source texture bullet under `Finished - Content Pipelines`.

- [x] **Step 3: Leave Phase 3 unchanged unless renderer code changed**

Do not edit `docs/phase-status/phase-03-vulkan-renderer.md` when this work only changes SVG import/cook behavior. Existing Phase 3 decal rendering remains true.

Phase 3 left unchanged.

- [x] **Step 4: Commit phase status**

```powershell
git add docs/phase-status/phase-05-asset-pipeline.md
git commit -m "docs: update asset pipeline svg decal status"
```

Committed as `c8101a4`; only the SVG status hunk was staged because the file had pre-existing unrelated local edits.

---

### Task 7: Run Completion Verification

**Files:**
- Read: `justfile`
- No source edits expected.

- [x] **Step 1: Inspect available command surface**

Run:

```powershell
just --list
```

Expected result:

```text
The command list includes verify and focused build/test commands.
```

Actual result:

```text
`just --list`: passed; command list includes `verify`, `build-debug`, `test`, `smoke-text`, and related focused recipes.
```

- [x] **Step 2: Run focused verification**

Run:

```powershell
cmake --build --preset build-windows-debug --target HockeyAssetTests HockeyRendererTests HockeyAssetTool
.\build\windows-debug\apps\asset_tests\HockeyAssetTests.exe --root .
.\build\windows-debug\apps\renderer_tests\HockeyRendererTests.exe --root . --no-gpu
```

Expected result:

```text
Asset tests pass with SvgTextureImportTests.
Renderer no-GPU tests pass.
```

Actual result:

```text
`. .\scripts\windows\Env.ps1; cmake --build --preset build-windows-debug --target HockeyAssetTests HockeyRendererTests HockeyAssetTool`: passed.
`.\build\windows-debug\apps\asset_tests\HockeyAssetTests.exe --root .`: passed, 378 passed, 0 failed, includes `SvgTextureImportTests`.
`.\build\windows-debug\apps\renderer_tests\HockeyRendererTests.exe --root . --no-gpu`: passed, 378 passed, 0 failed, includes `RendererDecalContractTests`.
```

- [x] **Step 3: Run normal completion verification if practical**

Run:

```powershell
just verify
```

Expected result:

```text
All configured project verification passes.
```

If `just verify` cannot run because a local tool or GPU/display dependency is unavailable, record the exact failing command and error output in this plan under `Verification Results`.

Actual result:

```text
`just verify`: failed in `scripts/windows/test.ps1` because `HockeyGameplayTests` reported 1018 passed, 3 failed:
- `apps/gameplay_tests/src/SettingsTuningTests.cpp:140` `HK_CHECK_NEAR(tuning.value.skater.maxSpeed, 9.0f)`
- `apps/gameplay_tests/src/SettingsTuningTests.cpp:141` `HK_CHECK_NEAR(tuning.value.skater.boostImpulse, 7.5f)`
- `apps/gameplay_tests/src/SettingsTuningTests.cpp:182` `HK_CHECK_NEAR(roundTrip.skater.boostImpulse, 7.5f)`

The same `just verify` run reported `HockeyAssetTests` 378 passed, 0 failed and `HockeyRendererTests` 398 passed, 0 failed. `smoke-text` did not run because `verify` stops at the failing `test` recipe.
```

- [x] **Step 4: Record verification results in this plan**

Append exact command results under `Verification Results` below using this format:

Recorded below.

- [x] **Step 5: Final commit**

If any verification notes were added to this plan after prior commits:

```powershell
git add -f plans/svg-decal-assets.md
git commit -m "docs: record svg decal verification"
```

Final plan/verification notes are recorded in this file and committed as the final task commit.

---

## Acceptance Criteria

- `.svg` files under `data/raw/**` are discovered as `AssetType::Texture`.
- `TextureImporter` imports `.svg` metadata without stb image validation.
- `TextureCooker` rasterizes `.svg` files to cooked `.tex.bin` assets.
- Cooked SVG textures load as `TextureAsset` with RGBA8 data, sRGB color space, base-color semantic, and generated mips.
- `MaterialSource` can reference an SVG in `Textures.BaseColor`.
- Cooked `MaterialAsset::baseColorTexture` resolves to the SVG texture asset ID.
- Existing `DecalComponent` projection renders the material without renderer code changes.
- `HockeyAssetTests` pass with SVG coverage.
- `HockeyRendererTests --no-gpu` pass to guard the existing renderer contract.
- `docs/phase-status/phase-05-asset-pipeline.md` reflects the completed SVG texture-source support.

## Risks And Constraints

- SVG text rendering depends on available fonts. Rink logos and ads should prefer outlined paths for deterministic builds.
- SVG filters, scripts, animation, and external references are outside this feature. Static paths, shapes, fills, gradients, and embedded style data are the target authoring set.
- The cooked texture resolution is fixed at cook time. Oversized rink ads should be authored with clear dimensions and rely on the 2048-pixel clamp unless project settings add per-asset texture import overrides.
- SVG rasterization happens in `hockey_assets`; runtime clients and servers consume cooked texture data only.
- This plan does not add new decal sorting, angle rejection, CPU culling, or `receivesDecals` mesh flags.

## Verification Results

- `.\scripts\windows\build_debug.ps1`: passed after adding the red SVG texture test.
- `.\build\windows-debug\apps\asset_tests\HockeyAssetTests.exe --root .`: failed as expected with 367 passed, 3 failed; failures were limited to `SvgTextureImportTests` SVG cook/load assertions before SVG rasterization support was implemented.
- `. .\scripts\windows\Env.ps1; cmake --preset windows-debug`: passed after approved cleanup of generated `build/windows-debug/vcpkg_installed`; vcpkg installed `lunasvg:x64-windows@3.5.0` and reported target `lunasvg::lunasvg`.
- `. .\scripts\windows\Env.ps1; cmake --build --preset build-windows-debug --target hockey_assets`: passed after adding LunaSVG dependency wiring.
- `. .\scripts\windows\Env.ps1; cmake --build --preset build-windows-debug --target hockey_assets`: passed after adding `SvgRasterizer`.
- `. .\scripts\windows\Env.ps1; cmake --build --preset build-windows-debug --target HockeyAssetTests`: passed after SVG import/cook integration.
- `.\build\windows-debug\apps\asset_tests\HockeyAssetTests.exe --root .`: passed, 378 passed, 0 failed, includes `SvgTextureImportTests`.
- `. .\scripts\windows\Env.ps1; cmake --build --preset build-windows-debug --target HockeyRendererTests`: passed.
- `.\build\windows-debug\apps\renderer_tests\HockeyRendererTests.exe --root . --no-gpu`: passed, 378 passed, 0 failed, includes `RendererDecalContractTests`.
- `just --list`: passed; listed `verify`, `build-debug`, `test`, and `smoke-text`.
- `. .\scripts\windows\Env.ps1; cmake --build --preset build-windows-debug --target HockeyAssetTests HockeyRendererTests HockeyAssetTool`: passed.
- `.\build\windows-debug\apps\asset_tests\HockeyAssetTests.exe --root .`: passed, 378 passed, 0 failed, includes `SvgTextureImportTests`.
- `.\build\windows-debug\apps\renderer_tests\HockeyRendererTests.exe --root . --no-gpu`: passed, 378 passed, 0 failed, includes `RendererDecalContractTests`.
- `just verify`: failed in `HockeyGameplayTests` with 1018 passed, 3 failed. Failing assertions were `SettingsTuningTests.cpp:140` `tuning.value.skater.maxSpeed == 9.0f`, `SettingsTuningTests.cpp:141` `tuning.value.skater.boostImpulse == 7.5f`, and `SettingsTuningTests.cpp:182` `roundTrip.skater.boostImpulse == 7.5f`. The run reported `HockeyAssetTests` 378 passed, 0 failed and `HockeyRendererTests` 398 passed, 0 failed before stopping at the failing `test` recipe; `smoke-text` did not run.

## Plan Self-Review

- Spec coverage: SVG discovery, import, cook, material dependency, existing decal renderer compatibility, phase status, and verification are covered by tasks.
- Placeholder scan: no unspecified implementation blocks are intentionally left open.
- Type consistency: all referenced types exist today except `SvgInfo` and `SvgRasterizer`, which are introduced in Task 3 before use in Task 4.
