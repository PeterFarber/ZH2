import json
import tempfile
import unittest
from pathlib import Path

from tools.blender.pbr_bake_export.addon.polyhaven import (
    PolyhavenClient,
    PolyhavenError,
    TextureFileSelection,
    match_texture_role,
    select_texture_files,
)
from tools.blender.pbr_bake_export.addon.types import TextureResolution


class FakeResponse:
    def __init__(self, payload: object):
        self.payload = payload

    def read(self) -> bytes:
        return json.dumps(self.payload).encode("utf-8")

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        return False


class FakeFetcher:
    def __init__(self):
        self.calls = []
        self.payloads = {
            "https://api.polyhaven.com/assets?type=textures": {
                "red_brick_floor": {"name": "Red Brick Floor", "type": "textures"},
                "ice_floor": {"name": "Ice Floor", "type": "textures"},
            },
            "https://api.polyhaven.com/files/red_brick_floor": {
                "4k": {
                    "png": {
                        "red_brick_floor_diff_4k.png": {"url": "https://cdn/basecolor.png"},
                        "red_brick_floor_nor_gl_4k.png": {"url": "https://cdn/normal.png"},
                        "red_brick_floor_rough_4k.png": {"url": "https://cdn/roughness.png"},
                        "red_brick_floor_metal_4k.png": {"url": "https://cdn/metallic.png"},
                        "red_brick_floor_ao_4k.png": {"url": "https://cdn/ao.png"},
                    }
                }
            },
        }

    def __call__(self, request):
        self.calls.append(request)
        return FakeResponse(self.payloads[request.full_url])


class PolyhavenTests(unittest.TestCase):
    def test_search_textures_sends_configured_user_agent(self):
        fetcher = FakeFetcher()
        client = PolyhavenClient(user_agent="ZH2-Test/1.0", fetcher=fetcher)

        results = client.search_textures("brick")

        self.assertEqual(results[0].asset_id, "red_brick_floor")
        self.assertEqual(results[0].name, "Red Brick Floor")
        self.assertEqual(fetcher.calls[0].headers["User-agent"], "ZH2-Test/1.0")

    def test_empty_user_agent_is_rejected(self):
        with self.assertRaises(PolyhavenError):
            PolyhavenClient(user_agent=" ")

    def test_match_texture_role_handles_polyhaven_names(self):
        self.assertEqual(match_texture_role("red_brick_floor_diff_4k.png"), "basecolor")
        self.assertEqual(match_texture_role("red_brick_floor_nor_gl_4k.png"), "normal")
        self.assertEqual(match_texture_role("red_brick_floor_rough_4k.png"), "roughness")
        self.assertEqual(match_texture_role("red_brick_floor_metal_4k.png"), "metallic")
        self.assertEqual(match_texture_role("red_brick_floor_ao_4k.png"), "ao")

    def test_select_texture_files_requires_all_roles_at_requested_resolution(self):
        client = PolyhavenClient(user_agent="ZH2-Test/1.0", fetcher=FakeFetcher())

        files = client.files_for_asset("red_brick_floor")
        selection = select_texture_files(files, TextureResolution.FOUR_K, allow_resolution_fallback=False)

        self.assertIsInstance(selection, TextureFileSelection)
        self.assertEqual(selection.urls["basecolor"], "https://cdn/basecolor.png")
        self.assertEqual(selection.urls["normal"], "https://cdn/normal.png")
        self.assertEqual(selection.resolution, TextureResolution.FOUR_K)

    def test_download_selection_writes_role_named_files(self):
        binary_payloads = {
            "https://cdn/basecolor.png": b"base",
            "https://cdn/normal.png": b"normal",
            "https://cdn/roughness.png": b"rough",
            "https://cdn/metallic.png": b"metal",
            "https://cdn/ao.png": b"ao",
        }

        def download_fetcher(request):
            class BinaryResponse:
                def __enter__(self):
                    return self

                def __exit__(self, exc_type, exc_value, traceback):
                    return False

                def read(self):
                    return binary_payloads[request.full_url]

            return BinaryResponse()

        client = PolyhavenClient(user_agent="ZH2-Test/1.0", fetcher=download_fetcher)
        selection = TextureFileSelection(
            resolution=TextureResolution.FOUR_K,
            urls={
                "basecolor": "https://cdn/basecolor.png",
                "normal": "https://cdn/normal.png",
                "roughness": "https://cdn/roughness.png",
                "metallic": "https://cdn/metallic.png",
                "ao": "https://cdn/ao.png",
            },
            warnings=[],
        )

        with tempfile.TemporaryDirectory() as temp:
            written = client.download_selection(selection, Path(temp), "brick")

            self.assertEqual((Path(temp) / "brick_basecolor.png").read_bytes(), b"base")
            self.assertEqual(written["ao"], Path(temp) / "brick_ao.png")


if __name__ == "__main__":
    unittest.main()
