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
    emit_source_input: str | None = None


@dataclass
class _EmitSourceTarget:
    nodes: object
    links: object
    previous_active_node: object
    original_surface_links: list[tuple[object, object]]
    temporary_nodes: list[object]
    temporary_links: list[object]
    original_links_removed: bool = False


def bake_map_specs() -> list[BakeMapSpec]:
    return [
        BakeMapSpec("basecolor", "DIFFUSE", "sRGB"),
        BakeMapSpec("normal", "NORMAL", "Non-Color"),
        BakeMapSpec("roughness", "ROUGHNESS", "Non-Color"),
        BakeMapSpec("metallic", "EMIT", "Non-Color", emit_source_input="Metallic"),
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


def _node_matches(node, *kinds: str) -> bool:
    node_kinds = {
        getattr(node, "type", None),
        getattr(node, "bl_idname", None),
        getattr(node, "node_type", None),
    }
    return any(kind in node_kinds for kind in kinds)


def _socket_by_name(sockets, name: str):
    if sockets is None:
        return None

    getter = getattr(sockets, "get", None)
    if callable(getter):
        socket = getter(name)
        if socket is not None:
            return socket

    try:
        return sockets[name]
    except (KeyError, IndexError, TypeError):
        return None


def _socket_links(socket) -> list[object]:
    return list(getattr(socket, "links", []) or [])


def _linked_node(socket):
    links = _socket_links(socket)
    if not links:
        return None
    return getattr(links[0].from_socket, "node", None)


def _find_principled_bsdf(nodes, output=None):
    if output is not None:
        surface_input = _socket_by_name(getattr(output, "inputs", None), "Surface")
        surface_node = _linked_node(surface_input)
        if _node_matches(surface_node, "BSDF_PRINCIPLED", "ShaderNodeBsdfPrincipled"):
            return surface_node

    for node in nodes:
        if _node_matches(node, "BSDF_PRINCIPLED", "ShaderNodeBsdfPrincipled"):
            return node

    return None


def _find_material_output(nodes):
    outputs = [
        node
        for node in nodes
        if _node_matches(node, "OUTPUT_MATERIAL", "ShaderNodeOutputMaterial")
    ]
    for output in outputs:
        if getattr(output, "is_active_output", False):
            return output

    return outputs[0] if outputs else None


def _set_grayscale_color_default(socket, value: object) -> None:
    try:
        scalar = float(value)
    except (TypeError, ValueError):
        scalar = 0.0

    socket.default_value = (scalar, scalar, scalar, 1.0)


def _remove_link_if_present(links, link) -> None:
    try:
        links.remove(link)
    except (ReferenceError, RuntimeError, ValueError):
        pass


def _remove_node_if_present(nodes, node) -> None:
    try:
        nodes.remove(node)
    except (ReferenceError, RuntimeError, ValueError):
        pass


def _restore_emit_source_nodes(targets: list[_EmitSourceTarget]) -> None:
    for target in reversed(targets):
        for link in reversed(target.temporary_links):
            _remove_link_if_present(target.links, link)

        for node in reversed(target.temporary_nodes):
            _remove_node_if_present(target.nodes, node)

        if target.original_links_removed:
            for from_socket, to_socket in target.original_surface_links:
                target.links.new(from_socket, to_socket)

        try:
            target.nodes.active = target.previous_active_node
        except (ReferenceError, RuntimeError, AttributeError):
            pass


def _configure_emit_source_nodes(materials: list[object], source_input_name: str) -> list[_EmitSourceTarget]:
    targets: list[_EmitSourceTarget] = []
    try:
        for material in materials:
            node_tree = getattr(material, "node_tree", None)
            nodes = getattr(node_tree, "nodes", None)
            links = getattr(node_tree, "links", None)
            if node_tree is None or nodes is None or links is None:
                continue

            output = _find_material_output(nodes)
            principled = _find_principled_bsdf(nodes, output)
            if principled is None or output is None:
                continue

            source_input = _socket_by_name(getattr(principled, "inputs", None), source_input_name)
            surface_input = _socket_by_name(getattr(output, "inputs", None), "Surface")
            if source_input is None or surface_input is None:
                continue

            target = _EmitSourceTarget(
                nodes=nodes,
                links=links,
                previous_active_node=getattr(nodes, "active", None),
                original_surface_links=[
                    (link.from_socket, link.to_socket)
                    for link in _socket_links(surface_input)
                ],
                temporary_nodes=[],
                temporary_links=[],
            )
            targets.append(target)

            target.original_links_removed = True
            for link in _socket_links(surface_input):
                links.remove(link)

            emission_node = nodes.new("ShaderNodeEmission")
            target.temporary_nodes.append(emission_node)

            emission_color_input = _socket_by_name(getattr(emission_node, "inputs", None), "Color")
            emission_strength_input = _socket_by_name(getattr(emission_node, "inputs", None), "Strength")
            emission_output = _socket_by_name(getattr(emission_node, "outputs", None), "Emission")
            if emission_color_input is None or emission_output is None:
                continue

            if emission_strength_input is not None:
                emission_strength_input.default_value = 1.0

            source_links = _socket_links(source_input)
            if source_links:
                target.temporary_links.append(
                    links.new(source_links[0].from_socket, emission_color_input)
                )
            else:
                _set_grayscale_color_default(
                    emission_color_input,
                    getattr(source_input, "default_value", 0.0),
                )

            target.temporary_links.append(links.new(emission_output, surface_input))
            try:
                nodes.active = target.previous_active_node
            except (ReferenceError, RuntimeError, AttributeError):
                pass
    except Exception:
        _restore_emit_source_nodes(targets)
        raise

    return targets


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
            emit_targets = []
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

                if spec.emit_source_input is not None:
                    emit_targets = _configure_emit_source_nodes(node_materials, spec.emit_source_input)

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
                    try:
                        _restore_emit_source_nodes(emit_targets)
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
