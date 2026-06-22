# Blender PBR Bake And GLB Export Add-on Design

Date: 2026-06-22

## Summary

Build a focused Blender Python add-on for authoring high-resolution PBR modular game assets. The add-on operates on selected mesh objects, can download Poly Haven texture materials, optionally generates low-poly bake/export targets, bakes 4K or 8K PBR textures, and exports one self-contained asset folder per selected object.

The add-on is source-authoring tooling only. It does not call the engine asset pipeline in v1. Exported assets are written under `assets/` for manual import into the game project.

## Goals

- Provide one-click `Bake + Export Selected` workflow from Blender.
- Export one modular asset per selected object.
- Support high-poly and low-poly export modes.
- Include an `Autogenerate low-poly mesh` checkbox for low-poly mode.
- Download and apply Poly Haven PBR texture materials.
- Support 4K and 8K texture output.
- Preserve external baked texture files beside the exported GLB.
- Avoid destructive changes to user-authored Blender objects.
- Keep the add-on isolated from C++ runtime, renderer, and asset-cooking code.

## Non-goals

- Do not run `HockeyAssetTool` automatically in v1.
- Do not implement a full Blender asset browser/cache UI in v1.
- Do not import exported files into `data/raw` automatically.
- Do not create runtime engine code.
- Do not delete or overwrite user-authored Blender objects.
- Do not perform commercial Poly Haven API usage without the user obtaining the required license or sponsorship.

## Existing Project Context

The repository already contains a Phase 5 asset pipeline with GLB import, material/texture import, cooking, cooked loading, metadata sidecars, and `HockeyAssetTool`. This add-on complements that pipeline by creating high-quality source assets for manual import rather than changing the engine-side asset system.

Relevant engine path expectations:

- Engine manual import path remains project-specific and user-controlled.
- The new authoring output root is `assets/`.
- Exported GLBs can later be copied or imported into the existing asset pipeline manually.

## Poly Haven API Constraints

Poly Haven provides a public API at `https://api.polyhaven.com` with endpoints for asset lists, individual asset info, and file download trees. The API page says requests require a unique User-Agent header, and commercial API usage requires a custom license and typically sponsorship.

The add-on must:

- Send a configurable unique User-Agent header with Poly Haven API requests.
- Surface Poly Haven API/license notes in add-on preferences or panel text.
- Use official API endpoints rather than scraping the website.
- Handle endpoint or file-layout failures gracefully.

Initial API usage:

- `/assets?type=textures` for texture search/listing.
- `/info/{id}` for material metadata.
- `/files/{id}` for downloadable texture file data.

Sources:

- https://polyhaven.com/our-api
- https://api.polyhaven.com/api-docs/swagger.json

## Add-on Location

Create repo-managed Blender tooling under:

```text
tools/blender/pbr_bake_export/
  __init__.py
  addon/
    __init__.py
    operators.py
    panel.py
    preferences.py
    polyhaven.py
    materials.py
    bake.py
    export.py
    manifest.py
```

If Blender add-on installation is easier with a flat package, implementation may keep the final package import-compatible while preserving these responsibilities.

## Architecture

The add-on is split into small Blender Python modules:

- `preferences`: stores output root, Poly Haven cache root, User-Agent, overwrite behavior, and default resolution.
- `panel`: exposes the workflow UI in Blender's 3D View sidebar.
- `polyhaven`: queries Poly Haven API, resolves texture download URLs, downloads files, and caches responses/files.
- `materials`: creates Blender Principled BSDF materials from downloaded texture sets or existing object materials.
- `bake`: prepares bake targets, creates bake images, performs PBR texture baking, and writes PNG files.
- `export`: exports per-object GLB files with PBR materials.
- `manifest`: writes per-export `manifest.json`.
- `operators`: coordinates UI actions and validates preconditions.

The add-on depends only on Blender's Python API and Python standard library modules available inside Blender, such as `json`, `urllib`, `pathlib`, and `datetime`.

## UI Design

The panel will expose:

- Output root: default `//assets`, where `//` means relative to the current `.blend` file.
- Texture resolution: `4K` or `8K`, with an internal small test resolution available only for development/testing.
- Asset mode: `High Poly` or `Low Poly`.
- Low-poly option: `Autogenerate low-poly mesh` checkbox.
- Low-poly target settings: conservative decimation ratio or target face percentage.
- Poly Haven search field.
- Poly Haven material result selector.
- Material source option: selected Poly Haven material or existing object material.
- Allow lower-resolution material fallback checkbox.
- Overwrite existing exports checkbox.
- `Download Material` button.
- `Bake + Export Selected` button.

The panel reports progress and errors through Blender reports and console output. Long network downloads and 8K bakes may block Blender in v1; progress feedback should still identify the current object and current stage.

## Workflow

User workflow:

1. Select one or more mesh objects in Blender.
2. Pick a Poly Haven texture material, or choose existing object material.
3. Choose `4K` or `8K`.
4. Choose `High Poly` or `Low Poly`.
5. In low-poly mode, optionally enable `Autogenerate low-poly mesh`.
6. Click `Bake + Export Selected`.

For each selected mesh object:

1. Sanitize the object name for filesystem-safe paths.
2. Create `assets/<object_name>/`.
3. Prepare a non-destructive working duplicate.
4. Apply downloaded or existing PBR material.
5. If low-poly autogeneration is enabled, duplicate and decimate the working mesh.
6. Bake required PBR maps to images at the selected resolution.
7. Save external PNG textures.
8. Assign baked textures to the export material.
9. Export one GLB for that object.
10. Write `manifest.json`.
11. Clean up temporary working objects and images unless a debug option keeps them.

