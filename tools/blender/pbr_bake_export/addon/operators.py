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
from .polyhaven import (
    POLYHAVEN_NO_SELECTION,
    REQUIRED_ROLES,
    PolyhavenAsset,
    PolyhavenClient,
    PolyhavenError,
    select_texture_files,
    serialize_asset_choices,
)
from .types import AssetMode, AssetPaths, MaterialSource, TextureResolution


DEFAULT_USER_AGENT = "ZH2-Blender-PBR-Bake-Export/0.1"
DEFAULT_CACHE_ROOT = "//assets/_polyhaven_cache"


class _ClassStore:
    classes: list[type] = []


class _DefaultAddonPreferences:
    user_agent = DEFAULT_USER_AGENT
    cache_root = DEFAULT_CACHE_ROOT


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
    addon_name = __package__
    try:
        return context.preferences.addons[addon_name].preferences
    except (AttributeError, KeyError, TypeError):
        return _DefaultAddonPreferences()


def _selected_mesh_objects(context):
    return [obj for obj in context.selected_objects if getattr(obj, "type", None) == "MESH"]


def _safe_select_set(context, obj, selected: bool) -> None:
    try:
        obj.select_set(selected)
    except (ReferenceError, RuntimeError, AttributeError):
        if not selected:
            selected_objects = getattr(context, "selected_objects", None)
            if isinstance(selected_objects, list) and obj in selected_objects:
                selected_objects.remove(obj)


def _restore_object_selection(context, selected_objects: list[object], active_object) -> None:
    for obj in list(getattr(context, "selected_objects", []) or []):
        _safe_select_set(context, obj, False)

    for obj in selected_objects:
        _safe_select_set(context, obj, True)

    try:
        context.view_layer.objects.active = active_object
    except (ReferenceError, RuntimeError, AttributeError, TypeError):
        try:
            context.view_layer.objects.active = None
        except (ReferenceError, RuntimeError, AttributeError, TypeError):
            pass


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
    return {
        role: texture_path
        for role in TEXTURE_ROLES
        if (texture_path := cache_dir / f"{asset_id}_{role}.png").is_file()
    }


def _validated_polyhaven_cache_texture_paths(cache_root: Path, asset_id: str) -> dict[str, Path]:
    texture_paths = _polyhaven_cache_texture_paths(cache_root, asset_id)
    missing_roles = [role for role in REQUIRED_ROLES if role not in texture_paths]
    if missing_roles:
        missing = ", ".join(missing_roles)
        raise ExportPreflightError(f"Downloaded Poly Haven material is missing required texture roles: {missing}")
    return texture_paths


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


def _apply_downloaded_polyhaven_material(
    selected_objects: list[object],
    material_name: str,
    texture_paths: dict[str, Path],
    polyhaven_asset_id: str,
    material_factory=create_blender_material,
    assign_material=_assign_material,
) -> int:
    if not selected_objects:
        raise ExportPreflightError("Select at least one mesh object to apply material")

    material_plan = build_material_plan(
        name=material_name,
        source=MaterialSource.POLY_HAVEN,
        texture_paths=texture_paths,
        polyhaven_asset_id=polyhaven_asset_id,
    )
    material = material_factory(material_plan)
    for selected_object in selected_objects:
        assign_material(selected_object, material)
    return len(selected_objects)


def _clean_polyhaven_selection(asset_id: str) -> str:
    clean_asset_id = asset_id.strip()
    if clean_asset_id == POLYHAVEN_NO_SELECTION:
        return ""
    return clean_asset_id


def _selected_polyhaven_asset_id(settings) -> str:
    return _clean_polyhaven_selection(getattr(settings, "selected_polyhaven_asset", ""))


def _selected_downloaded_polyhaven_material_id(settings) -> str:
    return _clean_polyhaven_selection(getattr(settings, "selected_downloaded_polyhaven_material", ""))


def _store_polyhaven_search_results(settings, results: list[PolyhavenAsset]) -> None:
    settings.polyhaven_search_results = serialize_asset_choices(results)
    if results:
        settings.selected_polyhaven_asset = results[0].asset_id
    else:
        settings.selected_polyhaven_asset = POLYHAVEN_NO_SELECTION


def _validate_export_prerequisites(
    settings,
    mode: AssetMode,
    material_source: MaterialSource,
) -> str:
    if mode is AssetMode.LOW_POLY and not settings.autogenerate_low_poly:
        raise ExportPreflightError("Low-poly export requires autogenerate low poly")

    if material_source is MaterialSource.POLY_HAVEN:
        polyhaven_asset_id = _selected_downloaded_polyhaven_material_id(settings)
        if not polyhaven_asset_id:
            raise ExportPreflightError("Select a downloaded Poly Haven material before exporting")
        return polyhaven_asset_id

    return ""


