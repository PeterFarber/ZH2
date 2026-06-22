"""Poly Haven API helpers for texture material discovery and download."""

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

    return TextureFileSelection(
        resolution=resolution,
        urls={role: candidates[role] for role in REQUIRED_ROLES},
        warnings=warnings,
    )


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
