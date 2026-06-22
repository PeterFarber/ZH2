# ZH2 PBR Bake Export Blender Add-on

This add-on bakes selected Blender mesh objects into modular PBR asset folders.

## Install For Development

1. Open Blender.
2. Add your ZH2 repo root (`<repo-root>`) to `sys.path` in a Blender Python console or startup script.
3. Import and register `tools.blender.pbr_bake_export.addon`.

## Output

Each selected object exports under the configured output root. The default is `//assets/blender_exports`, and each selected object uses this layout:

```text
<configured-output-root>/<object_name>/
  <object_name>.glb
  textures/
    <object_name>_basecolor.png
    <object_name>_normal.png
    <object_name>_roughness.png
    <object_name>_metallic.png
    <object_name>_ao.png
  manifest.json
```

## Poly Haven API Notice

The Poly Haven API requires a unique User-Agent header. Configure it in the add-on preferences. Poly Haven notes that commercial API usage requires a custom license and typically sponsorship.

## Manual Import

This add-on does not call `HockeyAssetTool`. Move or import exported GLBs manually when they are ready for the engine asset pipeline.