def _baked_texture_paths_present(asset_paths: AssetPaths) -> dict[str, Path]:
    return {
        role: texture_path
        for role, texture_path in asset_paths.textures.items()
        if texture_path.is_file()
    }


def _build_export_operator_asset_paths(
    output_root: Path,
    object_name: str,
    reserved_names: set[str] | None,
    overwrite_existing: bool,
) -> AssetPaths:
    asset_name = _deduplicate_reserved_asset_name(
        sanitize_asset_name(object_name),
        reserved_names,
    )
    existing_paths = _asset_paths_from_name(output_root, asset_name)
    if existing_paths.asset_dir.exists():
        return existing_paths
    return _build_operator_asset_paths(output_root, object_name, reserved_names, overwrite_existing)


def _prepare_asset_output_folder(
    asset_paths: AssetPaths,
    overwrite_existing: bool,
    bake_textures: bool,
    export_model: bool,
) -> None:
    if bake_textures:
        validate_output_folder(asset_paths.asset_dir, overwrite_existing)
        asset_paths.texture_dir.mkdir(parents=True, exist_ok=True)
        return

    if export_model and asset_paths.glb_path.exists() and not overwrite_existing:
        raise ExportPreflightError(f"asset GLB already exists: {asset_paths.glb_path}")
    asset_paths.asset_dir.mkdir(parents=True, exist_ok=True)


def _execute_selected_asset_workflow(
    context,
    report,
    *,
    bake_textures: bool,
    export_model: bool,
    action_label: str,
):
    original_selected_objects = list(getattr(context, "selected_objects", []) or [])
    original_view_layer_objects = getattr(getattr(context, "view_layer", None), "objects", None)
    original_active_object = getattr(original_view_layer_objects, "active", None)
    try:
        settings = context.scene.zh2_pbr_bake_export
        selected_objects = _selected_mesh_objects(context)
        if not selected_objects:
            report({"ERROR"}, f"Select at least one mesh object to {action_label}")
            return {"CANCELLED"}

        resolution = _resolution_from_settings(settings.texture_resolution)
        output_root = _abspath(settings.output_root)
        mode = _asset_mode_from_settings(settings.asset_mode)
        material_source = _material_source_from_settings(settings.material_source)
        reserved_names: set[str] = set()

        try:
            polyhaven_asset_id = _validate_export_prerequisites(settings, mode, material_source)
            material_texture_paths: dict[str, Path] = {}
            if material_source is MaterialSource.POLY_HAVEN:
                cache_root = _abspath(_addon_preferences(context).cache_root)
                material_texture_paths = _validated_polyhaven_cache_texture_paths(cache_root, polyhaven_asset_id)
        except ExportPreflightError as exc:
            report({"ERROR"}, str(exc))
            return {"CANCELLED"}

        for source_obj in selected_objects:
            export_obj = None
            try:
                if export_model and not bake_textures:
                    asset_paths = _build_export_operator_asset_paths(
                        output_root,
                        source_obj.name,
                        reserved_names,
                        settings.overwrite_existing,
                    )
                else:
                    asset_paths = _build_operator_asset_paths(
                        output_root,
                        source_obj.name,
                        reserved_names,
                        settings.overwrite_existing,
                    )
                reserved_names.add(asset_paths.asset_name)

                _prepare_asset_output_folder(
                    asset_paths,
                    settings.overwrite_existing,
                    bake_textures,
                    export_model,
                )

                export_obj = duplicate_for_export(source_obj, asset_paths.asset_name)

                autogenerated_low_poly = False
                if mode is AssetMode.LOW_POLY:
                    apply_low_poly_decimation(export_obj, settings.decimate_ratio)
                    autogenerated_low_poly = True

                ensure_uv_layer(export_obj)

                _apply_material_source(
                    export_obj,
                    material_source,
                    material_name=f"{asset_paths.asset_name}_material",
                    texture_paths=material_texture_paths,
                    polyhaven_asset_id=polyhaven_asset_id,
                )

                warnings: list[str] = []
                if bake_textures:
                    warnings = bake_object_maps(export_obj, asset_paths.textures, resolution)
                if export_model:
                    export_glb(export_obj, asset_paths.glb_path)

                manifest_textures = asset_paths.textures if bake_textures else _baked_texture_paths_present(asset_paths)
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
                        textures=_relative_texture_paths(manifest_textures, asset_paths.asset_dir),
                        warnings=warnings,
                    ),
                )
                if bake_textures and export_model:
                    report({"INFO"}, f"Exported {source_obj.name} to {asset_paths.glb_path}")
                elif bake_textures:
                    report({"INFO"}, f"Baked {source_obj.name} to {asset_paths.texture_dir}")
                else:
                    report({"INFO"}, f"Exported {source_obj.name} to {asset_paths.glb_path}")
            except ExportPreflightError as exc:
                report({"ERROR"}, str(exc))
                return {"CANCELLED"}
            except Exception as exc:
                report({"ERROR"}, f"{action_label.capitalize()} failed for {source_obj.name}: {exc}")
                return {"CANCELLED"}
            finally:
                if export_obj is not None:
                    try:
                        remove_temp_object(export_obj)
                    except Exception as exc:
                        report({"WARNING"}, f"Failed to remove temporary export object: {exc}")

        return {"FINISHED"}
    finally:
        _restore_object_selection(context, original_selected_objects, original_active_object)


