import ast
import unittest
from pathlib import Path


PREFERENCES_PATH = Path(__file__).parents[1] / "addon" / "preferences.py"


class PreferencesTests(unittest.TestCase):
    def test_dynamic_polyhaven_enum_does_not_define_string_default(self):
        tree = ast.parse(PREFERENCES_PATH.read_text(encoding="utf-8"))

        self._assert_dynamic_enum_has_no_default(tree, "selected_polyhaven_asset")

    def test_dynamic_downloaded_material_enum_does_not_define_string_default(self):
        tree = ast.parse(PREFERENCES_PATH.read_text(encoding="utf-8"))

        self._assert_dynamic_enum_has_no_default(tree, "selected_downloaded_polyhaven_material")

    def _assert_dynamic_enum_has_no_default(self, tree, property_name):
        for node in ast.walk(tree):
            if not isinstance(node, ast.AnnAssign):
                continue
            if not isinstance(node.target, ast.Name):
                continue
            if node.target.id != property_name:
                continue
            call = node.annotation
            self.assertIsInstance(call, ast.Call)
            keyword_names = {keyword.arg for keyword in call.keywords}
            self.assertIn("items", keyword_names)
            self.assertNotIn("default", keyword_names)
            return

        self.fail(f"{property_name} property was not found")


if __name__ == "__main__":
    unittest.main()
