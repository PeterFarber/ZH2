import unittest

from tools.blender.pbr_bake_export.addon.bake import bake_map_specs


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


if __name__ == "__main__":
    unittest.main()
