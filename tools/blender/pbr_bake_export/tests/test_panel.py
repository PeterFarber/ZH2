import ast
import unittest
from pathlib import Path


PANEL_PATH = Path(__file__).parents[1] / "addon" / "panel.py"


class PanelTests(unittest.TestCase):
    def _operator_ids(self):
        tree = ast.parse(PANEL_PATH.read_text(encoding="utf-8"))
        operator_ids = []
        for node in ast.walk(tree):
            if not isinstance(node, ast.Call):
                continue
            if not isinstance(node.func, ast.Attribute):
                continue
            if node.func.attr != "operator":
                continue
            if not node.args:
                continue
            operator_id = node.args[0]
            if isinstance(operator_id, ast.Constant) and isinstance(operator_id.value, str):
                operator_ids.append(operator_id.value)
        return operator_ids

    def test_panel_exposes_separate_bake_and_export_buttons(self):
        operator_ids = self._operator_ids()

        self.assertIn("zh2.bake_selected", operator_ids)
        self.assertIn("zh2.export_selected", operator_ids)
        self.assertNotIn("zh2.bake_export_selected", operator_ids)

    def test_panel_exposes_apply_downloaded_polyhaven_material_button(self):
        operator_ids = self._operator_ids()

        self.assertIn("zh2.polyhaven_apply_material", operator_ids)

    def test_panel_exposes_downloaded_material_dropdown(self):
        tree = ast.parse(PANEL_PATH.read_text(encoding="utf-8"))

        for node in ast.walk(tree):
            if not isinstance(node, ast.Call):
                continue
            if not isinstance(node.func, ast.Attribute):
                continue
            if node.func.attr != "prop" or len(node.args) < 2:
                continue
            prop_name = node.args[1]
            if isinstance(prop_name, ast.Constant) and prop_name.value == "selected_downloaded_polyhaven_material":
                return

        self.fail("Downloaded Poly Haven material dropdown is not drawn")


if __name__ == "__main__":
    unittest.main()
