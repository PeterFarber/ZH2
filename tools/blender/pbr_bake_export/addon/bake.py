"""Bake map definitions and Blender bake execution helpers."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from .types import TextureResolution


@dataclass(frozen=True)
class BakeMapSpec:
    role: str
    blender_bake_type: str
    color_space: str
    file_extension: str = ".png"


def bake_map_specs() -> list[BakeMapSpec]:
    return [
        BakeMapSpec("basecolor", "DIFFUSE", "sRGB"),
        BakeMapSpec("normal", "NORMAL", "Non-Color"),
        BakeMapSpec("roughness", "ROUGHNESS", "Non-Color"),
        BakeMapSpec("metallic", "EMIT", "Non-Color"),
        BakeMapSpec("ao", "AO", "Non-Color"),
    ]


def _object_materials(obj) -> list[object]:
    material_slots = getattr(obj, "material_slots", None)
    if material_slots is not None:
        materials = [getattr(slot, "material", None) for slot in material_slots]
        if materials:
            return materials

    return [getattr(obj, "active_material", None)]


def _node_materials_for_bake(obj) -> tuple[list[object], bool]:
    node_materials: list[object] = []
    has_missing_node_material = False
    seen_node_trees: set[int] = set()

    for material in _object_materials(obj):
        node_tree = getattr(material, "node_tree", None) if material is not None else None
        if material is None or not getattr(material, "use_nodes", False) or node_tree is None:
            has_missing_node_material = True
            continue

        node_tree_id = id(node_tree)
        if node_tree_id in seen_node_trees:
            continue

        seen_node_trees.add(node_tree_id)
        node_materials.append(material)

    return node_materials, has_missing_node_material


def _attach_bake_image_nodes(materials: list[object], image) -> list[tuple[object, object, object]]:
    targets = []
    try:
        for material in materials:
            nodes = material.node_tree.nodes
            previous_active_node = nodes.active
            texture_node = nodes.new("ShaderNodeTexImage")
            targets.append((nodes, previous_active_node, texture_node))
            texture_node.image = image
            nodes.active = texture_node
    except Exception:
        _restore_bake_image_nodes(targets)
        raise

    return targets


def _restore_bake_image_nodes(targets: list[tuple[object, object, object]]) -> None:
    for nodes, previous_active_node, texture_node in reversed(targets):
        nodes.active = previous_active_node
        nodes.remove(texture_node)


def bake_object_maps(
    obj,
    texture_paths: dict[str, Path],
    resolution: TextureResolution,
    margin: int = 16,
) -> list[str]:
    import bpy

    warnings: list[str] = []
    scene = bpy.context.scene
    view_layer = bpy.context.view_layer
    previous_render_engine = scene.render.engine
    previous_selected_objects = list(bpy.context.selected_objects)
    previous_active_object = view_layer.objects.active
    previous_use_pass_direct = scene.render.bake.use_pass_direct
    previous_use_pass_indirect = scene.render.bake.use_pass_indirect
    previous_use_pass_color = scene.render.bake.use_pass_color
    node_materials, has_missing_node_material = _node_materials_for_bake(obj)

    try:
        scene.render.engine = "CYCLES"
        bpy.ops.object.select_all(action="DESELECT")
        obj.select_set(True)
        view_layer.objects.active = obj

        for spec in bake_map_specs():
            image = None
            targets = []
            image = bpy.data.images.new(
                f"{obj.name}_{spec.role}",
                width=resolution.pixel_size,
                height=resolution.pixel_size,
                alpha=False,
            )
            try:
                image.colorspace_settings.name = spec.color_space

                texture_path = texture_paths[spec.role]
                texture_path.parent.mkdir(parents=True, exist_ok=True)

                if has_missing_node_material or not node_materials:
                    warnings.append(f"{spec.role} baked from object without node material")

                targets = _attach_bake_image_nodes(node_materials, image)

                if spec.blender_bake_type == "DIFFUSE":
                    scene.render.bake.use_pass_direct = False
                    scene.render.bake.use_pass_indirect = False
                    scene.render.bake.use_pass_color = True

                bpy.ops.object.bake(type=spec.blender_bake_type, margin=margin)
                image.filepath_raw = str(texture_path)
                image.file_format = "PNG"
                image.save()
            finally:
                try:
                    _restore_bake_image_nodes(targets)
                finally:
                    if image is not None:
                        bpy.data.images.remove(image)
    finally:
        scene.render.bake.use_pass_direct = previous_use_pass_direct
        scene.render.bake.use_pass_indirect = previous_use_pass_indirect
        scene.render.bake.use_pass_color = previous_use_pass_color
        scene.render.engine = previous_render_engine
        bpy.ops.object.select_all(action="DESELECT")
        for selected_object in previous_selected_objects:
            selected_object.select_set(True)
        view_layer.objects.active = previous_active_object

    return warnings
