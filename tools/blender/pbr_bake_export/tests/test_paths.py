import unittest
import tempfile
from pathlib import Path

from tools.blender.pbr_bake_export.addon.paths import build_asset_paths, sanitize_asset_name


class PathTests(unittest.TestCase):
    def test_sanitize_asset_name_is_ascii_lowercase_and_path_safe(self):
        self.assertEqual(sanitize_asset_name("Wall Panel 01"), "wall_panel_01")
        self.assertEqual(sanitize_asset_name("Goal: Frame/Left"), "goal_frame_left")
        self.assertEqual(sanitize_asset_name("  ICE.Surface  "), "ice_surface")

    def test_sanitize_asset_name_uses_asset_when_name_has_no_safe_characters(self):
        self.assertEqual(sanitize_asset_name("###"), "asset")

    def test_sanitize_asset_name_prefixes_windows_reserved_names(self):
        self.assertEqual(sanitize_asset_name("CON"), "asset_con")
        self.assertEqual(sanitize_asset_name("nul"), "asset_nul")
        self.assertEqual(sanitize_asset_name("COM1"), "asset_com1")

    def test_build_asset_paths_uses_per_object_folder_and_texture_names(self):
        paths = build_asset_paths(Path("assets"), "Wall Panel")
        self.assertEqual(paths.asset_name, "wall_panel")
        self.assertEqual(paths.asset_dir, Path("assets") / "wall_panel")
        self.assertEqual(paths.glb_path, Path("assets") / "wall_panel" / "wall_panel.glb")
        self.assertEqual(paths.texture_dir, Path("assets") / "wall_panel" / "textures")
        self.assertEqual(paths.textures["basecolor"], Path("assets") / "wall_panel" / "textures" / "wall_panel_basecolor.png")
        self.assertEqual(paths.textures["normal"], Path("assets") / "wall_panel" / "textures" / "wall_panel_normal.png")
        self.assertEqual(paths.textures["roughness"], Path("assets") / "wall_panel" / "textures" / "wall_panel_roughness.png")
        self.assertEqual(paths.textures["metallic"], Path("assets") / "wall_panel" / "textures" / "wall_panel_metallic.png")
        self.assertEqual(paths.textures["ao"], Path("assets") / "wall_panel" / "textures" / "wall_panel_ao.png")
        self.assertEqual(paths.manifest_path, Path("assets") / "wall_panel" / "manifest.json")

    def test_build_asset_paths_suffixes_reserved_name_collisions(self):
        paths = build_asset_paths(Path("assets"), "Goal Frame Left", reserved_names={"goal_frame_left"})
        self.assertEqual(paths.asset_name, "goal_frame_left_2")

    def test_build_asset_paths_suffixes_until_name_is_available(self):
        paths = build_asset_paths(Path("assets"), "###", reserved_names={"asset", "asset_2"})
        self.assertEqual(paths.asset_name, "asset_3")

    def test_build_asset_paths_suffixes_existing_output_directories(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "wall_panel").mkdir()

            paths = build_asset_paths(root, "Wall Panel")

            self.assertEqual(paths.asset_name, "wall_panel_2")

    def test_build_asset_paths_suffixes_existing_output_files(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "wall_panel").write_text("", encoding="utf-8")

            paths = build_asset_paths(root, "Wall Panel")

            self.assertEqual(paths.asset_name, "wall_panel_2")


if __name__ == "__main__":
    unittest.main()
