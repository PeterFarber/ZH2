# Blender PBR Bake Export Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a Blender add-on that downloads Poly Haven PBR texture sets, bakes selected mesh objects at 4K or 8K, optionally autogenerates low-poly export targets, and exports one GLB plus external texture files per selected object under `assets/<object_name>/`.

**Architecture:** Keep the add-on isolated under `tools/blender/pbr_bake_export/`. Pure Python modules handle path planning, manifest writing, Poly Haven API/file resolution, material plans, and export validation so they can be covered with stdlib `unittest`; Blender-dependent modules wrap those helpers with `bpy` operators, panels, baking, material creation, mesh duplication, decimation, and GLB export.

**Tech Stack:** Blender Python API (`bpy`), Python 3 standard library (`dataclasses`, `datetime`, `json`, `pathlib`, `re`, `urllib`, `unittest`), Poly Haven public API, Blender glTF exporter.

---

## File Structure

Create these files:

```text
tools/blender/pbr_bake_export/
  __init__.py
  README.md
  addon/
    __init__.py
    bake.py
    export.py
    manifest.py
    materials.py
    operators.py
    panel.py
    paths.py
    polyhaven.py
    preferences.py
    types.py
  tests/
    test_bake.py
    test_export.py
    test_manifest.py
    test_materials.py
    test_paths.py
    test_polyhaven.py
    test_types.py
```

Responsibilities:

- `types.py`: shared enums/dataclasses that do not import `bpy`.
- `paths.py`: filename sanitization and per-object output path planning.
- `manifest.py`: manifest data model and JSON writing.
- `polyhaven.py`: Poly Haven API client, recursive file tree parsing, map-role resolution, and file download.
- `materials.py`: pure material plan creation plus Blender material construction.
- `bake.py`: bake map definitions plus Blender image/bake execution.
- `export.py`: selected object duplication, optional decimation, UV checks, and GLB export.
- `preferences.py`: add-on preferences and workflow settings property group.
- `panel.py`: 3D View sidebar UI.
- `operators.py`: Blender operators that coordinate search, download, bake, and export.
- `README.md`: installation, usage, output contract, Poly Haven API notice, and manual verification notes.

Do not modify engine CMake, renderer, cooked assets, `data/raw`, `data/cooked`, or Phase 5 runtime code for this add-on.

---

### Task 1: Shared Types, Paths, And Manifest

**Files:**
- Create: `tools/blender/pbr_bake_export/__init__.py`
- Create: `tools/blender/pbr_bake_export/addon/__init__.py`
- Create: `tools/blender/pbr_bake_export/addon/types.py`
- Create: `tools/blender/pbr_bake_export/addon/paths.py`
- Create: `tools/blender/pbr_bake_export/addon/manifest.py`
- Create: `tools/blender/pbr_bake_export/tests/test_types.py`
- Create: `tools/blender/pbr_bake_export/tests/test_paths.py`
- Create: `tools/blender/pbr_bake_export/tests/test_manifest.py`

- [ ] **Step 1: Write failing tests for shared dataclasses and filename/path planning**

Create `tools/blender/pbr_bake_export/tests/test_types.py`:

```python
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
```

Create `tools/blender/pbr_bake_export/tests/test_paths.py`:

```python
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
```

- [ ] **Step 2: Write failing tests for manifest serialization**

Create `tools/blender/pbr_bake_export/tests/test_manifest.py`:

```python
import json
import tempfile
import unittest
from pathlib import Path

from tools.blender.pbr_bake_export.addon.manifest import ExportManifest, write_manifest
from tools.blender.pbr_bake_export.addon.types import AssetMode, MaterialSource, TextureResolution


class ManifestTests(unittest.TestCase):
    def test_write_manifest_outputs_stable_relative_paths(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            manifest = ExportManifest(
                asset_name="wall_panel",
                source_object="Wall Panel",
                exported_at="2026-06-22T12:00:00-04:00",
                mode=AssetMode.LOW_POLY,
                autogenerated_low_poly=True,
                resolution=TextureResolution.FOUR_K,
                material_source=MaterialSource.POLY_HAVEN,
                polyhaven_asset_id="red_brick_floor",
                glb="wall_panel.glb",
                textures={
                    "basecolor": "textures/wall_panel_basecolor.png",
                    "normal": "textures/wall_panel_normal.png",
                    "roughness": "textures/wall_panel_roughness.png",
                    "metallic": "textures/wall_panel_metallic.png",
                    "ao": "textures/wall_panel_ao.png",
                },
                warnings=["roughness used constant 0.5"],
            )

            write_manifest(root / "manifest.json", manifest)

            data = json.loads((root / "manifest.json").read_text(encoding="utf-8"))
            self.assertEqual(data["asset_name"], "wall_panel")
            self.assertEqual(data["mode"], "Low Poly")
            self.assertEqual(data["resolution"], 4096)
            self.assertEqual(data["material_source"], "Poly Haven")
            self.assertEqual(data["textures"]["normal"], "textures/wall_panel_normal.png")
            self.assertEqual(data["warnings"], ["roughness used constant 0.5"])


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 3: Run tests to verify they fail because modules do not exist**

Run:

```powershell
python -m unittest discover -s tools/blender/pbr_bake_export/tests -p "test_*.py"
```

Expected: `FAILED` or `ERROR` with import errors for `tools.blender.pbr_bake_export.addon`.

- [ ] **Step 4: Implement shared package files**

Create `tools/blender/pbr_bake_export/__init__.py`:

```python
"""Repo-managed Blender PBR bake/export add-on package."""
```

Create `tools/blender/pbr_bake_export/addon/types.py`:

```python
from __future__ import annotations

from dataclasses import dataclass
from enum import Enum
from pathlib import Path


class TextureResolution(Enum):
    FOUR_K = ("4K", 4096)
    EIGHT_K = ("8K", 8192)
    TEST = ("Test 128", 128)

    @property
    def label(self) -> str:
        return self.value[0]

    @property
    def pixel_size(self) -> int:
        return self.value[1]


class AssetMode(Enum):
    HIGH_POLY = "High Poly"
    LOW_POLY = "Low Poly"


class MaterialSource(Enum):
    EXISTING = "Existing Material"
    POLY_HAVEN = "Poly Haven"


@dataclass(frozen=True)
class AssetPaths:
    asset_name: str
    asset_dir: Path
    texture_dir: Path
    glb_path: Path
    manifest_path: Path
    textures: dict[str, Path]