def register():
    """Register PBR bake/export operators."""
    import bpy

    global ZH2_OT_PolyhavenSearch
    global ZH2_OT_PolyhavenDownload
    global ZH2_OT_PolyhavenApplyMaterial
    global ZH2_OT_BakeSelected
    global ZH2_OT_ExportSelected

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
                _store_polyhaven_search_results(settings, [])
                self.report({"WARNING"}, "No Poly Haven texture results found")
                return {"CANCELLED"}

            _store_polyhaven_search_results(settings, results)
            selected = results[0]
            self.report({"INFO"}, f"Found {len(results)} Poly Haven materials; selected {selected.name}")
            return {"FINISHED"}

    class ZH2_OT_PolyhavenDownload(bpy.types.Operator):
        bl_idname = "zh2.polyhaven_download"
        bl_label = "Download Material"

        def execute(self, context):
            settings = context.scene.zh2_pbr_bake_export
            prefs = _addon_preferences(context)
            asset_id = _selected_polyhaven_asset_id(settings)
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

            try:
                settings.selected_downloaded_polyhaven_material = asset_id
            except (AttributeError, TypeError, ValueError):
                pass
            self.report({"INFO"}, f"Downloaded Poly Haven material: {asset_id}")
            return {"FINISHED"}

    class ZH2_OT_PolyhavenApplyMaterial(bpy.types.Operator):
        bl_idname = "zh2.polyhaven_apply_material"
        bl_label = "Apply Material to Selected"

        def execute(self, context):
            settings = context.scene.zh2_pbr_bake_export
            asset_id = _selected_downloaded_polyhaven_material_id(settings)
            if not asset_id:
                self.report({"ERROR"}, "Select a downloaded Poly Haven material before applying")
                return {"CANCELLED"}

            try:
                selected_objects = _selected_mesh_objects(context)
                cache_root = _abspath(_addon_preferences(context).cache_root)
                texture_paths = _validated_polyhaven_cache_texture_paths(cache_root, asset_id)
                assigned_count = _apply_downloaded_polyhaven_material(
                    selected_objects,
                    material_name=f"{asset_id}_material",
                    texture_paths=texture_paths,
                    polyhaven_asset_id=asset_id,
                )
            except ExportPreflightError as exc:
                self.report({"ERROR"}, str(exc))
                return {"CANCELLED"}
            except Exception as exc:
                self.report({"ERROR"}, f"Failed to apply Poly Haven material {asset_id}: {exc}")
                return {"CANCELLED"}

            self.report({"INFO"}, f"Applied Poly Haven material {asset_id} to {assigned_count} object(s)")
            return {"FINISHED"}

    class ZH2_OT_BakeSelected(bpy.types.Operator):
        bl_idname = "zh2.bake_selected"
        bl_label = "Bake Selected"

        def execute(self, context):
            return _execute_selected_asset_workflow(
                context,
                self.report,
                bake_textures=True,
                export_model=False,
                action_label="bake",
            )

    class ZH2_OT_ExportSelected(bpy.types.Operator):
        bl_idname = "zh2.export_selected"
        bl_label = "Export Selected"

        def execute(self, context):
            return _execute_selected_asset_workflow(
                context,
                self.report,
                bake_textures=False,
                export_model=True,
                action_label="export",
            )

    _ClassStore.classes = [
        ZH2_OT_PolyhavenSearch,
        ZH2_OT_PolyhavenDownload,
        ZH2_OT_PolyhavenApplyMaterial,
        ZH2_OT_BakeSelected,
        ZH2_OT_ExportSelected,
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
    globals().pop("ZH2_OT_ExportSelected", None)
    globals().pop("ZH2_OT_BakeSelected", None)
    globals().pop("ZH2_OT_PolyhavenApplyMaterial", None)
    globals().pop("ZH2_OT_PolyhavenDownload", None)
    globals().pop("ZH2_OT_PolyhavenSearch", None)
