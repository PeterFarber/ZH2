import unittest

from tools.blender.pbr_bake_export.addon.bake import bake_map_specs


class BakeTests(unittest.TestCase):
    def test_bake_map_specs_include_required_pbr_outputs(self):
        specs = bake_map_specs()
        self.assertEqual([spec.role for spec in specs], ["basecolor", "normal", "roughness", "metallic", "ao"])
        self.assertEqual(specs[0].blender_bake_type, "DIFFUSE")
        self.assertEqual(specs[0].color_space, "sRGB")
        self.assertEqual(specs[1].blender_bake_type, "NORMAL")
        self.assertEqual(specs[1].color_space, "Non-Color")

    def test_every_bake_map_has_png_filename_suffix(self):
        for spec in bake_map_specs():
            self.assertEqual(spec.file_extension, ".png")


if __name__ == "__main__":
    unittest.main()
