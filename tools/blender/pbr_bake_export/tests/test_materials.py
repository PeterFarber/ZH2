import unittest
from pathlib import Path

from tools.blender.pbr_bake_export.addon.materials import MaterialPlan, build_material_plan
from tools.blender.pbr_bake_export.addon.types import MaterialSource


class MaterialTests(unittest.TestCase):
    def test_build_polyhaven_material_plan_records_texture_paths_and_color_spaces(self):
        plan = build_material_plan(
            name="wall_panel_material",
            source=MaterialSource.POLY_HAVEN,
            texture_paths={
                "basecolor": Path("cache/wall_basecolor.png"),
                "normal": Path("cache/wall_normal.png"),
                "roughness": Path("cache/wall_roughness.png"),
                "metallic": Path("cache/wall_metallic.png"),
                "ao": Path("cache/wall_ao.png"),
            },
            polyhaven_asset_id="wall_plaster",
        )
        self.assertIsInstance(plan, MaterialPlan)
        self.assertEqual(plan.name, "wall_panel_material")
        self.assertEqual(plan.source, MaterialSource.POLY_HAVEN)
        self.assertEqual(plan.texture_color_spaces["basecolor"], "sRGB")
        self.assertEqual(plan.texture_color_spaces["normal"], "Non-Color")
        self.assertEqual(plan.texture_color_spaces["roughness"], "Non-Color")
        self.assertEqual(plan.polyhaven_asset_id, "wall_plaster")

    def test_build_existing_material_plan_allows_empty_texture_paths(self):
        plan = build_material_plan(
            name="existing",
            source=MaterialSource.EXISTING,
            texture_paths={},
            polyhaven_asset_id="",
        )
        self.assertEqual(plan.source, MaterialSource.EXISTING)
        self.assertEqual(plan.texture_paths, {})


if __name__ == "__main__":
    unittest.main()
