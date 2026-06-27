# Phase 5 - Asset Pipeline

Status: Implemented, with advanced format and future-system gaps.

Source material:

- [x] Read `docs/phase-plans/phase-05-asset-pipeline.md`.
- [x] Read `docs/phase-rules/090-phase-5-asset-pipeline.mdc`.
- [x] Checked `docs/project-structure.md`.
- [x] Checked current source/CMake/test state.

## What This Phase Implements

- [x] `hockey_assets` CPU-side content pipeline.
- [x] Asset identity, handles, metadata, database, registry, and manager.
- [x] Importer/cooker framework.
- [x] Texture, material, glTF/model/mesh, shader, scene, and prefab pipelines.
- [x] Cooked asset formats, dependency tracking, dirty/missing validation, and hot reload.
- [x] Renderer/editor integration for asset references and loaded content.
- [x] `HockeyAssetTool` command-line workflows.

## Finished - Target And Boundaries

- [x] `hockey_assets` target exists.
- [x] `HockeyAssetTool` target exists.
- [x] `HockeyAssetTests` target exists.
- [x] Assets depend on core only at the public boundary.
- [x] Assets privately own fastgltf, meshoptimizer, shaderc, stb, yaml-cpp, and glm usage.
- [x] Assets do not link renderer, editor, ECS, physics, gameplay, or networking.
- [x] Dedicated server remains headless and does not need renderer/editor for asset code.

## Finished - Identity, Metadata, Database

- [x] `AssetID` exists.
- [x] `AssetHandle` exists.
- [x] Asset paths and path normalization exist.
- [x] Asset hashes exist.
- [x] Metadata sidecars exist.
- [x] Asset events exist.
- [x] Asset database exists and serializes.
- [x] Asset registry exists for runtime instances.
- [x] Asset manager coordinates database, import, cook, load, and validation work.
- [x] Stable uint64 asset IDs are used intentionally.

## Finished - Import/Cook Framework

- [x] Importer interface exists.
- [x] Cooker interface exists.
- [x] Raw-to-cooked flow exists.
- [x] Dirty detection exists.
- [x] Missing-file validation exists.
- [x] Dependency tracking exists.
- [x] Circular dependency detection exists.
- [x] Cooked asset database serialization exists.
- [x] Hot reload events exist.
- [x] Move detection preserves asset identity when raw files are renamed/moved.

## Finished - Content Pipelines

- [x] Texture import path exists through stb.
- [x] Texture cook path exists.
- [x] Texture `maxSize` downsample/clamp behavior exists.
- [x] Material import and YAML serialization exist.
- [x] Material cook path exists.
- [x] Canonical reusable visual materials are authored as raw PBR material assets under `data/raw/materials`.
- [x] PBR texture slot handling exists for base color, normal, metallic/roughness, occlusion, and emissive.
- [x] glTF/GLB import exists through fastgltf.
- [x] Mesh optimization/cooking exists through meshoptimizer.
- [x] Canonical basic shape mesh assets exist for cube, sphere, cylinder, capsule, torus, and plane.
- [x] Tangent generation exists for glTF primitives that omit tangents.
- [x] Model cook/load path exists.
- [x] Shader import/cook path exists through shaderc.
- [x] Scene asset tracking exists.
- [x] Prefab asset tracking exists.
- [x] Default waypoint marker prefab is tracked as a raw/cooked prefab asset.
- [x] Player prefab assets embed cooked stick child authoring from raw stick mesh/material assets.
- [x] Runtime loaders exist for textures, meshes, models, materials, shaders, scenes, and prefabs.

## Finished - Tooling And Editor Integration

- [x] `HockeyAssetTool discover` workflow exists.
- [x] `HockeyAssetTool import` / `import-all` workflows exist.
- [x] `HockeyAssetTool cook` / `cook-all-dirty` / `recook-all` workflows exist.
- [x] `HockeyAssetTool validate` workflow exists.
- [x] Editor host honors raw/cooked asset path config.
- [x] Editor auto-discover, auto-import, and auto-cook-dirty config exists.
- [x] Project browser integrates asset database and asset actions.
- [x] Project browser can delete metadata without deleting raw/cooked payloads.
- [x] Asset field drawers and drag/drop assignment exist.
- [x] Editor material texture slot drag/drop exists.
- [x] Renderer consumes cooked/runtime asset data for meshes, materials, and textures.

## Finished - Tests And Verification

- [x] Asset ID tests exist.
- [x] Metadata tests exist.
- [x] Path tests exist.
- [x] Database tests exist.
- [x] Texture import tests exist.
- [x] glTF import tests exist.
- [x] Material tests exist.
- [x] Shader cooking tests exist.
- [x] Dependency and circular dependency tests exist.
- [x] Hot reload behavior tests exist.
- [x] Asset manager and registry behavior tests exist.

## Started / Partial

- [ ] `.ktx` and `.ktx2` files are classified, but no real KTX/Basis compression pipeline exists.
- [ ] `TextureImportSettings.compress` exists, but compression is not implemented.
- [ ] Cubemap/environment map cooking is not implemented.
- [ ] `Audio` asset type is a placeholder for Phase 9.
- [ ] `Animation` asset type is a placeholder for Phase 9.
- [ ] No LOD pipeline exists.
- [ ] No shader variant system exists.
- [ ] Asset tool CLI and editor browser workflows would benefit from more cross-platform end-to-end testing.
- [ ] KTX/HDR cook behavior is not fully tested.

## Left To Do

- [ ] Add KTX/Basis support only when a texture compression dependency is approved.
- [ ] Add compression/cubemap/environment map cooking when renderer IBL support exists.
- [ ] Add audio asset import/cook/load during Phase 9 audio work.
- [ ] Add animation clip/skeleton asset import/cook/load during Phase 9 animation work.
- [ ] Add LOD generation/import if required by performance goals.
- [ ] Add shader variant system if material/rendering complexity requires it.
- [ ] Add packaged cross-platform asset workflow verification.
