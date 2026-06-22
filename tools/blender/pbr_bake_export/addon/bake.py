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

    try:
        scene.render.engine = "CYCLES"
        bpy.ops.object.select_all(action="DESELECT")
        obj.select_set(True)
        view_layer.objects.active = obj

        for spec in bake_map_specs():
            image = bpy.data.images.new(
                f"{obj.name}_{spec.role}",
                width=resolution.pixel_size,
                height=resolution.pixel_size,
                alpha=False,
            )
            image.colorspace_settings.name = spec.color_space

            texture_path = texture_paths[spec.role]
            texture_path.parent.mkdir(parents=True, exist_ok=True)

            material = getattr(obj, "active_material", None)
            if material is not None and material.use_nodes and material.node_tree is not None:
                texture_node = material.node_tree.nodes.new("ShaderNodeTexImage")
                texture_node.image = image
                material.node_tree.nodes.active = texture_node
            else:
                warnings.append(f"{spec.role} baked from object without node material")

            if spec.blender_bake_type == "DIFFUSE":
                scene.render.bake.use_pass_direct = False
                scene.render.bake.use_pass_indirect = False
                scene.render.bake.use_pass_color = True

            bpy.ops.object.bake(type=spec.blender_bake_type, margin=margin)
            image.filepath_raw = str(texture_path)
            image.file_format = "PNG"
            image.save()
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
