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
        self.timeouts = []
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
            "https://api.polyhaven.com/files/folder%2Fname%20with%20spaces": {},
        }

    def __call__(self, request, timeout=None):
        self.calls.append(request)
        self.timeouts.append(timeout)
        return FakeResponse(self.payloads[request.full_url])


class PolyhavenTests(unittest.TestCase):
    def test_search_textures_sends_configured_user_agent(self):
        fetcher = FakeFetcher()
        client = PolyhavenClient(user_agent="ZH2-Test/1.0", fetcher=fetcher)

        results = client.search_textures("brick")

        self.assertEqual(results[0].asset_id, "red_brick_floor")
        self.assertEqual(results[0].name, "Red Brick Floor")
        self.assertEqual(fetcher.calls[0].headers["User-agent"], "ZH2-Test/1.0")

    def test_search_textures_uses_configured_timeout(self):
        fetcher = FakeFetcher()
        client = PolyhavenClient(user_agent="ZH2-Test/1.0", fetcher=fetcher, timeout_seconds=12.5)

        client.search_textures("brick")

        self.assertEqual(fetcher.timeouts, [12.5])

    def test_empty_user_agent_is_rejected(self):
        with self.assertRaises(PolyhavenError):
            PolyhavenClient(user_agent=" ")

    def test_match_texture_role_handles_polyhaven_names(self):
        self.assertEqual(match_texture_role("red_brick_floor_diff_4k.png"), "basecolor")
        self.assertEqual(match_texture_role("red_brick_floor_nor_gl_4k.png"), "normal")
        self.assertEqual(match_texture_role("red_brick_floor_rough_4k.png"), "roughness")
        self.assertEqual(match_texture_role("red_brick_floor_metal_4k.png"), "metallic")
        self.assertEqual(match_texture_role("red_brick_floor_ao_4k.png"), "ao")

    def test_match_texture_role_uses_map_suffix_tokens_not_asset_slug_tokens(self):
        self.assertEqual(match_texture_role("metal_plate_ao_4k.png"), "ao")
        self.assertEqual(match_texture_role("rough_concrete_ao_4k.png"), "ao")
        self.assertEqual(match_texture_role("nordic_wall_rough_4k.png"), "roughness")

    def test_select_texture_files_requires_all_roles_at_requested_resolution(self):
        client = PolyhavenClient(user_agent="ZH2-Test/1.0", fetcher=FakeFetcher())

        files = client.files_for_asset("red_brick_floor")
        selection = select_texture_files(files, TextureResolution.FOUR_K, allow_resolution_fallback=False)

        self.assertIsInstance(selection, TextureFileSelection)
        self.assertEqual(selection.urls["basecolor"], "https://cdn/basecolor.png")
        self.assertEqual(selection.urls["normal"], "https://cdn/normal.png")
        self.assertEqual(selection.resolution, TextureResolution.FOUR_K)

    def test_select_texture_files_uses_deterministic_fallback_resolution(self):
        def tree_with_resolution_order(order):
            role_suffixes = {
                "basecolor": "diff",
                "normal": "nor_gl",
                "roughness": "rough",
                "metallic": "metal",
                "ao": "ao",
            }
            tree = {}
            for resolution in order:
                tree[resolution] = {
                    "png": {
                        f"sample_{suffix}_{resolution}.png": {"url": f"https://cdn/{resolution}/{role}.png"}
                        for role, suffix in role_suffixes.items()
                    }
                }
            return tree

        first_selection = select_texture_files(
            tree_with_resolution_order(("2k", "4k")),
            TextureResolution.EIGHT_K,
            allow_resolution_fallback=True,
        )
        reversed_selection = select_texture_files(
            tree_with_resolution_order(("4k", "2k")),
            TextureResolution.EIGHT_K,
            allow_resolution_fallback=True,
        )

        self.assertEqual(first_selection.urls["basecolor"], "https://cdn/4k/basecolor.png")
        self.assertEqual(reversed_selection.urls["basecolor"], "https://cdn/4k/basecolor.png")
        self.assertEqual(first_selection.urls, reversed_selection.urls)

    def test_files_for_asset_url_encodes_asset_id_path_segment(self):
        fetcher = FakeFetcher()
        client = PolyhavenClient(user_agent="ZH2-Test/1.0", fetcher=fetcher)

        client.files_for_asset("folder/name with spaces")

        self.assertEqual(fetcher.calls[0].full_url, "https://api.polyhaven.com/files/folder%2Fname%20with%20spaces")

    def test_download_selection_writes_role_named_files(self):
        binary_payloads = {
            "https://cdn/basecolor.png": b"base",
            "https://cdn/normal.png": b"normal",
            "https://cdn/roughness.png": b"rough",
            "https://cdn/metallic.png": b"metal",
            "https://cdn/ao.png": b"ao",
        }

        def download_fetcher(request, timeout=None):
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

    def test_download_selection_rejects_unsafe_prefixes_before_writing(self):
        selection = TextureFileSelection(
            resolution=TextureResolution.FOUR_K,
            urls={"basecolor": "https://cdn/basecolor.png"},
            warnings=[],
        )

        def fetcher(request, timeout=None):
            raise AssertionError("download should not start for unsafe paths")

        for prefix in ("../x", "..\\x", "C:\\temp\\x"):
            with self.subTest(prefix=prefix):
                with tempfile.TemporaryDirectory() as temp:
                    root = Path(temp)
                    cache = root / "cache"
                    client = PolyhavenClient(user_agent="ZH2-Test/1.0", fetcher=fetcher)

                    with self.assertRaises(PolyhavenError):
                        client.download_selection(selection, cache, prefix)

                    self.assertEqual(sorted(path.relative_to(root) for path in root.rglob("*")), [Path("cache")])

    def test_download_selection_rejects_unsafe_roles_before_writing(self):
        selection = TextureFileSelection(
            resolution=TextureResolution.FOUR_K,
            urls={"../basecolor": "https://cdn/basecolor.png"},
            warnings=[],
        )

        def fetcher(request, timeout=None):
            raise AssertionError("download should not start for unsafe paths")

        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            cache = root / "cache"
            client = PolyhavenClient(user_agent="ZH2-Test/1.0", fetcher=fetcher)

            with self.assertRaises(PolyhavenError):
                client.download_selection(selection, cache, "brick")

            self.assertEqual(sorted(path.relative_to(root) for path in root.rglob("*")), [Path("cache")])

    def test_search_textures_wraps_fetch_failures_as_polyhaven_errors(self):
        def failing_fetcher(request, timeout=None):
            raise OSError("network unavailable")

        client = PolyhavenClient(user_agent="ZH2-Test/1.0", fetcher=failing_fetcher)

        with self.assertRaisesRegex(PolyhavenError, "/assets"):
            client.search_textures("brick")

    def test_search_textures_wraps_json_decode_failures_as_polyhaven_errors(self):
        class InvalidJsonResponse:
            def __enter__(self):
                return self

            def __exit__(self, exc_type, exc_value, traceback):
                return False

            def read(self):
                return b"{"

        def invalid_json_fetcher(request, timeout=None):
            return InvalidJsonResponse()

        client = PolyhavenClient(user_agent="ZH2-Test/1.0", fetcher=invalid_json_fetcher)

        with self.assertRaisesRegex(PolyhavenError, "invalid JSON"):
            client.search_textures("brick")

    def test_download_selection_wraps_download_failures_with_role_context(self):
        selection = TextureFileSelection(
            resolution=TextureResolution.FOUR_K,
            urls={"basecolor": "https://cdn/basecolor.png"},
            warnings=[],
        )

        def failing_fetcher(request, timeout=None):
            raise OSError("network unavailable")

        client = PolyhavenClient(user_agent="ZH2-Test/1.0", fetcher=failing_fetcher)

        with tempfile.TemporaryDirectory() as temp:
            with self.assertRaisesRegex(PolyhavenError, "basecolor"):
                client.download_selection(selection, Path(temp), "brick")

    def test_download_selection_rejects_invalid_url_schemes_and_hosts_before_fetching(self):
        cases = (
            "file:///tmp/secret.png",
            "",
            "https:///missing-host.png",
        )
        for url in cases:
            with self.subTest(url=url):
                calls = []

                def fetcher(request, timeout=None):
                    calls.append(request)
                    raise AssertionError("download should not start for invalid URLs")

                selection = TextureFileSelection(
                    resolution=TextureResolution.FOUR_K,
                    urls={"basecolor": url},
                    warnings=[],
                )
                client = PolyhavenClient(user_agent="ZH2-Test/1.0", fetcher=fetcher)

                with tempfile.TemporaryDirectory() as temp:
                    with self.assertRaisesRegex(PolyhavenError, "basecolor"):
                        client.download_selection(selection, Path(temp), "brick")
                    self.assertEqual(calls, [])

    def test_download_selection_rejects_malformed_hosts_and_ports_before_fetching(self):
        cases = (
            ("https://@/x", "missing or blank host"),
            ("https://:443/x", "missing or blank host"),
            ("https://user:pass@/x", "missing or blank host"),
            ("https:// /x", "missing or blank host"),
            ("https://example.com:bad/x", "Port could not be cast"),
        )
        for url, expected_error in cases:
            with self.subTest(url=url):
                calls = []

                def fetcher(request, timeout=None):
                    calls.append(request)
                    raise AssertionError("download should not start for malformed hosts")

                selection = TextureFileSelection(
                    resolution=TextureResolution.FOUR_K,
                    urls={"basecolor": url},
                    warnings=[],
                )
                client = PolyhavenClient(user_agent="ZH2-Test/1.0", fetcher=fetcher)

                with tempfile.TemporaryDirectory() as temp:
                    with self.assertRaisesRegex(PolyhavenError, f"basecolor.*{expected_error}"):
                        client.download_selection(selection, Path(temp), "brick")
                    self.assertEqual(calls, [])


if __name__ == "__main__":
    unittest.main()
