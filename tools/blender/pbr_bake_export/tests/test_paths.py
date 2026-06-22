import unittest
from pathlib import Path

from tools.blender.pbr_bake_export.addon.paths import build_asset_paths, sanitize_asset_name


class PathTests(unittest.TestCase):
    def test_sanitize_asset_name_is_ascii_lowercase_and_path_safe(self):
        self.assertEqual(sanitize_asset_name("Wall Panel 01"), "wall_panel_01")
        self.assertEqual(sanitize_asset_name("Goal: Frame/Left"), "goal_frame_left")
        self.assertEqual(sanitize_asset_name("  ICE.Surface  "), "ice_surface")

    def test_sanitize_asset_name_uses_asset_when_name_has_no_safe_characters(self):
        self.assertEqual(sanitize_asset_name("###"), "asset")

    def test_build_asset_paths_uses_per_object_folder_and_texture_names(self):
        paths = build_asset_paths(Path("assets"), "Wall Panel")
        self.assertEqual(paths.asset_name, "wall_panel")
        self.assertEqual(paths.asset_dir, Path("assets") / "wall_panel")
        self.assertEqual(paths.glb_path, Path("assets") / "wall_panel" / "wall_panel.glb")
        self.assertEqual(paths.textures["basecolor"], Path("assets") / "wall_panel" / "textures" / "wall_panel_basecolor.png")
        self.assertEqual(paths.textures["normal"], Path("assets") / "wall_panel" / "textures" / "wall_panel_normal.png")
        self.assertEqual(paths.manifest_path, Path("assets") / "wall_panel" / "manifest.json")


if __name__ == "__main__":
    unittest.main()