```

Create `tools/blender/pbr_bake_export/addon/paths.py`:

```python
from __future__ import annotations

import re
from pathlib import Path

from .types import AssetPaths


TEXTURE_ROLES = ("basecolor", "normal", "roughness", "metallic", "ao")


def sanitize_asset_name(name: str) -> str:
    ascii_name = name.encode("ascii", errors="ignore").decode("ascii")
    safe = re.sub(r"[^A-Za-z0-9]+", "_", ascii_name.strip()).strip("_").lower()
    safe = re.sub(r"_+", "_", safe)
    return safe or "asset"


def build_asset_paths(output_root: Path, object_name: str) -> AssetPaths:
    asset_name = sanitize_asset_name(object_name)
    asset_dir = output_root / asset_name
    texture_dir = asset_dir / "textures"
    textures = {
        role: texture_dir / f"{asset_name}_{role}.png"
        for role in TEXTURE_ROLES
    }
    return AssetPaths(
        asset_name=asset_name,
        asset_dir=asset_dir,
        texture_dir=texture_dir,
        glb_path=asset_dir / f"{asset_name}.glb",
        manifest_path=asset_dir / "manifest.json",
        textures=textures,
    )
```

Create `tools/blender/pbr_bake_export/addon/manifest.py`:

```python
from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path

from .types import AssetMode, MaterialSource, TextureResolution


@dataclass(frozen=True)
class ExportManifest:
    asset_name: str
    source_object: str
    exported_at: str
    mode: AssetMode
    autogenerated_low_poly: bool
    resolution: TextureResolution
    material_source: MaterialSource
    polyhaven_asset_id: str
    glb: str
    textures: dict[str, str]
    warnings: list[str]

    def to_json_dict(self) -> dict[str, object]:
        return {
            "asset_name": self.asset_name,
            "source_object": self.source_object,
            "exported_at": self.exported_at,
            "mode": self.mode.value,
            "autogenerated_low_poly": self.autogenerated_low_poly,
            "resolution": self.resolution.pixel_size,
            "material_source": self.material_source.value,
            "polyhaven_asset_id": self.polyhaven_asset_id,
            "glb": self.glb,
            "textures": self.textures,
            "warnings": self.warnings,
        }


