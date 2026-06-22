"""Blender operators coordinating PBR bake/export workflows."""

from __future__ import annotations

from datetime import datetime, timezone
from pathlib import Path

from .bake import bake_object_maps
from .export import (
    ExportPreflightError,
    apply_low_poly_decimation,
    duplicate_for_export,
    ensure_uv_layer,
    export_glb,
    remove_temp_object,
    validate_output_folder,
)
from .manifest import ExportManifest, write_manifest
from .materials import build_material_plan, create_blender_material
from .paths import TEXTURE_ROLES, build_asset_paths, sanitize_asset_name
from .polyhaven import PolyhavenClient, PolyhavenError, select_texture_files
from .types import AssetMode, AssetPaths, MaterialSource, TextureResolution


class _ClassStore:
    classes: list[type] = []


def _resolution_from_settings(value: str) -> TextureResolution:
    if value == "8192":
        return TextureResolution.EIGHT_K
    if value == "128":
        return TextureResolution.TEST
    return TextureResolution.FOUR_K


def _abspath(blender_path: str) -> Path:
    import bpy

    return Path(bpy.path.abspath(blender_path))


def _addon_preferences(context):
    import bpy  # noqa: F401

    addon_name = __package__
    return context.preferences.addons[addon_name].preferences


def _selected_mesh_objects(context):
    return [obj for obj in context.selected_objects if getattr(obj, "type", None) == "MESH"]


def _asset_mode_from_settings(value: str) -> AssetMode:
    if value == "HIGH":
        return AssetMode.HIGH_POLY
    return AssetMode.LOW_POLY


def _material_source_from_settings(value: str) -> MaterialSource:
    if value == "POLY_HAVEN":
        return MaterialSource.POLY_HAVEN
    return MaterialSource.EXISTING


def _polyhaven_cache_texture_paths(cache_root: Path, asset_id: str) -> dict[str, Path]:
    cache_dir = cache_root / asset_id
    return {role: cache_dir / f"{asset_id}_{role}.png" for role in TEXTURE_ROLES}


def _deduplicate_reserved_asset_name(asset_name: str, reserved_names: set[str] | None) -> str:
    occupied_names = {name.lower() for name in reserved_names or set()}
    candidate = asset_name
    suffix = 2
    while candidate.lower() in occupied_names:
        candidate = f"{asset_name}_{suffix}"
        suffix += 1
    return candidate


def _asset_paths_from_name(output_root: Path, asset_name: str) -> AssetPaths:
    asset_dir = Path(output_root) / asset_name
    texture_dir = asset_dir / "textures"
    textures = {
        role: texture_dir / f"{asset_name}_{role}.png"
        for role in TEXTURE_ROLES
    }

    return AssetPaths(
        asset_name=asset_name,
        asset_dir=asset_dir,
        texture_dir=texture_dir,
        glb_path=asset_dir / f"{asset_name}.glb",
        manifest_path=asset_dir / "manifest.json",
        textures=textures,
    )


def _build_operator_asset_paths(
    output_root: Path,
    object_name: str,
    reserved_names: set[str] | None,
    overwrite_existing: bool,
) -> AssetPaths:
    if not overwrite_existing:
        return build_asset_paths(output_root, object_name, reserved_names)

    asset_name = _deduplicate_reserved_asset_name(
        sanitize_asset_name(object_name),
        reserved_names,
    )
    return _asset_paths_from_name(output_root, asset_name)


def _relative_texture_paths(texture_paths: dict[str, Path], asset_dir: Path) -> dict[str, Path]:
    return {
        role: texture_path.relative_to(asset_dir)
        for role, texture_path in texture_paths.items()
    }


def _assign_material(obj, material) -> None:
    materials = getattr(getattr(obj, "data", None), "materials", None)
    if materials is None:
        obj.active_material = material
        return

    materials.clear()
    materials.append(material)


def _apply_material_source(
    export_obj,
    material_source: MaterialSource,
    material_name: str,
    texture_paths: dict[str, Path],
    polyhaven_asset_id: str,
    material_factory=create_blender_material,
    assign_material=_assign_material,
) -> None:
    if material_source is MaterialSource.EXISTING:
        return

    material_plan = build_material_plan(
        name=material_name,
        source=material_source,
        texture_paths=texture_paths,
        polyhaven_asset_id=polyhaven_asset_id,
    )
    material = material_factory(material_plan)
    assign_material(export_obj, material)


