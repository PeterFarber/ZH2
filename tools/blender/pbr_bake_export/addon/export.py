"""Export preflight and Blender GLB export helpers."""

from __future__ import annotations

from pathlib import Path


class ExportPreflightError(RuntimeError):
    pass


def validate_output_folder(asset_dir: Path, overwrite: bool) -> None:
    if asset_dir.exists() and not overwrite:
        raise ExportPreflightError(f"asset folder already exists: {asset_dir}")

    asset_dir.parent.mkdir(parents=True, exist_ok=True)


def duplicate_for_export(source_obj, export_name: str):
    import bpy

    duplicate = source_obj.copy()
    duplicate.data = source_obj.data.copy()
    duplicate.name = export_name
    duplicate.data.name = f"{export_name}_mesh"
    bpy.context.collection.objects.link(duplicate)
    return duplicate


def ensure_uv_layer(obj) -> None:
    uv_layers = getattr(obj.data, "uv_layers", None)
    if uv_layers is not None and len(uv_layers) > 0:
        return

    import bpy

    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.mode_set(mode="EDIT")
    try:
        bpy.ops.mesh.select_all(action="SELECT")
        bpy.ops.uv.smart_project(angle_limit=1.15192, island_margin=0.02)
    finally:
        bpy.ops.object.mode_set(mode="OBJECT")


def apply_low_poly_decimation(obj, ratio: float) -> None:
    import bpy

    modifier = obj.modifiers.new("ZH2_low_poly_decimate", "DECIMATE")
    modifier.ratio = max(0.01, min(1.0, ratio))

    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=modifier.name)


def export_glb(obj, glb_path: Path) -> None:
    import bpy

    glb_path.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.gltf(
        filepath=str(glb_path),
        export_format="GLB",
        use_selection=True,
        export_materials="EXPORT",
        export_apply=True,
    )


def remove_temp_object(obj) -> None:
    import bpy

    mesh = obj.data
    bpy.data.objects.remove(obj, do_unlink=True)
    if mesh.users == 0:
        bpy.data.meshes.remove(mesh)
