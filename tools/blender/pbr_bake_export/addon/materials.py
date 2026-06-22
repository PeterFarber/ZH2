"""Material planning and Blender material construction helpers."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from .types import MaterialSource


TEXTURE_COLOR_SPACES = {
    "basecolor": "sRGB",
    "normal": "Non-Color",
    "roughness": "Non-Color",
    "metallic": "Non-Color",
    "ao": "Non-Color",
}


@dataclass(frozen=True)
class MaterialPlan:
    name: str
    source: MaterialSource
    texture_paths: dict[str, Path]
    texture_color_spaces: dict[str, str]
    polyhaven_asset_id: str


def build_material_plan(
    name: str,
    source: MaterialSource,
    texture_paths: dict[str, Path],
    polyhaven_asset_id: str,
) -> MaterialPlan:
    return MaterialPlan(
        name=name,
        source=source,
        texture_paths=dict(texture_paths),
        texture_color_spaces=dict(TEXTURE_COLOR_SPACES),
        polyhaven_asset_id=polyhaven_asset_id,
    )


def create_blender_material(plan: MaterialPlan):
    import bpy

    material = bpy.data.materials.new(plan.name)
    material.use_nodes = True

    node_tree = material.node_tree
    nodes = node_tree.nodes
    links = node_tree.links

    principled = _find_or_create_node(nodes, "BSDF_PRINCIPLED", "ShaderNodeBsdfPrincipled")
    output = _find_or_create_node(nodes, "OUTPUT_MATERIAL", "ShaderNodeOutputMaterial")
    _link_if_available(links, principled.outputs, "BSDF", output.inputs, "Surface")

    texture_nodes = {}
    for role, texture_path in plan.texture_paths.items():
        image = bpy.data.images.load(str(texture_path), check_existing=True)
        color_space = plan.texture_color_spaces.get(role)
        if color_space:
            image.colorspace_settings.name = color_space

        texture_node = nodes.new("ShaderNodeTexImage")
        texture_node.label = role
        texture_node.image = image
        texture_nodes[role] = texture_node

    if "basecolor" in texture_nodes:
        _link_if_available(links, texture_nodes["basecolor"].outputs, "Color", principled.inputs, "Base Color")
    if "roughness" in texture_nodes:
        _link_if_available(links, texture_nodes["roughness"].outputs, "Color", principled.inputs, "Roughness")
    if "metallic" in texture_nodes:
        _link_if_available(links, texture_nodes["metallic"].outputs, "Color", principled.inputs, "Metallic")
    if "normal" in texture_nodes:
        normal_map = nodes.new("ShaderNodeNormalMap")
        _link_if_available(links, texture_nodes["normal"].outputs, "Color", normal_map.inputs, "Color")
        _link_if_available(links, normal_map.outputs, "Normal", principled.inputs, "Normal")

    return material


def _find_or_create_node(nodes, node_type: str, node_id: str):
    for node in nodes:
        if node.type == node_type:
            return node
    return nodes.new(node_id)


def _link_if_available(links, from_sockets, from_name: str, to_sockets, to_name: str) -> None:
    from_socket = from_sockets.get(from_name)
    to_socket = to_sockets.get(to_name)
    if from_socket is not None and to_socket is not None:
        links.new(from_socket, to_socket)
