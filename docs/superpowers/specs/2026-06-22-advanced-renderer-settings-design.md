# Advanced Renderer Project Settings Design

Date: 2026-06-22

## Summary

Revamp Project Settings so Editor, Client, and Server settings remain editable, while Editor and Client graphics expose a more complete, guarded set of renderer controls for lighting and shadows. The goal is to make shadows, lighting budgets, filtering, cascade behavior, and related post-processing tunable from config/UI without requiring shader or renderer source edits.

Client settings edited from the editor remain project defaults. Build output also receives copied config files so a built client can run from its local build directory and users can edit that copied `client.toml` without modifying repo source config.

## Goals

- Keep Project Settings pages for Editor, Client, and Server.
- Make Editor and Client renderer settings substantially more complete.
- Expose safe advanced controls for shadows, local lights, AO, reflections, tone mapping, and debug overlays.
- Move currently hard-coded directional and local shadow constants into `RendererSettings`.
- Persist new renderer controls under `[renderer]` in `editor.toml` and `client.toml`.
- Apply live-compatible renderer settings to the running editor renderer when possible.
- Rebuild renderer targets when changed settings affect shadow atlas sizes or render target layout.
- Copy app config files into build output so local build runs can use build-local config.
- Preserve per-scene light authoring through `LightComponent`; Project Settings owns global renderer budgets and algorithms, not individual scene lights.

## Non-goals

- Do not expose unbounded debug values that can trivially break rendering.
- Do not make the dedicated server link renderer, Vulkan, editor, or ImGui.
- Do not move per-light color, intensity, range, or cone controls out of scene component editing.
- Do not implement reflection probe rendering, decals, TAA, MSAA, or Phase 9 polish systems.
- Do not create a full packaging/installer pipeline in this change.
- Do not add networking behavior to client/server settings.

## Existing Project Context

`ProjectSettingsPanel` currently edits:

- `data/config/editor.toml` for editor app, renderer, physics, gameplay, scene, input, and asset options.
- `data/config/client.toml` for game client app, renderer, physics, gameplay, scene, input, and asset options.
- `data/config/server.toml` for headless server options.
- `data/editor/editor_settings.toml` for user editor preferences.

`RendererSettings` currently exposes broad quality settings plus only three direct shadow controls: `shadowQuality`, `shadowDistance`, and `contactShadows`. Several important renderer behaviors are currently hard-coded in renderer/shader code:

- Directional shadow atlas resolution and cascade count are derived only from `ShadowQuality`.
- Local light shadow atlas resolution is derived only from `ShadowQuality`.
- Directional cascade split lambda, overlap, blend width, and receiver offset values are constants.
- Directional and local PCF radii are derived only from `ShadowQuality`.
- Directional receiver bias, contact shadow bias, and local shadow bias are constants in GLSL.
- Shadow pass raster depth bias is fixed in Vulkan pipeline creation.
- Maximum rendered lights and local shadow tiles are compile-time shader limits.

`LightComponent` remains scene data and already exposes:

- Type: directional, point, spot.
- Color.
- Intensity.
- Range.
- Inner and outer cone angles.
- Casts shadows.

## Settings Model

Add guarded renderer settings fields. Values are clamped during load and in the UI.

### Lighting Budgets

- `maxRenderedLights`: maximum active lights packed into the scene UBO, clamped to the shader limit of 16.
- `maxLocalShadowTiles`: maximum local shadow atlas tiles used in a frame, clamped to the shader limit of 16. A spot light consumes one tile; a point light consumes six.

### Shadow Atlas Controls

- `directionalShadowAtlasResolution`: requested cascaded directional shadow atlas size. `0` means use the `ShadowQuality` preset mapping.
- `localShadowAtlasResolution`: requested point/spot shadow atlas size. `0` means use the `ShadowQuality` preset mapping.
- `shadowCascadeCount`: requested directional cascade count. `0` means use the `ShadowQuality` preset mapping. Values clamp to 0-4.

### Directional Cascade Controls

- `shadowCascadeSplitLambda`: blend between uniform and logarithmic split placement. Higher values favor near-camera resolution.
- `shadowCascadeOverlapScale`: overlap amount used when fitting cascade caster regions.
- `shadowCascadeMinOverlap`: minimum world-space overlap.
- `shadowCascadeMaxOverlapScale`: maximum overlap as a fraction of shadow distance.
- `shadowCascadeBlendScale`: receiver-side split blend width as a fraction of cascade range.
- `shadowCascadeMinBlend`: minimum split blend width.

### Directional Shadow Filter And Bias

- `directionalShadowPcfRadius`: PCF radius in texels. `0` means a single compare sample; `1` means 3x3; `2` means 5x5.
- `directionalShadowNormalOffsetScale`: receiver normal offset as a multiple of cascade texel size.
- `directionalShadowNormalOffsetMin`.
- `directionalShadowNormalOffsetMax`.
- `directionalShadowBiasBase`.
- `directionalShadowBiasSlope`.
- `directionalShadowBiasMin`.
- `directionalShadowBiasMax`.
- `directionalShadowDepthBiasConstant`: Vulkan raster depth bias constant factor for shadow map writing.
- `directionalShadowDepthBiasSlope`: Vulkan raster depth bias slope factor for shadow map writing.

### Contact Shadow Controls

