import sys
import tempfile
import types
import unittest
from pathlib import Path

from tools.blender.pbr_bake_export.addon.bake import bake_map_specs, bake_object_maps
from tools.blender.pbr_bake_export.addon.types import TextureResolution


class FakeImage:
    def __init__(self, name, width, height, alpha):
        self.name = name
        self.width = width
        self.height = height
        self.alpha = alpha
        self.colorspace_settings = types.SimpleNamespace(name="")
        self.filepath_raw = ""
        self.file_format = ""
        self.saved = False

    def save(self):
        self.saved = True


class FakeImages:
    def __init__(self):
        self.active_images = []
        self.saved_names = []
        self.removed_names = []

    def new(self, name, width, height, alpha):
        image = FakeImage(name, width, height, alpha)
        self.active_images.append(image)
        return image

    def remove(self, image):
        if image.saved:
            self.saved_names.append(image.name)
        self.active_images.remove(image)
        self.removed_names.append(image.name)


class FakeNode:
    def __init__(self, node_type):
        self.node_type = node_type
        self.image = None


class FakeNodes:
    def __init__(self):
        self._nodes = []
        self.active = None

    def __iter__(self):
        return iter(self._nodes)

    def new(self, node_type):
        node = FakeNode(node_type)
        self._nodes.append(node)
        return node

    def remove(self, node):
        self._nodes.remove(node)
        if self.active is node:
            self.active = None

    def as_list(self):
        return list(self._nodes)


class FakeMaterial:
    def __init__(self, name, use_nodes=True, has_node_tree=True):
        self.name = name
        self.use_nodes = use_nodes
        self.node_tree = types.SimpleNamespace(nodes=FakeNodes()) if has_node_tree else None
        if self.node_tree is not None:
            self.previous_active = FakeNode("PreviousActive")
            self.node_tree.nodes._nodes.append(self.previous_active)
            self.node_tree.nodes.active = self.previous_active


class FakeObject:
    def __init__(self, name, materials, context):
        self.name = name
        self.material_slots = [types.SimpleNamespace(material=material) for material in materials]
        self.active_material = materials[0] if materials else None
        self._context = context

    def select_set(self, selected):
        if selected and self not in self._context.selected_objects:
            self._context.selected_objects.append(self)
        elif not selected and self in self._context.selected_objects:
            self._context.selected_objects.remove(self)


class FakeObjectOps:
    def __init__(self, context):
        self._context = context
        self.bake_calls = []

    def select_all(self, action):
        if action == "DESELECT":
            for obj in list(self._context.selected_objects):
                obj.select_set(False)

    def bake(self, type, margin):
        active_object = self._context.view_layer.objects.active
        targets = {}
        for slot in active_object.material_slots:
            material = slot.material
            if material is None or not material.use_nodes or material.node_tree is None:
                continue

            active_node = material.node_tree.nodes.active
            targets[material.name] = active_node.image.name if active_node and active_node.image else None

        self.bake_calls.append({"type": type, "margin": margin, "targets": targets})


class FakeBpy(types.SimpleNamespace):
    def __init__(self):
        bake_settings = types.SimpleNamespace(
            use_pass_direct=True,
            use_pass_indirect=True,
            use_pass_color=False,
        )
        context = types.SimpleNamespace(
            scene=types.SimpleNamespace(
                render=types.SimpleNamespace(engine="BLENDER_EEVEE_NEXT", bake=bake_settings)
            ),
            view_layer=types.SimpleNamespace(objects=types.SimpleNamespace(active=None)),
            selected_objects=[],
        )
        super().__init__(
            context=context,
            data=types.SimpleNamespace(images=FakeImages()),
            ops=types.SimpleNamespace(object=FakeObjectOps(context)),
        )


