# ZH2 PBR Bake Export Blender Add-on

This add-on bakes and exports selected Blender mesh objects into modular PBR asset folders.

## Install In Blender

Build the installable add-on zip:

```powershell
python tools\blender\pbr_bake_export\package_addon.py
```

The script writes:

```text
out/blender/zh2_pbr_bake_export.zip
```

In Blender:

1. Open `Edit > Preferences > Add-ons`.
2. Click the install button and select `out/blender/zh2_pbr_bake_export.zip`.
3. Enable `Hockey PBR Bake Export`.
4. Open the 3D View sidebar and use the `ZH2 Assets` tab.

For live Python development, keep using the repo checkout and reload the add-on
after edits. The zip flow is the normal path for installing or refreshing the
tool in Blender.

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

## Poly Haven Materials

Set `Material Source` to `Poly Haven`, enter a query, and click `Search Poly
Haven`. The add-on fills the material dropdown with results formatted as:

```text
Material Name (asset_id)
```

Choose a search result, then click `Download Material`. Downloaded materials are
cached under the add-on's `Poly Haven Cache Root` preference and appear in the
separate `Downloaded Material` dropdown. Select a downloaded material to reuse it
without downloading again.

`Apply Material to Selected` assigns the selected downloaded material to all
selected mesh objects in the current Blender scene. `Bake Selected` and `Export
Selected` also use the selected downloaded material when `Material Source` is
set to `Poly Haven`.

Some non-metal Poly Haven materials do not publish a metallic texture. The
add-on still downloads the material and leaves Blender's Principled Metallic
input at its default `0.0` value.

## Bake And Export

Use `Bake Selected` to write/update the texture set and manifest for the
selected mesh objects. Use `Export Selected` to write/update the GLB and manifest.
This lets you preview or adjust baked materials before exporting the model.

## Manual Import

This add-on does not call `HockeyAssetTool`. Move or import exported GLBs manually when they are ready for the engine asset pipeline.
