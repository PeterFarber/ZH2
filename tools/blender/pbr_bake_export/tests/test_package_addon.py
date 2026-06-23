import tempfile
import unittest
from pathlib import Path
from zipfile import ZipFile

from tools.blender.pbr_bake_export.package_addon import build_addon_zip


class PackageAddonTests(unittest.TestCase):
    def test_build_addon_zip_creates_blender_installable_package(self):
        with tempfile.TemporaryDirectory() as temp:
            output_path = Path(temp) / "zh2_pbr_bake_export.zip"

            build_addon_zip(output_path)

            self.assertTrue(output_path.exists())
            with ZipFile(output_path) as archive:
                names = set(archive.namelist())

            self.assertIn("zh2_pbr_bake_export/__init__.py", names)
            self.assertIn("zh2_pbr_bake_export/operators.py", names)
            self.assertIn("zh2_pbr_bake_export/panel.py", names)
            self.assertNotIn("zh2_pbr_bake_export/tests/test_polyhaven.py", names)
            self.assertFalse(any("__pycache__" in name for name in names))


if __name__ == "__main__":
    unittest.main()