- `contactShadowDistance`: maximum view-space distance where contact tightening is evaluated.
- `contactShadowStrength`: blend strength applied to the tighter contact sample.
- `contactShadowNormalOffsetScale`.
- `contactShadowNormalOffsetMin`.
- `contactShadowBiasBase`.
- `contactShadowBiasSlope`.
- `contactShadowBiasMin`.
- `contactShadowBiasMax`.

### Local Shadow Controls

- `localShadowPcfRadius`.
- `localShadowBiasScale`.
- `localShadowBiasMin`.
- `localShadowBiasMax`.

## Presets

Graphics presets continue to work. Low, Medium, High, and Ultra set guarded defaults for the new fields. Custom keeps caller-provided values.

Suggested defaults:

- Low: small atlases, 2 cascades, PCF 1, contact shadows off.
- Medium: medium atlases, 3 cascades, PCF 1, contact shadows off.
- High: 4096 directional atlas, 2048 local atlas, 4 cascades, PCF 1, contact shadows on.
- Ultra: 8192 directional atlas, 4096 local atlas, 4 cascades, PCF 2, contact shadows on.

If a user edits any advanced renderer field in Project Settings, the preset becomes `Custom`.

## Renderer Data Flow

1. Editor/client config is loaded into `Config`.
2. `LoadRendererSettings` parses existing and new `[renderer]` keys, clamps them, and fills `RendererSettings`.
3. `RendererInitInfo.settings` passes the values into the renderer at startup.
4. `Renderer::ApplySettings` compares old/new settings and requests rebuilds when atlas sizes, cascade count, AO, bloom, or preset-dependent target layout changes.
5. `GatherScene` uses `RendererSettings` for light count limits, local tile limits, cascade computation, PCF radius, atlas parameters, and shadow data uploaded to the scene UBO.
6. Shadow sampling shaders read new bias/filter/contact values from the scene UBO instead of hard-coded constants.
7. Shadow pass pipeline depth bias uses settings at pipeline creation; changing it triggers a pipeline rebuild or controlled renderer rebuild path.

## Shader Interface

Extend the scene UBO carefully while preserving alignment:

- Keep existing camera and material interfaces unchanged.
- Add one or more `vec4` parameter groups for directional shadow bias/filter/contact settings and local shadow bias settings.
- Update C++ `SceneGPU` packing to match GLSL exactly.
- Update renderer contract tests to assert the new settings reach shader-visible data.

No descriptor set numbers or bindings should change.

## Project Settings UI

Keep one Project Settings panel, but restructure it so the navigation is easier to scan:

- Editor
  - Application
  - Window/Input
  - Graphics
  - Lighting & Shadows
  - Scene/Autosave
  - Assets
  - Physics Preview
  - Gameplay Preview
  - Preferences
- Client
  - Application
  - Window/Input
  - Graphics
  - Lighting & Shadows
  - Physics
  - Gameplay
  - Startup Scene
- Server
  - Application
  - Simulation
  - Physics
  - Gameplay
  - Startup Scene

The Lighting & Shadows page uses grouped controls:

- Budgets.
- Shadow atlases.
- Directional cascades.
- Directional filtering and bias.
- Contact shadows.
- Local light shadows.
- AO/reflections/GI adjacent controls.

The UI should use sliders/drag controls with clear numeric ranges. It should not rely on explanatory wall text inside the app; section names and labels should be enough.

## Build-Local Config Flow

Add a build copy step for app configs:

```text
build/<preset>/data/config/editor.toml
build/<preset>/data/config/client.toml
build/<preset>/data/config/server.toml
```

Directly launching a built executable should be able to find a local `data/config/*.toml` beside or above the executable without requiring the repo root. Existing helper scripts can continue to pass `--root <repo>` for repo-authored development runs, but direct build-local runs should use copied config.

This change is intentionally smaller than packaging. It gives the client the same editable local config shape a future package/deploy command will use.

## Error Handling And Validation

- Config load clamps out-of-range numeric values to safe limits.
- Atlas requests are still clamped to Vulkan device image limits.
- If a custom atlas value is invalid or zero, fall back to quality-derived defaults.
- If max lights exceeds the shader limit, clamp to 16.
- If max local shadow tiles exceeds the shader limit, clamp to 16.
- If cascade count is zero or shadows are off, directional shadows are disabled.
- UI save failures continue to report status in the Project Settings panel.

## Testing And Verification

Automated tests:

- Renderer settings round-trip persists new fields.
- Graphics presets populate new fields with expected safe defaults.
- Shadow atlas/cascade helpers respect explicit custom values and quality fallback.
- Config load clamps unsafe values.
- Renderer contract tests prove shadow settings reach C++/GLSL interfaces.
- Project Settings tests cover editor/client/server config save paths where practical.
- Path/config-root tests cover build-local `data/config` discovery.

Manual/GPU verification:

- Build and run editor.
- Change Editor Lighting & Shadows settings and verify renderer applies live-compatible settings.
- Run client from the build directory and verify it uses copied `build/<preset>/data/config/client.toml`.
- Capture directional and local shadow screenshots from `Main.scene.yaml` after tuning.

Completion verification:

- Shader compilation.
- Focused renderer/editor/core tests.
- `just verify`.

## Phase Status

This work extends Phase 3 renderer settings and Phase 4 Project Settings UI. The matching phase status docs should be updated if implementation changes the verified state of renderer settings, shadow tuning, or editor Project Settings behavior.