def write_manifest(path: Path, manifest: ExportManifest) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(manifest.to_json_dict(), indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
```

Create `tools/blender/pbr_bake_export/addon/__init__.py`:

```python
bl_info = {
    "name": "ZH2 PBR Bake Export",
    "author": "ZH2",
    "version": (0, 1, 0),
    "blender": (4, 0, 0),
    "location": "View3D > Sidebar > ZH2 Assets",
    "description": "Bake selected PBR modular assets and export GLB folders.",
    "category": "Import-Export",
}


def register():
    return None


def unregister():
    return None
```

- [ ] **Step 5: Run tests to verify they pass**

Run:

```powershell
python -m unittest discover -s tools/blender/pbr_bake_export/tests -p "test_*.py"
```

Expected: all Task 1 tests pass.

- [ ] **Step 6: Commit Task 1**

```powershell
git add tools/blender/pbr_bake_export
git commit -m "Add Blender PBR export path and manifest helpers"
```

---

### Task 2: Poly Haven API Client And Texture File Resolution

**Files:**
- Create: `tools/blender/pbr_bake_export/addon/polyhaven.py`
- Create: `tools/blender/pbr_bake_export/tests/test_polyhaven.py`

- [ ] **Step 1: Write failing tests for User-Agent, search parsing, and file role resolution**

Create `tools/blender/pbr_bake_export/tests/test_polyhaven.py`:

```python
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
```

- [ ] **Step 2: Run tests to verify they fail because `polyhaven.py` does not exist**

Run:

```powershell
python -m unittest tools.blender.pbr_bake_export.tests.test_polyhaven
```

Expected: `ERROR` with `ModuleNotFoundError` for `polyhaven`.

- [ ] **Step 3: Implement Poly Haven client and file resolver**

Create `tools/blender/pbr_bake_export/addon/polyhaven.py`:

```python
from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Iterator
from urllib.parse import urlencode
from urllib.request import Request, urlopen

from .types import TextureResolution


API_ROOT = "https://api.polyhaven.com"
REQUIRED_ROLES = ("basecolor", "normal", "roughness", "metallic", "ao")


class PolyhavenError(RuntimeError):
    pass


@dataclass(frozen=True)
class PolyhavenAsset:
    asset_id: str
    name: str


@dataclass(frozen=True)
class TextureFileSelection:
    resolution: TextureResolution
    urls: dict[str, str]
    warnings: list[str]


def match_texture_role(filename: str) -> str:
    lower = filename.lower().replace("-", "_").replace(" ", "_")
    if any(token in lower for token in ("diff", "albedo", "basecolor", "base_color", "col_")):
        return "basecolor"
    if any(token in lower for token in ("nor", "normal", "nrm")):
        return "normal"
    if any(token in lower for token in ("rough", "roughness")):
        return "roughness"
    if any(token in lower for token in ("metal", "metallic", "metalness")):
        return "metallic"
    if any(token in lower for token in ("ao", "ambient_occlusion", "ambientocclusion")):
        return "ao"
    return ""


def _iter_file_records(node: object, labels: tuple[str, ...] = ()) -> Iterator[tuple[tuple[str, ...], dict[str, object]]]:
    if isinstance(node, dict):
        if "url" in node and isinstance(node["url"], str):
            yield labels, node
        for key, value in node.items():
            yield from _iter_file_records(value, labels + (str(key),))
    elif isinstance(node, list):
        for index, value in enumerate(node):
            yield from _iter_file_records(value, labels + (str(index),))


def _resolution_label(resolution: TextureResolution) -> str:
    if resolution is TextureResolution.EIGHT_K:
        return "8k"
    if resolution is TextureResolution.FOUR_K:
        return "4k"
    return "test"


def select_texture_files(
    file_tree: object,
    resolution: TextureResolution,
    allow_resolution_fallback: bool,
) -> TextureFileSelection:
    requested_label = _resolution_label(resolution)
    candidates: dict[str, str] = {}
    fallback_candidates: dict[str, str] = {}

    for labels, record in _iter_file_records(file_tree):
        joined = "_".join(labels).lower()
        url = str(record["url"])
        role = match_texture_role(joined)
        if not role:
            continue
        if requested_label in joined:
            candidates.setdefault(role, url)
        fallback_candidates.setdefault(role, url)

    missing = [role for role in REQUIRED_ROLES if role not in candidates]
    warnings: list[str] = []
    if missing and allow_resolution_fallback:
        for role in missing:
            if role in fallback_candidates:
                candidates[role] = fallback_candidates[role]
                warnings.append(f"{role} used available fallback resolution")
        missing = [role for role in REQUIRED_ROLES if role not in candidates]

    if missing:
        joined_missing = ", ".join(missing)
        raise PolyhavenError(f"missing required texture roles: {joined_missing}")

    return TextureFileSelection(resolution=resolution, urls={role: candidates[role] for role in REQUIRED_ROLES}, warnings=warnings)


class PolyhavenClient:
    def __init__(self, user_agent: str, fetcher: Callable[[Request], object] | None = None):
        if not user_agent.strip():
            raise PolyhavenError("Poly Haven requests require a unique User-Agent")
        self.user_agent = user_agent.strip()
        self.fetcher = fetcher or urlopen

    def _request_json(self, path: str, query: dict[str, str] | None = None) -> object:
        url = f"{API_ROOT}{path}"
        if query:
            url = f"{url}?{urlencode(query)}"
        request = Request(url, headers={"User-Agent": self.user_agent})
        with self.fetcher(request) as response:
            return json.loads(response.read().decode("utf-8"))

    def search_textures(self, query: str) -> list[PolyhavenAsset]:
        data = self._request_json("/assets", {"type": "textures"})
        assets: list[PolyhavenAsset] = []
        query_lower = query.strip().lower()
        if not isinstance(data, dict):
            raise PolyhavenError("unexpected Poly Haven assets response")
        for asset_id, payload in sorted(data.items()):
            if not isinstance(payload, dict):
                continue
            name = str(payload.get("name", asset_id))
            searchable = f"{asset_id} {name}".lower()
            if query_lower and query_lower not in searchable:
                continue
            assets.append(PolyhavenAsset(asset_id=str(asset_id), name=name))
        return assets

    def files_for_asset(self, asset_id: str) -> object:
        return self._request_json(f"/files/{asset_id}")

    def download_selection(self, selection: TextureFileSelection, cache_dir: Path, prefix: str) -> dict[str, Path]:
        cache_dir.mkdir(parents=True, exist_ok=True)
        written: dict[str, Path] = {}
        for role, url in selection.urls.items():
            path = cache_dir / f"{prefix}_{role}.png"
            request = Request(url, headers={"User-Agent": self.user_agent})
            with self.fetcher(request) as response:
                path.write_bytes(response.read())
            written[role] = path
        return written
```

- [ ] **Step 4: Run tests to verify they pass**

Run:

```powershell
python -m unittest tools.blender.pbr_bake_export.tests.test_polyhaven
```

Expected: all Poly Haven tests pass.

- [ ] **Step 5: Run all pure Python tests**

Run:

```powershell
python -m unittest discover -s tools/blender/pbr_bake_export/tests -p "test_*.py"
```

Expected: all tests pass.

- [ ] **Step 6: Commit Task 2**

```powershell
git add tools/blender/pbr_bake_export
git commit -m "Add Poly Haven texture API client"
```

---

### Task 3: Material Plans And Blender Material Construction

**Files:**
- Create: `tools/blender/pbr_bake_export/addon/materials.py`
- Create: `tools/blender/pbr_bake_export/tests/test_materials.py`

- [ ] **Step 1: Write failing tests for material plan creation**

Create `tools/blender/pbr_bake_export/tests/test_materials.py`:

```python
import unittest
from pathlib import Path

from tools.blender.pbr_bake_export.addon.materials import MaterialPlan, build_material_plan
from tools.blender.pbr_bake_export.addon.types import MaterialSource


class MaterialTests(unittest.TestCase):
    def test_build_polyhaven_material_plan_records_texture_paths_and_color_spaces(self):
        plan = build_material_plan(
            name="wall_panel_material",
            source=MaterialSource.POLY_HAVEN,
            texture_paths={
                "basecolor": Path("cache/wall_basecolor.png"),
                "normal": Path("cache/wall_normal.png"),
                "roughness": Path("cache/wall_roughness.png"),
                "metallic": Path("cache/wall_metallic.png"),
                "ao": Path("cache/wall_ao.png"),
            },
            polyhaven_asset_id="wall_plaster",
        )

        self.assertIsInstance(plan, MaterialPlan)
        self.assertEqual(plan.name, "wall_panel_material")
        self.assertEqual(plan.source, MaterialSource.POLY_HAVEN)
        self.assertEqual(plan.texture_color_spaces["basecolor"], "sRGB")
        self.assertEqual(plan.texture_color_spaces["normal"], "Non-Color")
        self.assertEqual(plan.texture_color_spaces["roughness"], "Non-Color")
        self.assertEqual(plan.polyhaven_asset_id, "wall_plaster")

    def test_build_existing_material_plan_allows_empty_texture_paths(self):
        plan = build_material_plan(
            name="existing",
            source=MaterialSource.EXISTING,
            texture_paths={},
            polyhaven_asset_id="",
        )

        self.assertEqual(plan.source, MaterialSource.EXISTING)
        self.assertEqual(plan.texture_paths, {})


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: Run tests to verify they fail because `materials.py` does not exist**

Run:

```powershell
python -m unittest tools.blender.pbr_bake_export.tests.test_materials
```

Expected: `ERROR` with `ModuleNotFoundError` for `materials`.

- [ ] **Step 3: Implement material plan helpers and Blender material builder**

Create `tools/blender/pbr_bake_export/addon/materials.py`:

```python
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from .types import MaterialSource


@dataclass(frozen=True)
class MaterialPlan:
    name: str
    source: MaterialSource
    texture_paths: dict[str, Path]
    texture_color_spaces: dict[str, str]
    polyhaven_asset_id: str


def build_material_plan(
    name: str,
    source: MaterialSource,
    texture_paths: dict[str, Path],
    polyhaven_asset_id: str,
) -> MaterialPlan:
    color_spaces = {
        "basecolor": "sRGB",
        "normal": "Non-Color",
        "roughness": "Non-Color",
        "metallic": "Non-Color",
        "ao": "Non-Color",
    }
    return MaterialPlan(
        name=name,
        source=source,
        texture_paths=dict(texture_paths),
        texture_color_spaces=color_spaces,
        polyhaven_asset_id=polyhaven_asset_id,
    )


def create_blender_material(plan: MaterialPlan):
    import bpy

    material = bpy.data.materials.new(plan.name)
    material.use_nodes = True
    nodes = material.node_tree.nodes
    links = material.node_tree.links
    principled = nodes.get("Principled BSDF")
    if principled is None:
        principled = nodes.new(type="ShaderNodeBsdfPrincipled")

    def add_image(role: str):
        path = plan.texture_paths.get(role)
        if path is None:
            return None
        image = bpy.data.images.load(str(path), check_existing=True)
        image.colorspace_settings.name = plan.texture_color_spaces[role]
        node = nodes.new(type="ShaderNodeTexImage")
        node.name = f"{role}_texture"
        node.image = image
        return node

    base = add_image("basecolor")
    if base is not None:
        links.new(base.outputs["Color"], principled.inputs["Base Color"])

    roughness = add_image("roughness")
    if roughness is not None:
        links.new(roughness.outputs["Color"], principled.inputs["Roughness"])

    metallic = add_image("metallic")
    if metallic is not None:
        links.new(metallic.outputs["Color"], principled.inputs["Metallic"])

    normal = add_image("normal")
    if normal is not None:
        normal_map = nodes.new(type="ShaderNodeNormalMap")
        links.new(normal.outputs["Color"], normal_map.inputs["Color"])
        links.new(normal_map.outputs["Normal"], principled.inputs["Normal"])

    return material
```

- [ ] **Step 4: Run material tests and all pure Python tests**

Run:

```powershell
python -m unittest tools.blender.pbr_bake_export.tests.test_materials
python -m unittest discover -s tools/blender/pbr_bake_export/tests -p "test_*.py"
```

Expected: all tests pass.

- [ ] **Step 5: Commit Task 3**

```powershell
git add tools/blender/pbr_bake_export
git commit -m "Add Blender PBR material planning"
```

---

### Task 4: Bake Map Plan And Blender Bake Execution

**Files:**
- Create: `tools/blender/pbr_bake_export/addon/bake.py`
- Create: `tools/blender/pbr_bake_export/tests/test_bake.py`

- [ ] **Step 1: Write failing tests for bake map definitions and export texture plan**

Create `tools/blender/pbr_bake_export/tests/test_bake.py`:

```python
import unittest

from tools.blender.pbr_bake_export.addon.bake import bake_map_specs


class BakeTests(unittest.TestCase):
    def test_bake_map_specs_include_required_pbr_outputs(self):
        specs = bake_map_specs()
        self.assertEqual([spec.role for spec in specs], ["basecolor", "normal", "roughness", "metallic", "ao"])
        self.assertEqual(specs[0].blender_bake_type, "DIFFUSE")
        self.assertEqual(specs[0].color_space, "sRGB")
        self.assertEqual(specs[1].blender_bake_type, "NORMAL")
        self.assertEqual(specs[1].color_space, "Non-Color")

    def test_every_bake_map_has_png_filename_suffix(self):
        for spec in bake_map_specs():
            self.assertEqual(spec.file_extension, ".png")


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: Run tests to verify they fail because `bake.py` does not exist**

Run:

```powershell
python -m unittest tools.blender.pbr_bake_export.tests.test_bake
```

Expected: `ERROR` with `ModuleNotFoundError` for `bake`.

- [ ] **Step 3: Implement bake definitions and Blender bake executor**

Create `tools/blender/pbr_bake_export/addon/bake.py`:

```python
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from .types import TextureResolution


@dataclass(frozen=True)
class BakeMapSpec:
    role: str
    blender_bake_type: str
    color_space: str
    file_extension: str = ".png"


def bake_map_specs() -> list[BakeMapSpec]:
    return [
        BakeMapSpec("basecolor", "DIFFUSE", "sRGB"),
        BakeMapSpec("normal", "NORMAL", "Non-Color"),
        BakeMapSpec("roughness", "ROUGHNESS", "Non-Color"),
        BakeMapSpec("metallic", "EMIT", "Non-Color"),
        BakeMapSpec("ao", "AO", "Non-Color"),
    ]


def bake_object_maps(obj, texture_paths: dict[str, Path], resolution: TextureResolution, margin: int = 16) -> list[str]:
    import bpy

    warnings: list[str] = []
    scene = bpy.context.scene
    previous_engine = scene.render.engine
    previous_selected = list(bpy.context.selected_objects)
    previous_active = bpy.context.view_layer.objects.active

    try:
        scene.render.engine = "CYCLES"
        bpy.ops.object.select_all(action="DESELECT")
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj

        for spec in bake_map_specs():
            image = bpy.data.images.new(
                name=f"{obj.name}_{spec.role}",
                width=resolution.pixel_size,
                height=resolution.pixel_size,
                alpha=False,
            )
            image.colorspace_settings.name = spec.color_space
            texture_paths[spec.role].parent.mkdir(parents=True, exist_ok=True)

            nodes = obj.active_material.node_tree.nodes if obj.active_material and obj.active_material.use_nodes else None
            if nodes is None:
                warnings.append(f"{spec.role} baked from object without node material")
            else:
                image_node = nodes.new(type="ShaderNodeTexImage")
                image_node.image = image
                nodes.active = image_node

            if spec.blender_bake_type == "DIFFUSE":
                scene.render.bake.use_pass_direct = False
                scene.render.bake.use_pass_indirect = False
                scene.render.bake.use_pass_color = True

            bpy.ops.object.bake(type=spec.blender_bake_type, margin=margin)
            image.filepath_raw = str(texture_paths[spec.role])
            image.file_format = "PNG"
            image.save()
    finally:
        scene.render.engine = previous_engine
        bpy.ops.object.select_all(action="DESELECT")
        for selected in previous_selected:
            if selected.name in bpy.context.scene.objects:
                selected.select_set(True)
        if previous_active and previous_active.name in bpy.context.scene.objects:
            bpy.context.view_layer.objects.active = previous_active

    return warnings
```

- [ ] **Step 4: Run bake tests and all pure Python tests**

Run:

```powershell
python -m unittest tools.blender.pbr_bake_export.tests.test_bake
python -m unittest discover -s tools/blender/pbr_bake_export/tests -p "test_*.py"
```

Expected: all tests pass.

- [ ] **Step 5: Commit Task 4**

```powershell
git add tools/blender/pbr_bake_export
git commit -m "Add Blender PBR bake definitions"
```

---

### Task 5: Export Validation, Low-poly Preparation, And GLB Export

**Files:**
- Create: `tools/blender/pbr_bake_export/addon/export.py`
- Create: `tools/blender/pbr_bake_export/tests/test_export.py`

- [ ] **Step 1: Write failing tests for export preflight behavior**

Create `tools/blender/pbr_bake_export/tests/test_export.py`:

```python
import tempfile
import unittest
from dataclasses import dataclass
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
            asset_dir = Path(temp) / "new_asset"

            validate_output_folder(asset_dir, overwrite=False)

            self.assertTrue(Path(temp).exists())


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: Run tests to verify they fail because `export.py` does not exist**

Run:

```powershell
python -m unittest tools.blender.pbr_bake_export.tests.test_export
```

Expected: `ERROR` with `ModuleNotFoundError` for `export`.

- [ ] **Step 3: Implement export helpers and Blender object preparation**

Create `tools/blender/pbr_bake_export/addon/export.py`:

```python
from __future__ import annotations

from pathlib import Path


class ExportPreflightError(RuntimeError):
    pass


def validate_output_folder(asset_dir: Path, overwrite: bool) -> None:
    if asset_dir.exists() and not overwrite:
        raise ExportPreflightError(f"asset folder already exists: {asset_dir}")
    asset_dir.parent.mkdir(parents=True, exist_ok=True)


def duplicate_for_export(source_obj, export_name: str):
    import bpy

    duplicate = source_obj.copy()
    duplicate.data = source_obj.data.copy()
    duplicate.name = export_name
    duplicate.data.name = f"{export_name}_mesh"
    bpy.context.collection.objects.link(duplicate)
    return duplicate


def ensure_uv_layer(obj) -> None:
    if obj.data.uv_layers:
        return
    import bpy

    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=1.15192, island_margin=0.02)
    bpy.ops.object.mode_set(mode="OBJECT")


def apply_low_poly_decimation(obj, ratio: float) -> None:
    import bpy

    modifier = obj.modifiers.new("ZH2_low_poly_decimate", "DECIMATE")
    modifier.ratio = max(0.01, min(1.0, ratio))
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.modifier_apply(modifier=modifier.name)


def export_glb(obj, glb_path: Path) -> None:
    import bpy

    glb_path.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.gltf(
        filepath=str(glb_path),
        export_format="GLB",
        use_selection=True,
        export_materials="EXPORT",
        export_apply=True,
    )


def remove_temp_object(obj) -> None:
    import bpy

    mesh = obj.data
    bpy.data.objects.remove(obj, do_unlink=True)
    if mesh.users == 0:
        bpy.data.meshes.remove(mesh)
```

- [ ] **Step 4: Run export tests and all pure Python tests**

Run:

```powershell
python -m unittest tools.blender.pbr_bake_export.tests.test_export
python -m unittest discover -s tools/blender/pbr_bake_export/tests -p "test_*.py"
```

Expected: all tests pass.

- [ ] **Step 5: Commit Task 5**

```powershell
git add tools/blender/pbr_bake_export
git commit -m "Add Blender GLB export helpers"
```

---

### Task 6: Preferences, Panel, And Add-on Registration

**Files:**
- Modify: `tools/blender/pbr_bake_export/addon/__init__.py`
- Create: `tools/blender/pbr_bake_export/addon/preferences.py`
- Create: `tools/blender/pbr_bake_export/addon/panel.py`

- [ ] **Step 1: Add registration code before Blender UI implementation**

Modify `tools/blender/pbr_bake_export/addon/__init__.py`:

```python
bl_info = {
    "name": "ZH2 PBR Bake Export",
    "author": "ZH2",
    "version": (0, 1, 0),
    "blender": (4, 0, 0),
    "location": "View3D > Sidebar > ZH2 Assets",
    "description": "Bake selected PBR modular assets and export GLB folders.",
    "category": "Import-Export",
}

from . import operators, panel, preferences


MODULES = (
    preferences,
    operators,
    panel,
)


def register():
    for module in MODULES:
        module.register()


def unregister():
    for module in reversed(MODULES):
        module.unregister()
```

- [ ] **Step 2: Create preferences and workflow setting classes**

Create `tools/blender/pbr_bake_export/addon/preferences.py`:

```python
from __future__ import annotations


def _resolution_items():
    return (
        ("4096", "4K", "Bake 4096 x 4096 textures"),
        ("8192", "8K", "Bake 8192 x 8192 textures"),
        ("128", "Test 128", "Bake 128 x 128 textures for development verification"),
    )


def register():
    import bpy

    class ZH2PBRBakeExportPreferences(bpy.types.AddonPreferences):
        bl_idname = __package__

        user_agent: bpy.props.StringProperty(
            name="Poly Haven User-Agent",
            default="ZH2-Blender-PBR-Bake-Export/0.1",
            description="Unique User-Agent sent to Poly Haven API requests",
        )
        cache_root: bpy.props.StringProperty(
            name="Poly Haven Cache Root",
            default="//assets/_polyhaven_cache",
            subtype="DIR_PATH",
        )

        def draw(self, context):
            layout = self.layout
            layout.prop(self, "user_agent")
            layout.prop(self, "cache_root")
            layout.label(text="Poly Haven API usage may require custom licensing for commercial API use.")

    class ZH2PBRBakeExportSettings(bpy.types.PropertyGroup):
        output_root: bpy.props.StringProperty(name="Output Root", default="//assets", subtype="DIR_PATH")
        texture_resolution: bpy.props.EnumProperty(name="Texture Resolution", items=_resolution_items(), default="4096")
        asset_mode: bpy.props.EnumProperty(
            name="Asset Mode",
            items=(("HIGH", "High Poly", "Export selected mesh detail"), ("LOW", "Low Poly", "Export low-poly target")),
            default="HIGH",
        )
        autogenerate_low_poly: bpy.props.BoolProperty(name="Autogenerate low-poly mesh", default=True)
        decimate_ratio: bpy.props.FloatProperty(name="Low-poly ratio", min=0.01, max=1.0, default=0.35)
        material_source: bpy.props.EnumProperty(
            name="Material Source",
            items=(("POLY_HAVEN", "Poly Haven", "Use downloaded Poly Haven material"), ("EXISTING", "Existing", "Use object material")),
            default="POLY_HAVEN",
        )
        polyhaven_query: bpy.props.StringProperty(name="Search", default="")
        selected_polyhaven_asset: bpy.props.StringProperty(name="Poly Haven Asset ID", default="")
        allow_resolution_fallback: bpy.props.BoolProperty(name="Allow lower-resolution fallback", default=False)
        overwrite_existing: bpy.props.BoolProperty(name="Overwrite existing exports", default=False)

    bpy.utils.register_class(ZH2PBRBakeExportPreferences)
    bpy.utils.register_class(ZH2PBRBakeExportSettings)
    bpy.types.Scene.zh2_pbr_bake_export = bpy.props.PointerProperty(type=ZH2PBRBakeExportSettings)


def unregister():
    import bpy

    if hasattr(bpy.types.Scene, "zh2_pbr_bake_export"):
        del bpy.types.Scene.zh2_pbr_bake_export
    settings_class = getattr(bpy.types, "ZH2PBRBakeExportSettings", None)
    preferences_class = getattr(bpy.types, "ZH2PBRBakeExportPreferences", None)
    if settings_class is not None:
        bpy.utils.unregister_class(settings_class)
    if preferences_class is not None:
        bpy.utils.unregister_class(preferences_class)
```

- [ ] **Step 3: Create the Blender sidebar panel**

Create `tools/blender/pbr_bake_export/addon/panel.py`:

```python
from __future__ import annotations


def register():
    import bpy

    class ZH2_PT_PBRBakeExportPanel(bpy.types.Panel):
        bl_label = "ZH2 PBR Bake Export"
        bl_idname = "ZH2_PT_pbr_bake_export"
        bl_space_type = "VIEW_3D"
        bl_region_type = "UI"
        bl_category = "ZH2 Assets"

        def draw(self, context):
            settings = context.scene.zh2_pbr_bake_export
            layout = self.layout
            layout.prop(settings, "output_root")
            layout.prop(settings, "texture_resolution")
            layout.prop(settings, "asset_mode")
            if settings.asset_mode == "LOW":
                layout.prop(settings, "autogenerate_low_poly")
                layout.prop(settings, "decimate_ratio")
            layout.separator()
            layout.prop(settings, "material_source")
            if settings.material_source == "POLY_HAVEN":
                layout.prop(settings, "polyhaven_query")
                layout.operator("zh2.polyhaven_search", icon="VIEWZOOM")
                layout.prop(settings, "selected_polyhaven_asset")
                layout.prop(settings, "allow_resolution_fallback")
                layout.operator("zh2.polyhaven_download", icon="IMPORT")
            layout.separator()
            layout.prop(settings, "overwrite_existing")
            layout.operator("zh2.bake_export_selected", icon="EXPORT")

    bpy.utils.register_class(ZH2_PT_PBRBakeExportPanel)


def unregister():
    import bpy

    panel_class = getattr(bpy.types, "ZH2_PT_PBRBakeExportPanel", None)
    if panel_class is not None:
        bpy.utils.unregister_class(panel_class)
```

- [ ] **Step 4: Create a temporary operators module with class shells**

Create `tools/blender/pbr_bake_export/addon/operators.py`:

```python
from __future__ import annotations


def register():
    import bpy

    class ZH2_OT_PolyhavenSearch(bpy.types.Operator):
        bl_idname = "zh2.polyhaven_search"
        bl_label = "Search Poly Haven"

        def execute(self, context):
            self.report({"INFO"}, "Poly Haven search operator is registered")
            return {"FINISHED"}

    class ZH2_OT_PolyhavenDownload(bpy.types.Operator):
        bl_idname = "zh2.polyhaven_download"
        bl_label = "Download Material"

        def execute(self, context):
            self.report({"INFO"}, "Poly Haven download operator is registered")
            return {"FINISHED"}

    class ZH2_OT_BakeExportSelected(bpy.types.Operator):
        bl_idname = "zh2.bake_export_selected"
        bl_label = "Bake + Export Selected"

        def execute(self, context):
            self.report({"INFO"}, "Bake/export operator is registered")
            return {"FINISHED"}

    bpy.utils.register_class(ZH2_OT_PolyhavenSearch)
    bpy.utils.register_class(ZH2_OT_PolyhavenDownload)
    bpy.utils.register_class(ZH2_OT_BakeExportSelected)


def unregister():
    import bpy

    for class_name in ("ZH2_OT_BakeExportSelected", "ZH2_OT_PolyhavenDownload", "ZH2_OT_PolyhavenSearch"):
        cls = getattr(bpy.types, class_name, None)
        if cls is not None:
            bpy.utils.unregister_class(cls)
```

- [ ] **Step 5: Run syntax checks**

Run:

```powershell
Get-ChildItem -Recurse tools/blender/pbr_bake_export -Filter *.py | ForEach-Object { python -m py_compile $_.FullName }
```

Expected: no output and exit code `0`.

- [ ] **Step 6: Register add-on in open Blender through MCP and inspect panel availability**

Run Blender Python through MCP:

```python
import sys
from pathlib import Path

repo = Path(r"C:\Users\Peter Farber\Documents\ZH2")
if str(repo) not in sys.path:
    sys.path.insert(0, str(repo))

import tools.blender.pbr_bake_export.addon as addon
addon.unregister()
addon.register()
print("ZH2 PBR Bake Export registered")
```

Expected: Blender console prints `ZH2 PBR Bake Export registered` and no exception is raised.

- [ ] **Step 7: Run all pure Python tests**

Run:

```powershell
python -m unittest discover -s tools/blender/pbr_bake_export/tests -p "test_*.py"
```

Expected: all tests pass.

- [ ] **Step 8: Commit Task 6**

```powershell
git add tools/blender/pbr_bake_export
git commit -m "Add Blender PBR bake export UI"
```

---

### Task 7: Operators And End-to-End Export Coordination

**Files:**
- Modify: `tools/blender/pbr_bake_export/addon/operators.py`

- [ ] **Step 1: Replace operator shells with real coordination code**

Modify `tools/blender/pbr_bake_export/addon/operators.py`:

```python
from __future__ import annotations

from datetime import datetime, timezone
from pathlib import Path

from .bake import bake_object_maps
from .export import (
    ExportPreflightError,
    apply_low_poly_decimation,
    duplicate_for_export,
    ensure_uv_layer,
    export_glb,
    remove_temp_object,
    validate_output_folder,
)
from .manifest import ExportManifest, write_manifest
from .materials import build_material_plan, create_blender_material
from .paths import build_asset_paths
from .polyhaven import PolyhavenClient, PolyhavenError, select_texture_files
from .types import AssetMode, MaterialSource, TextureResolution


def _resolution_from_settings(value: str) -> TextureResolution:
    if value == "8192":
        return TextureResolution.EIGHT_K
    if value == "128":
        return TextureResolution.TEST
    return TextureResolution.FOUR_K


def _abspath(blender_path: str) -> Path:
    import bpy

    return Path(bpy.path.abspath(blender_path))


def _addon_preferences(context):
    import bpy

    addon_name = __package__
    return context.preferences.addons[addon_name].preferences


def _selected_mesh_objects(context):
    return [obj for obj in context.selected_objects if getattr(obj, "type", "") == "MESH"]


class _ClassStore:
    classes = []


def register():
    import bpy

    class ZH2_OT_PolyhavenSearch(bpy.types.Operator):
        bl_idname = "zh2.polyhaven_search"
        bl_label = "Search Poly Haven"

        def execute(self, context):
            settings = context.scene.zh2_pbr_bake_export
            prefs = _addon_preferences(context)
            try:
                client = PolyhavenClient(user_agent=prefs.user_agent)
                results = client.search_textures(settings.polyhaven_query)
            except PolyhavenError as exc:
                self.report({"ERROR"}, str(exc))
                return {"CANCELLED"}
            if not results:
                self.report({"WARNING"}, "No Poly Haven texture assets matched the search")
                return {"CANCELLED"}
            settings.selected_polyhaven_asset = results[0].asset_id
            self.report({"INFO"}, f"Selected Poly Haven material: {results[0].name}")
            return {"FINISHED"}

    class ZH2_OT_PolyhavenDownload(bpy.types.Operator):
        bl_idname = "zh2.polyhaven_download"
        bl_label = "Download Material"

        def execute(self, context):
            settings = context.scene.zh2_pbr_bake_export
            prefs = _addon_preferences(context)
            if not settings.selected_polyhaven_asset.strip():
                self.report({"ERROR"}, "Select a Poly Haven asset before downloading")
                return {"CANCELLED"}
            try:
                resolution = _resolution_from_settings(settings.texture_resolution)
                client = PolyhavenClient(user_agent=prefs.user_agent)
                files = client.files_for_asset(settings.selected_polyhaven_asset)
                selection = select_texture_files(files, resolution, settings.allow_resolution_fallback)
                cache_dir = _abspath(prefs.cache_root) / settings.selected_polyhaven_asset
                client.download_selection(selection, cache_dir, settings.selected_polyhaven_asset)
            except PolyhavenError as exc:
                self.report({"ERROR"}, str(exc))
                return {"CANCELLED"}
            self.report({"INFO"}, f"Downloaded material: {settings.selected_polyhaven_asset}")
            return {"FINISHED"}

    class ZH2_OT_BakeExportSelected(bpy.types.Operator):
        bl_idname = "zh2.bake_export_selected"
        bl_label = "Bake + Export Selected"

        def execute(self, context):
            settings = context.scene.zh2_pbr_bake_export
            prefs = _addon_preferences(context)
            selected = _selected_mesh_objects(context)
            if not selected:
                self.report({"ERROR"}, "Select at least one mesh object")
                return {"CANCELLED"}

            resolution = _resolution_from_settings(settings.texture_resolution)
            output_root = _abspath(settings.output_root)
            mode = AssetMode.LOW_POLY if settings.asset_mode == "LOW" else AssetMode.HIGH_POLY
            material_source = MaterialSource.POLY_HAVEN if settings.material_source == "POLY_HAVEN" else MaterialSource.EXISTING

            for source_obj in selected:
                paths = build_asset_paths(output_root, source_obj.name)
                try:
                    validate_output_folder(paths.asset_dir, settings.overwrite_existing)
                except ExportPreflightError as exc:
                    self.report({"ERROR"}, str(exc))
                    return {"CANCELLED"}

                export_obj = None
                try:
                    paths.asset_dir.mkdir(parents=True, exist_ok=True)
                    paths.texture_dir.mkdir(parents=True, exist_ok=True)
                    export_obj = duplicate_for_export(source_obj, f"{paths.asset_name}_export")

                    if mode is AssetMode.LOW_POLY:
                        if not settings.autogenerate_low_poly:
                            self.report({"ERROR"}, "Low-poly mode requires Autogenerate low-poly mesh in v1")
                            return {"CANCELLED"}
                        apply_low_poly_decimation(export_obj, settings.decimate_ratio)

                    ensure_uv_layer(export_obj)

                    texture_paths = {}
                    polyhaven_id = ""
                    if material_source is MaterialSource.POLY_HAVEN:
                        polyhaven_id = settings.selected_polyhaven_asset.strip()
                        if not polyhaven_id:
                            self.report({"ERROR"}, "Select and download a Poly Haven material before export")
                            return {"CANCELLED"}
                        cache_dir = _abspath(prefs.cache_root) / polyhaven_id
                        texture_paths = {
                            role: cache_dir / f"{polyhaven_id}_{role}.png"
                            for role in ("basecolor", "normal", "roughness", "metallic", "ao")
                        }
                    material_plan = build_material_plan(
                        name=f"{paths.asset_name}_material",
                        source=material_source,
                        texture_paths=texture_paths,
                        polyhaven_asset_id=polyhaven_id,
                    )
                    export_obj.data.materials.clear()
                    export_obj.data.materials.append(create_blender_material(material_plan))

                    warnings = bake_object_maps(export_obj, paths.textures, resolution)
                    export_glb(export_obj, paths.glb_path)
                    manifest = ExportManifest(
                        asset_name=paths.asset_name,
                        source_object=source_obj.name,
                        exported_at=datetime.now(timezone.utc).astimezone().isoformat(timespec="seconds"),
                        mode=mode,
                        autogenerated_low_poly=mode is AssetMode.LOW_POLY and settings.autogenerate_low_poly,
                        resolution=resolution,
                        material_source=material_source,
                        polyhaven_asset_id=polyhaven_id,
                        glb=paths.glb_path.name,
                        textures={role: str(path.relative_to(paths.asset_dir)).replace("\\", "/") for role, path in paths.textures.items()},
                        warnings=warnings,
                    )
                    write_manifest(paths.manifest_path, manifest)
                    self.report({"INFO"}, f"Exported {paths.glb_path}")
                except Exception as exc:
                    self.report({"ERROR"}, f"Export failed for {source_obj.name}: {exc}")
                    return {"CANCELLED"}
                finally:
                    if export_obj is not None:
                        remove_temp_object(export_obj)

            return {"FINISHED"}

    _ClassStore.classes = [
        ZH2_OT_PolyhavenSearch,
        ZH2_OT_PolyhavenDownload,
        ZH2_OT_BakeExportSelected,
    ]
    for cls in _ClassStore.classes:
        bpy.utils.register_class(cls)


def unregister():
    import bpy

    for cls in reversed(_ClassStore.classes):
        bpy.utils.unregister_class(cls)
    _ClassStore.classes = []
```

- [ ] **Step 2: Run syntax checks**

Run:

```powershell
Get-ChildItem -Recurse tools/blender/pbr_bake_export -Filter *.py | ForEach-Object { python -m py_compile $_.FullName }
```

Expected: no output and exit code `0`.

- [ ] **Step 3: Run all pure Python tests**

Run:

```powershell
python -m unittest discover -s tools/blender/pbr_bake_export/tests -p "test_*.py"
```

Expected: all tests pass.

- [ ] **Step 4: Run Blender registration smoke through MCP**

Run in Blender through MCP:

```python
import sys
from pathlib import Path

repo = Path(r"C:\Users\Peter Farber\Documents\ZH2")
if str(repo) not in sys.path:
    sys.path.insert(0, str(repo))

import tools.blender.pbr_bake_export.addon as addon
try:
    addon.unregister()
except Exception:
    pass
addon.register()
print("registered", hasattr(__import__("bpy").types.Scene, "zh2_pbr_bake_export"))
```

Expected: console prints `registered True`.

- [ ] **Step 5: Commit Task 7**

```powershell
git add tools/blender/pbr_bake_export
git commit -m "Wire Blender PBR bake export operators"
```

---

### Task 8: Blender Smoke Export And Documentation

**Files:**
- Create: `tools/blender/pbr_bake_export/README.md`

- [ ] **Step 1: Add documentation**

Create `tools/blender/pbr_bake_export/README.md`:

```markdown
# ZH2 PBR Bake Export Blender Add-on

This add-on bakes selected Blender mesh objects into modular PBR asset folders.

## Install For Development

1. Open Blender.
2. Add `C:\Users\Peter Farber\Documents\ZH2` to `sys.path` in a Blender Python console or startup script.
3. Import and register `tools.blender.pbr_bake_export.addon`.

## Output

Each selected object exports to:

```text
assets/<object_name>/
  <object_name>.glb
  textures/
    <object_name>_basecolor.png
    <object_name>_normal.png
    <object_name>_roughness.png
    <object_name>_metallic.png
    <object_name>_ao.png
  manifest.json
```

## Poly Haven API Notice

The Poly Haven API requires a unique User-Agent header. Configure it in the add-on preferences. Poly Haven notes that commercial API usage requires a custom license and typically sponsorship.

## Manual Import

This add-on does not call `HockeyAssetTool`. Move or import exported GLBs manually when they are ready for the engine asset pipeline.
```

- [ ] **Step 2: Run a no-network Blender smoke export using existing material**

Run in Blender through MCP:

```python
import sys
from pathlib import Path

repo = Path(r"C:\Users\Peter Farber\Documents\ZH2")
if str(repo) not in sys.path:
    sys.path.insert(0, str(repo))

import bpy
import tools.blender.pbr_bake_export.addon as addon

try:
    addon.unregister()
except Exception:
    pass
addon.register()

bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 0, 0))
cube = bpy.context.object
cube.name = "Smoke Export Cube"
material = bpy.data.materials.new("Smoke Material")
material.use_nodes = True
cube.data.materials.append(material)

settings = bpy.context.scene.zh2_pbr_bake_export
settings.output_root = str(repo / "out" / "blender_pbr_smoke_assets")
settings.texture_resolution = "128"
settings.asset_mode = "HIGH"
settings.material_source = "EXISTING"
settings.overwrite_existing = True

bpy.ops.object.select_all(action="DESELECT")
cube.select_set(True)
bpy.context.view_layer.objects.active = cube

result = bpy.ops.zh2.bake_export_selected()
print("smoke result", result)
```

Expected: operator returns `{'FINISHED'}`. Confirm these files exist:

```text
out/blender_pbr_smoke_assets/smoke_export_cube/smoke_export_cube.glb
out/blender_pbr_smoke_assets/smoke_export_cube/textures/smoke_export_cube_basecolor.png
out/blender_pbr_smoke_assets/smoke_export_cube/manifest.json
```

- [ ] **Step 3: Run syntax checks and unit tests**

Run:

```powershell
Get-ChildItem -Recurse tools/blender/pbr_bake_export -Filter *.py | ForEach-Object { python -m py_compile $_.FullName }
python -m unittest discover -s tools/blender/pbr_bake_export/tests -p "test_*.py"
```

Expected: syntax checks succeed and all tests pass.

- [ ] **Step 4: Run final status and phase-status check**

Run:

```powershell
git status --short
git diff -- docs/phase-status/phase-05-asset-pipeline.md
```

Expected: only intentional add-on files are changed for this work. `docs/phase-status/phase-05-asset-pipeline.md` has no diff because the add-on is authoring tooling and does not change Phase 5 engine status.

- [ ] **Step 5: Commit Task 8**

```powershell
git add tools/blender/pbr_bake_export
git commit -m "Document Blender PBR bake export add-on"
```

---

## Final Verification

Run:

```powershell
Get-ChildItem -Recurse tools/blender/pbr_bake_export -Filter *.py | ForEach-Object { python -m py_compile $_.FullName }
python -m unittest discover -s tools/blender/pbr_bake_export/tests -p "test_*.py"
```

Run the Blender smoke export from Task 8 Step 2.

Manual verification before relying on production exports:

```text
1. Search for a Poly Haven texture in the panel.
2. Download the selected material at 4K.
3. Select one simple mesh and run High Poly export.
4. Select one mesh and run Low Poly export with Autogenerate low-poly mesh enabled.
5. Inspect GLB and external textures under assets/<object_name>/.
6. Run an 8K export only after confirming local memory headroom.
```

Phase status expectation:

```text
No phase status change needed.
```

The add-on creates authoring-side exports and does not alter the Phase 5 engine asset pipeline state.
