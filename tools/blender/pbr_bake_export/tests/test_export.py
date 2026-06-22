import tempfile
import unittest
from pathlib import Path

from tools.blender.pbr_bake_export.addon.export import ExportPreflightError, validate_output_folder


class ExportTests(unittest.TestCase):
    def test_validate_output_folder_blocks_existing_folder_when_overwrite_disabled(self):
        with tempfile.TemporaryDirectory() as temp:
            asset_dir = Path(temp) / "wall_panel"
            asset_dir.mkdir()

            with self.assertRaises(ExportPreflightError):
                validate_output_folder(asset_dir, overwrite=False)

    def test_validate_output_folder_allows_existing_folder_when_overwrite_enabled(self):
        with tempfile.TemporaryDirectory() as temp:
            asset_dir = Path(temp) / "wall_panel"
            asset_dir.mkdir()

            validate_output_folder(asset_dir, overwrite=True)

    def test_validate_output_folder_creates_parent_for_new_asset(self):
        with tempfile.TemporaryDirectory() as temp:
            asset_dir = Path(temp) / "nested" / "new_asset"

            validate_output_folder(asset_dir, overwrite=False)

            self.assertTrue(asset_dir.parent.exists())


if __name__ == "__main__":
    unittest.main()
