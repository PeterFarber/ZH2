import tempfile
import types
import unittest
from pathlib import Path

from tools.blender.pbr_bake_export.addon import operators
from tools.blender.pbr_bake_export.addon.export import ExportPreflightError
from tools.blender.pbr_bake_export.addon.polyhaven import POLYHAVEN_NO_SELECTION, PolyhavenAsset, polyhaven_asset_enum_items
from tools.blender.pbr_bake_export.addon.types import AssetMode, MaterialSource


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


class OperatorPolyhavenCacheTests(unittest.TestCase):
    def test_polyhaven_cache_texture_paths_include_only_downloaded_files(self):
        with tempfile.TemporaryDirectory() as temp:
            cache_root = Path(temp)
            cache_dir = cache_root / "dirt"
            cache_dir.mkdir()
            (cache_dir / "dirt_basecolor.png").write_bytes(b"base")
            (cache_dir / "dirt_normal.png").write_bytes(b"normal")
            (cache_dir / "dirt_roughness.png").write_bytes(b"rough")
            (cache_dir / "dirt_ao.png").write_bytes(b"ao")

            texture_paths = operators._polyhaven_cache_texture_paths(cache_root, "dirt")

        self.assertEqual(set(texture_paths), {"basecolor", "normal", "roughness", "ao"})
        self.assertNotIn("metallic", texture_paths)

    def test_validate_polyhaven_cache_material_requires_downloaded_core_textures(self):
        with tempfile.TemporaryDirectory() as temp:
            cache_root = Path(temp)
            cache_dir = cache_root / "dirt"
            cache_dir.mkdir()
            (cache_dir / "dirt_basecolor.png").write_bytes(b"base")

            with self.assertRaisesRegex(ExportPreflightError, "Downloaded Poly Haven material is missing"):
                operators._validated_polyhaven_cache_texture_paths(cache_root, "dirt")

    def test_validate_polyhaven_cache_material_allows_missing_metallic(self):
        with tempfile.TemporaryDirectory() as temp:
            cache_root = Path(temp)
            cache_dir = cache_root / "dirt"
            cache_dir.mkdir()
            for role in ("basecolor", "normal", "roughness", "ao"):
                (cache_dir / f"dirt_{role}.png").write_bytes(role.encode("ascii"))

            texture_paths = operators._validated_polyhaven_cache_texture_paths(cache_root, "dirt")

        self.assertEqual(set(texture_paths), {"basecolor", "normal", "roughness", "ao"})


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

    def test_downloaded_polyhaven_material_applies_one_generated_material_to_all_selected_meshes(self):
        first = _FakeObject(["source_one"])
        second = _FakeObject(["source_two"])
        generated_material = object()
        texture_paths = {"basecolor": Path("cache/ice_floor_basecolor.png")}
        calls = {"assigned": []}

        def material_factory(plan):
            calls["plan"] = plan
            return generated_material

        def assign_material(obj, material):
            calls["assigned"].append((obj, material))

        assigned_count = operators._apply_downloaded_polyhaven_material(
            [first, second],
            material_name="ice_floor_material",
            texture_paths=texture_paths,
            polyhaven_asset_id="ice_floor",
            material_factory=material_factory,
            assign_material=assign_material,
        )

        self.assertEqual(assigned_count, 2)
        self.assertEqual(calls["plan"].name, "ice_floor_material")
        self.assertEqual(calls["plan"].source, MaterialSource.POLY_HAVEN)
        self.assertEqual(calls["plan"].texture_paths, texture_paths)
        self.assertEqual(calls["plan"].polyhaven_asset_id, "ice_floor")
        self.assertEqual(calls["assigned"], [(first, generated_material), (second, generated_material)])

    def test_downloaded_polyhaven_material_requires_selected_meshes(self):
        with self.assertRaisesRegex(ExportPreflightError, "Select at least one mesh object"):
            operators._apply_downloaded_polyhaven_material(
                [],
                material_name="ice_floor_material",
                texture_paths={"basecolor": Path("cache/ice_floor_basecolor.png")},
                polyhaven_asset_id="ice_floor",
                material_factory=lambda _plan: object(),
                assign_material=lambda _obj, _material: None,
            )


class OperatorPreferencesTests(unittest.TestCase):
    def test_addon_preferences_falls_back_for_direct_repo_registration(self):
        context = types.SimpleNamespace(
            preferences=types.SimpleNamespace(addons={}),
        )

        prefs = operators._addon_preferences(context)

        self.assertEqual(prefs.user_agent, "ZH2-Blender-PBR-Bake-Export/0.1")
        self.assertEqual(prefs.cache_root, "//assets/_polyhaven_cache")


class OperatorPolyhavenSearchStateTests(unittest.TestCase):
    def _store_polyhaven_search_results(self, *args, **kwargs):
        self.assertTrue(
            hasattr(operators, "_store_polyhaven_search_results"),
            "operators should expose Poly Haven search result state storage",
        )
        return operators._store_polyhaven_search_results(*args, **kwargs)

    def test_store_polyhaven_search_results_populates_dropdown_and_selects_first_result(self):
        settings = types.SimpleNamespace(
            polyhaven_search_results="",
            selected_polyhaven_asset=POLYHAVEN_NO_SELECTION,
        )

        self._store_polyhaven_search_results(
            settings,
            [
                PolyhavenAsset(asset_id="red_brick_floor", name="Red Brick Floor"),
                PolyhavenAsset(asset_id="ice_floor", name="Ice Floor"),
            ],
        )

        self.assertEqual(settings.selected_polyhaven_asset, "red_brick_floor")
        items = polyhaven_asset_enum_items(settings.polyhaven_search_results)
        self.assertEqual(items[0][1], "Red Brick Floor (red_brick_floor)")
        self.assertEqual(items[1][1], "Ice Floor (ice_floor)")

    def test_store_polyhaven_search_results_clears_selection_when_results_are_empty(self):
        settings = types.SimpleNamespace(
            polyhaven_search_results="old",
            selected_polyhaven_asset="old_asset",
        )

        self._store_polyhaven_search_results(settings, [])

        self.assertEqual(settings.polyhaven_search_results, "")
        self.assertEqual(settings.selected_polyhaven_asset, POLYHAVEN_NO_SELECTION)