## Output Contract

Each selected object exports to:

```text
assets/<object_name>/
  <object_name>.glb
  textures/
    <object_name>_basecolor.png
    <object_name>_normal.png
    <object_name>_roughness.png
    <object_name>_metallic.png
    <object_name>_ao.png
  manifest.json
```

Rules:

- One selected object produces one folder and one GLB.
- Texture filenames are lowercase, ASCII-safe, and path-safe.
- The GLB includes the mesh and a PBR material.
- External PNG files are retained for inspection and reuse.
- The original selected object is not deleted.
- Existing output folders are not overwritten unless overwrite is enabled.

## High-poly Mode

High-poly mode exports the selected object as the render/export target. Baking still writes the same texture set so the output contract is consistent.

This mode is useful for high-resolution asset inspection, hero objects, or source-quality modular assets where polygon count is acceptable.

## Low-poly Mode

Low-poly mode exports a low-poly target and bakes PBR detail onto it.

If `Autogenerate low-poly mesh` is enabled:

- The add-on duplicates the selected object.
- It applies a conservative decimation modifier to the duplicate.
- It unwraps or verifies UVs for baking if needed.
- It bakes detail/material maps onto the generated low-poly duplicate.
- It exports only the generated low-poly duplicate.

If `Autogenerate low-poly mesh` is disabled in v1:

- The add-on reports that manual low-poly target selection is not yet implemented.
- The user can switch back to high-poly mode or enable autogeneration.

Manual high-to-low target pairing can be added later without changing the output folder contract.

## Texture Baking

Required maps:

- Base color, sRGB.
- Normal, linear.
- Roughness, linear.
- Metallic, linear.
- Ambient occlusion, linear.

Supported output resolutions:

- 4096 x 4096.
- 8192 x 8192.

Blender bake settings:

- Use Cycles for baking.
- Preserve or temporarily set render engine as needed, then restore original settings.
- Set bake margin/padding to reduce texture seams.
- Create one image per map per object.
- Save PNG output for v1.

The implementation should avoid assuming every source material has every channel. Missing channels should use sensible constants and record a warning in the manifest.

## Poly Haven Material Handling

The add-on will:

- Search Poly Haven texture assets by keyword.
- Resolve available file entries for a selected texture asset.
- Prefer the requested 8K or 4K files.
- Download required maps into a local cache.
- Build a Blender material using Principled BSDF.

Map role matching should cover common Poly Haven file roles:

- diffuse, albedo, base color -> base color.
- normal, nor, nrm -> normal.
- roughness, rough -> roughness.
- metallic, metalness, metal -> metallic.
- ao, ambient occlusion -> AO.

If a selected material does not provide a required map at the requested resolution:

- The add-on may fall back to another available resolution only if the user enables fallback behavior.
- Otherwise it fails visibly before export.

## Manifest

Each export writes:

```json
{
  "asset_name": "wall_panel",
  "source_object": "Wall Panel",
  "exported_at": "2026-06-22T12:00:00-04:00",
  "mode": "LowPoly",
  "autogenerated_low_poly": true,
  "resolution": "4096",
  "material_source": "Poly Haven",
  "polyhaven_asset_id": "example_id",
  "glb": "wall_panel.glb",
  "textures": {
    "basecolor": "textures/wall_panel_basecolor.png",
    "normal": "textures/wall_panel_normal.png",
    "roughness": "textures/wall_panel_roughness.png",
    "metallic": "textures/wall_panel_metallic.png",
    "ao": "textures/wall_panel_ao.png"
  },
  "warnings": []
}
```

The timestamp format should be ISO 8601 with timezone when practical.

## Error Handling

The add-on fails visibly when:

- No mesh objects are selected.
- Output root cannot be created.
- The target asset folder exists and overwrite is disabled.
- Poly Haven API requests fail.
- Poly Haven download URLs are missing or unusable.
- Required material maps are unavailable and fallback is disabled.
- 4K/8K image creation fails.
- Blender bake fails.
- GLB export fails.

Failure should leave the original selected objects unchanged. Partial output for the currently failing object should be left in place only if it is useful for diagnosis; otherwise incomplete files should be cleaned up.

## Safety

- Duplicate objects before destructive operations.
- Do not delete selected source objects.
- Do not apply modifiers to source objects.
- Restore render engine and relevant bake settings after export.
- Avoid hidden writes outside configured output/cache paths.
- Do not automatically import into `data/raw`.
- Do not automatically modify engine config, CMake, cooked assets, or phase status docs.

## Testing And Verification

Implementation verification:

- Run Python syntax checks on add-on modules.
- Register the add-on in Blender through MCP or a local Blender Python command where possible.
- Run a Blender smoke test using a simple mesh and a small internal test resolution.
- Confirm output folder, GLB, textures, and manifest are written.
- Confirm the original source object remains in the scene.
- Confirm overwrite-disabled behavior blocks existing folders.

Manual verification:

- Search and download a Poly Haven texture.
- Apply the material to a selected object.
- Run a 4K bake/export.
- Run an 8K bake/export if local memory and Blender stability allow.
- Import the GLB manually into the existing project pipeline and validate with `HockeyAssetTool` when desired.

## Phase Status

This add-on is authoring tooling that complements Phase 5 but does not change the implemented engine asset pipeline state. No phase status change is expected for the design/spec change.
