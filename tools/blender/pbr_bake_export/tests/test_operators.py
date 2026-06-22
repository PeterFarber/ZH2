import tempfile
import types
import unittest
from pathlib import Path

from tools.blender.pbr_bake_export.addon import operators
from tools.blender.pbr_bake_export.addon.types import MaterialSource


class OperatorPathTests(unittest.TestCase):
    def _build_paths(self, output_root, object_name, reserved_names, overwrite_existing):
        self.assertTrue(
            hasattr(operators, "_build_operator_asset_paths"),
            "operators should expose overwrite-aware asset path planning",
        )
        return operators._build_operator_asset_paths(
            output_root,
            object_name,
            reserved_names,
            overwrite_existing=overwrite_existing,
        )

    def test_overwrite_existing_targets_unsuffixed_existing_output_folder(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "wall_panel").mkdir()

            paths = self._build_paths(root, "Wall Panel", set(), overwrite_existing=True)

            self.assertEqual(paths.asset_name, "wall_panel")
            self.assertEqual(paths.asset_dir, root / "wall_panel")
            self.assertEqual(paths.glb_path, root / "wall_panel" / "wall_panel.glb")

    def test_without_overwrite_existing_output_folder_gets_suffix(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "wall_panel").mkdir()

            paths = self._build_paths(root, "Wall Panel", set(), overwrite_existing=False)

            self.assertEqual(paths.asset_name, "wall_panel_2")
            self.assertEqual(paths.asset_dir, root / "wall_panel_2")

    def test_overwrite_existing_still_avoids_reserved_names_from_same_run(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)

            paths = self._build_paths(root, "Wall Panel", {"wall_panel"}, overwrite_existing=True)

            self.assertEqual(paths.asset_name, "wall_panel_2")


class _FakeData:
    def __init__(self, materials):
        self.materials = materials


class _FakeObject:
    def __init__(self, materials):
        self.data = _FakeData(materials)
        self.active_material = None


class OperatorMaterialTests(unittest.TestCase):
    def _apply_material_source(self, *args, **kwargs):
        self.assertTrue(
            hasattr(operators, "_apply_material_source"),
            "operators should expose material-source assignment behavior",
        )
        return operators._apply_material_source(*args, **kwargs)

    def test_existing_material_source_leaves_copied_material_slots_unchanged(self):
        export_obj = _FakeObject(["source_material"])

        def fail_material_factory(_plan):
            raise AssertionError("existing material should not create a generated material")

        def fail_assign_material(_obj, _material):
            raise AssertionError("existing material should not replace material slots")

        self._apply_material_source(
            export_obj,
            MaterialSource.EXISTING,
            material_name="wall_panel_material",
            texture_paths={},
            polyhaven_asset_id="",
            material_factory=fail_material_factory,
            assign_material=fail_assign_material,
        )

        self.assertEqual(export_obj.data.materials, ["source_material"])

    def test_polyhaven_material_source_creates_and_assigns_generated_material(self):
        export_obj = _FakeObject(["source_material"])
        texture_paths = {"basecolor": Path("cache/wall_basecolor.png")}
        generated_material = object()
        calls = {}

        def material_factory(plan):
            calls["plan"] = plan
            return generated_material

        def assign_material(obj, material):
            calls["assigned_obj"] = obj
            calls["assigned_material"] = material

        self._apply_material_source(
            export_obj,
            MaterialSource.POLY_HAVEN,
            material_name="wall_panel_material",
            texture_paths=texture_paths,
            polyhaven_asset_id="wall_plaster",
            material_factory=material_factory,
            assign_material=assign_material,
        )

        self.assertEqual(calls["plan"].name, "wall_panel_material")
        self.assertEqual(calls["plan"].source, MaterialSource.POLY_HAVEN)
        self.assertEqual(calls["plan"].texture_paths, texture_paths)
        self.assertEqual(calls["plan"].polyhaven_asset_id, "wall_plaster")
        self.assertIs(calls["assigned_obj"], export_obj)
        self.assertIs(calls["assigned_material"], generated_material)


class OperatorPreferencesTests(unittest.TestCase):
    def test_addon_preferences_falls_back_for_direct_repo_registration(self):
        context = types.SimpleNamespace(
            preferences=types.SimpleNamespace(addons={}),
        )

        prefs = operators._addon_preferences(context)

        self.assertEqual(prefs.user_agent, "ZH2-Blender-PBR-Bake-Export/0.1")
        self.assertEqual(prefs.cache_root, "//assets/_polyhaven_cache")


if __name__ == "__main__":
    unittest.main()
