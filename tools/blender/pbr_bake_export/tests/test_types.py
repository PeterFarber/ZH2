import unittest

from tools.blender.pbr_bake_export.addon.types import AssetMode, MaterialSource, TextureResolution


class TypesTests(unittest.TestCase):
    def test_texture_resolution_pixel_sizes(self):
        self.assertEqual(TextureResolution.FOUR_K.pixel_size, 4096)
        self.assertEqual(TextureResolution.EIGHT_K.pixel_size, 8192)
        self.assertEqual(TextureResolution.TEST.pixel_size, 128)

    def test_texture_resolution_labels_match_ui_labels(self):
        self.assertEqual(TextureResolution.FOUR_K.label, "4K")
        self.assertEqual(TextureResolution.EIGHT_K.label, "8K")
        self.assertEqual(TextureResolution.TEST.label, "Test 128")

    def test_asset_mode_values_match_ui_labels(self):
        self.assertEqual(AssetMode.HIGH_POLY.value, "High Poly")
        self.assertEqual(AssetMode.LOW_POLY.value, "Low Poly")

    def test_material_source_values_match_ui_labels(self):
        self.assertEqual(MaterialSource.EXISTING.value, "Existing Material")


if __name__ == "__main__":
    unittest.main()