def register():
    """Register PBR bake/export operators."""
    import bpy

    global ZH2_OT_PolyhavenSearch
    global ZH2_OT_PolyhavenDownload
    global ZH2_OT_BakeExportSelected

    if _ClassStore.classes:
        unregister()

    class ZH2_OT_PolyhavenSearch(bpy.types.Operator):
        bl_idname = "zh2.polyhaven_search"
        bl_label = "Search Poly Haven"

        def execute(self, context):
            settings = context.scene.zh2_pbr_bake_export
            prefs = _addon_preferences(context)

            try:
                client = PolyhavenClient(prefs.user_agent)
                results = client.search_textures(settings.polyhaven_query)
            except PolyhavenError as exc:
                self.report({"ERROR"}, str(exc))
                return {"CANCELLED"}

            if not results:
                self.report({"WARNING"}, "No Poly Haven texture results found")
                return {"CANCELLED"}

            selected = results[0]
            settings.selected_polyhaven_asset = selected.asset_id
            self.report({"INFO"}, f"Selected Poly Haven material: {selected.name}")
            return {"FINISHED"}

    class ZH2_OT_PolyhavenDownload(bpy.types.Operator):
        bl_idname = "zh2.polyhaven_download"
        bl_label = "Download Material"

        def execute(self, context):
            settings = context.scene.zh2_pbr_bake_export
            prefs = _addon_preferences(context)
            asset_id = settings.selected_polyhaven_asset.strip()
            if not asset_id:
                self.report({"ERROR"}, "Select a Poly Haven material before downloading")
                return {"CANCELLED"}

            try:
                resolution = _resolution_from_settings(settings.texture_resolution)
                client = PolyhavenClient(prefs.user_agent)
                files = client.files_for_asset(asset_id)
                selection = select_texture_files(
                    files,
                    resolution,
                    settings.allow_resolution_fallback,
                )
                cache_dir = _abspath(prefs.cache_root) / asset_id
                client.download_selection(selection, cache_dir, asset_id)
            except PolyhavenError as exc:
                self.report({"ERROR"}, str(exc))
                return {"CANCELLED"}

            self.report({"INFO"}, f"Downloaded Poly Haven material: {asset_id}")
            return {"FINISHED"}

    class ZH2_OT_BakeExportSelected(bpy.types.Operator):
        bl_idname = "zh2.bake_export_selected"
        bl_label = "Bake + Export Selected"

        def execute(self, context):
            settings = context.scene.zh2_pbr_bake_export
            selected_objects = _selected_mesh_objects(context)
            if not selected_objects:
                self.report({"ERROR"}, "Select at least one mesh object to bake and export")
                return {"CANCELLED"}

            resolution = _resolution_from_settings(settings.texture_resolution)
            output_root = _abspath(settings.output_root)
            cache_root = _abspath(_addon_preferences(context).cache_root)
            mode = _asset_mode_from_settings(settings.asset_mode)
            material_source = _material_source_from_settings(settings.material_source)
            reserved_names: set[str] = set()

            for source_obj in selected_objects:
                export_obj = None
                try:
                    asset_paths = _build_operator_asset_paths(
                        output_root,
                        source_obj.name,
                        reserved_names,
                        settings.overwrite_existing,
                    )
                    reserved_names.add(asset_paths.asset_name)

                    validate_output_folder(asset_paths.asset_dir, settings.overwrite_existing)
                    asset_paths.texture_dir.mkdir(parents=True, exist_ok=True)

                    export_obj = duplicate_for_export(source_obj, asset_paths.asset_name)

                    autogenerated_low_poly = False
                    if mode is AssetMode.LOW_POLY:
                        if not settings.autogenerate_low_poly:
                            self.report({"ERROR"}, "Low-poly export requires autogenerate low poly")
                            return {"CANCELLED"}
                        apply_low_poly_decimation(export_obj, settings.decimate_ratio)
                        autogenerated_low_poly = True

                    ensure_uv_layer(export_obj)

                    polyhaven_asset_id = ""
                    material_texture_paths: dict[str, Path] = {}
                    if material_source is MaterialSource.POLY_HAVEN:
                        polyhaven_asset_id = settings.selected_polyhaven_asset.strip()
                        if not polyhaven_asset_id:
                            self.report({"ERROR"}, "Select a Poly Haven material before exporting")
                            return {"CANCELLED"}
                        material_texture_paths = _polyhaven_cache_texture_paths(cache_root, polyhaven_asset_id)

                    _apply_material_source(
                        export_obj,
                        material_source,
                        material_name=f"{asset_paths.asset_name}_material",
                        texture_paths=material_texture_paths,
                        polyhaven_asset_id=polyhaven_asset_id,
                    )

                    warnings = bake_object_maps(export_obj, asset_paths.textures, resolution)
                    export_glb(export_obj, asset_paths.glb_path)

                    write_manifest(
                        asset_paths.manifest_path,
                        ExportManifest(
                            asset_name=asset_paths.asset_name,
                            source_object=source_obj.name,
                            exported_at=datetime.now(timezone.utc).isoformat(),
                            mode=mode,
                            autogenerated_low_poly=autogenerated_low_poly,
                            resolution=resolution,
                            material_source=material_source,
                            polyhaven_asset_id=polyhaven_asset_id or None,
                            glb=asset_paths.glb_path.relative_to(asset_paths.asset_dir),
                            textures=_relative_texture_paths(asset_paths.textures, asset_paths.asset_dir),
                            warnings=warnings,
                        ),
                    )
                    self.report({"INFO"}, f"Exported {source_obj.name} to {asset_paths.glb_path}")
                except ExportPreflightError as exc:
                    self.report({"ERROR"}, str(exc))
                    return {"CANCELLED"}
                except Exception as exc:
                    self.report({"ERROR"}, f"Export failed for {source_obj.name}: {exc}")
                    return {"CANCELLED"}
                finally:
                    if export_obj is not None:
                        try:
                            remove_temp_object(export_obj)
                        except Exception as exc:
                            self.report({"WARNING"}, f"Failed to remove temporary export object: {exc}")

            return {"FINISHED"}

    _ClassStore.classes = [
        ZH2_OT_PolyhavenSearch,
        ZH2_OT_PolyhavenDownload,
        ZH2_OT_BakeExportSelected,
    ]

    registered_classes = []
    try:
        for cls in _ClassStore.classes:
            bpy.utils.register_class(cls)
            registered_classes.append(cls)
    except Exception:
        for cls in reversed(registered_classes):
            try:
                bpy.utils.unregister_class(cls)
            except RuntimeError:
                pass
        _ClassStore.classes = []
        raise


def unregister():
    """Unregister PBR bake/export operators."""
    import bpy

    for cls in reversed(_ClassStore.classes):
        try:
            bpy.utils.unregister_class(cls)
        except (RuntimeError, ValueError):
            pass

    _ClassStore.classes = []
    globals().pop("ZH2_OT_BakeExportSelected", None)
    globals().pop("ZH2_OT_PolyhavenDownload", None)
    globals().pop("ZH2_OT_PolyhavenSearch", None)