class FakeBpyModuleTestCase(unittest.TestCase):
    def setUp(self):
        self._previous_bpy = sys.modules.get("bpy")
        self.bpy = FakeBpy()
        sys.modules["bpy"] = self.bpy

    def tearDown(self):
        if self._previous_bpy is None:
            sys.modules.pop("bpy", None)
        else:
            sys.modules["bpy"] = self._previous_bpy


class BakeTests(unittest.TestCase):
    def test_bake_map_specs_include_required_pbr_outputs(self):
        specs = bake_map_specs()
        self.assertEqual(
            [(spec.role, spec.blender_bake_type, spec.color_space) for spec in specs],
            [
                ("basecolor", "DIFFUSE", "sRGB"),
                ("normal", "NORMAL", "Non-Color"),
                ("roughness", "ROUGHNESS", "Non-Color"),
                ("metallic", "EMIT", "Non-Color"),
                ("ao", "AO", "Non-Color"),
            ],
        )

    def test_every_bake_map_has_png_filename_suffix(self):
        for spec in bake_map_specs():
            self.assertEqual(spec.file_extension, ".png")


class BakeObjectMapsTests(FakeBpyModuleTestCase):
    def texture_paths(self, temp_dir):
        return {
            spec.role: Path(temp_dir) / f"{spec.role}{spec.file_extension}"
            for spec in bake_map_specs()
        }

    def test_bake_object_maps_targets_all_node_material_slots_and_cleans_up_each_map(self):
        first_material = FakeMaterial("first")
        second_material = FakeMaterial("second")
        obj = FakeObject("test_asset", [first_material, second_material], self.bpy.context)

        with tempfile.TemporaryDirectory() as temp_dir:
            warnings = bake_object_maps(obj, self.texture_paths(temp_dir), TextureResolution.TEST)

        self.assertEqual(warnings, [])
        self.assertEqual(len(self.bpy.ops.object.bake_calls), len(bake_map_specs()))
        for spec, bake_call in zip(bake_map_specs(), self.bpy.ops.object.bake_calls):
            self.assertEqual(
                bake_call["targets"],
                {"first": f"test_asset_{spec.role}", "second": f"test_asset_{spec.role}"},
            )

        for material in [first_material, second_material]:
            self.assertIs(material.node_tree.nodes.active, material.previous_active)
            self.assertEqual(material.node_tree.nodes.as_list(), [material.previous_active])

        self.assertEqual(self.bpy.data.images.active_images, [])
        self.assertEqual(
            self.bpy.data.images.saved_names,
            [f"test_asset_{spec.role}" for spec in bake_map_specs()],
        )
        self.assertEqual(
            self.bpy.data.images.removed_names,
            [f"test_asset_{spec.role}" for spec in bake_map_specs()],
        )

    def test_bake_object_maps_warns_for_missing_slots_but_targets_available_node_materials(self):
        node_material = FakeMaterial("node_material")
        non_node_material = FakeMaterial("non_node_material", use_nodes=False)
        missing_tree_material = FakeMaterial("missing_tree_material", has_node_tree=False)
        obj = FakeObject(
            "mixed_asset",
            [node_material, None, non_node_material, missing_tree_material],
            self.bpy.context,
        )

        with tempfile.TemporaryDirectory() as temp_dir:
            warnings = bake_object_maps(obj, self.texture_paths(temp_dir), TextureResolution.TEST)

        self.assertEqual(
            warnings,
            [f"{spec.role} baked from object without node material" for spec in bake_map_specs()],
        )
        for spec, bake_call in zip(bake_map_specs(), self.bpy.ops.object.bake_calls):
            self.assertEqual(bake_call["targets"], {"node_material": f"mixed_asset_{spec.role}"})

        self.assertIs(node_material.node_tree.nodes.active, node_material.previous_active)
        self.assertEqual(node_material.node_tree.nodes.as_list(), [node_material.previous_active])
        self.assertEqual(self.bpy.data.images.active_images, [])


if __name__ == "__main__":
    unittest.main()
