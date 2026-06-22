import unittest

from tools.blender.pbr_bake_export.addon.types import AssetMode, TextureResolution


class TypesTests(unittest.TestCase):
    def test_texture_resolution_pixel_sizes(self):
        self.assertEqual(TextureResolution.FOUR_K.pixel_size, 4096)
        self.assertEqual(TextureResolution.EIGHT_K.pixel_size, 8192)

    def test_asset_mode_values_match_ui_labels(self):
        self.assertEqual(AssetMode.HIGH_POLY.value, "High Poly")
        self.assertEqual(AssetMode.LOW_POLY.value, "Low Poly")


if __name__ == "__main__":
    unittest.main()