class OperatorPreflightTests(unittest.TestCase):
    def _validate_export_prerequisites(self, *args, **kwargs):
        self.assertTrue(
            hasattr(operators, "_validate_export_prerequisites"),
            "operators should expose export prerequisite validation before output side effects",
        )
        return operators._validate_export_prerequisites(*args, **kwargs)

    def test_low_poly_export_requires_autogenerate_before_output_side_effects(self):
        settings = types.SimpleNamespace(
            autogenerate_low_poly=False,
            selected_polyhaven_asset="",
        )

        with self.assertRaisesRegex(ExportPreflightError, "Low-poly export requires autogenerate low poly"):
            self._validate_export_prerequisites(
                settings,
                mode=AssetMode.LOW_POLY,
                material_source=MaterialSource.EXISTING,
            )

    def test_polyhaven_export_requires_selected_asset_before_output_side_effects(self):
        settings = types.SimpleNamespace(
            autogenerate_low_poly=True,
            selected_downloaded_polyhaven_material=" ",
        )

        with self.assertRaisesRegex(ExportPreflightError, "Select a downloaded Poly Haven material before exporting"):
            self._validate_export_prerequisites(
                settings,
                mode=AssetMode.HIGH_POLY,
                material_source=MaterialSource.POLY_HAVEN,
            )

    def test_polyhaven_export_treats_dropdown_placeholder_as_missing_selection(self):
        settings = types.SimpleNamespace(
            autogenerate_low_poly=True,
            selected_downloaded_polyhaven_material=POLYHAVEN_NO_SELECTION,
        )

        with self.assertRaisesRegex(ExportPreflightError, "Select a downloaded Poly Haven material before exporting"):
            self._validate_export_prerequisites(
                settings,
                mode=AssetMode.HIGH_POLY,
                material_source=MaterialSource.POLY_HAVEN,
            )

    def test_valid_prerequisites_return_stripped_polyhaven_asset_id(self):
        settings = types.SimpleNamespace(
            autogenerate_low_poly=True,
            selected_downloaded_polyhaven_material="  wall_plaster  ",
        )

        asset_id = self._validate_export_prerequisites(
            settings,
            mode=AssetMode.LOW_POLY,
            material_source=MaterialSource.POLY_HAVEN,
        )

        self.assertEqual(asset_id, "wall_plaster")


class _FakeSelectableObject:
    def __init__(self, name, context):
        self.name = name
        self._context = context
        self.removed = False

    def select_set(self, selected):
        if self.removed:
            raise ReferenceError(f"{self.name} has been removed")
        if selected and self not in self._context.selected_objects:
            self._context.selected_objects.append(self)
        elif not selected and self in self._context.selected_objects:
            self._context.selected_objects.remove(self)


class _FakeActiveObjects:
    def __init__(self):
        self._active = None

    @property
    def active(self):
        return self._active

    @active.setter
    def active(self, value):
        if getattr(value, "removed", False):
            raise ReferenceError(f"{value.name} has been removed")
        self._active = value


class OperatorSelectionRestoreTests(unittest.TestCase):
    def _restore_object_selection(self, *args, **kwargs):
        self.assertTrue(
            hasattr(operators, "_restore_object_selection"),
            "operators should expose object selection restoration",
        )
        return operators._restore_object_selection(*args, **kwargs)

    def test_restore_object_selection_reinstates_original_selection_and_active_object(self):
        context = types.SimpleNamespace(
            selected_objects=[],
            view_layer=types.SimpleNamespace(objects=_FakeActiveObjects()),
        )
        first = _FakeSelectableObject("first", context)
        second = _FakeSelectableObject("second", context)
        temporary = _FakeSelectableObject("temporary", context)
        first.select_set(True)
        second.select_set(True)
        context.view_layer.objects.active = second
        original_selection = list(context.selected_objects)
        original_active = context.view_layer.objects.active

        first.select_set(False)
        temporary.select_set(True)
        context.view_layer.objects.active = temporary

        self._restore_object_selection(context, original_selection, original_active)

        self.assertEqual(context.selected_objects, [first, second])
        self.assertIs(context.view_layer.objects.active, second)

    def test_restore_object_selection_ignores_removed_objects(self):
        context = types.SimpleNamespace(
            selected_objects=[],
            view_layer=types.SimpleNamespace(objects=_FakeActiveObjects()),
        )
        kept = _FakeSelectableObject("kept", context)
        removed = _FakeSelectableObject("removed", context)
        temporary = _FakeSelectableObject("temporary", context)
        kept.select_set(True)
        removed.select_set(True)
        original_selection = list(context.selected_objects)
        removed.removed = True
        temporary.select_set(True)
        context.view_layer.objects.active = temporary

        self._restore_object_selection(context, original_selection, removed)

        self.assertEqual(context.selected_objects, [kept])


if __name__ == "__main__":
    unittest.main()
