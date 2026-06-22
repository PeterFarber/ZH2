"""Poly Haven API helpers for texture material discovery and download."""

from __future__ import annotations

import json
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Iterator
from urllib.parse import quote, urlencode, urlsplit
from urllib.request import Request, urlopen

from .types import TextureResolution


API_ROOT = "https://api.polyhaven.com"
REQUIRED_ROLES = ("basecolor", "normal", "roughness", "metallic", "ao")
_DEFAULT_TIMEOUT_SECONDS = 30.0
_DOWNLOAD_URL_SCHEMES = {"http", "https"}
_ROLE_TOKEN_SEQUENCES = (
    ("basecolor", ("diff",)),
    ("basecolor", ("diffuse",)),
    ("basecolor", ("albedo",)),
    ("basecolor", ("basecolor",)),
    ("basecolor", ("base", "color")),
    ("basecolor", ("col",)),
    ("normal", ("nor",)),
    ("normal", ("normal",)),
    ("normal", ("nrm",)),
    ("roughness", ("rough",)),
    ("roughness", ("roughness",)),
    ("metallic", ("metal",)),
    ("metallic", ("metallic",)),
    ("metallic", ("metalness",)),
    ("ao", ("ao",)),
    ("ao", ("ambient", "occlusion")),
    ("ao", ("ambientocclusion",)),
)
_FORMAT_PREFERENCE = {"png": 0, "jpg": 1, "jpeg": 2}


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


@dataclass(frozen=True)
class _TextureCandidate:
    resolution_label: str
    file_format: str
    url: str
    labels: tuple[str, ...]


def _tokens(value: str) -> list[str]:
    return [token for token in re.split(r"[^a-z0-9]+", value.lower()) if token]


def match_texture_role(filename: str) -> str:
    tokens = _tokens(filename)
    best_match: tuple[int, int, str] | None = None
    for role, sequence in _ROLE_TOKEN_SEQUENCES:
        sequence_length = len(sequence)
        for index in range(0, len(tokens) - sequence_length + 1):
            if tuple(tokens[index : index + sequence_length]) != sequence:
                continue
            candidate = (index, sequence_length, role)
            if best_match is None or candidate[:2] > best_match[:2]:
                best_match = candidate
    return best_match[2] if best_match else ""


def _is_resolution_token(token: str) -> bool:
    return token == "test" or bool(re.fullmatch(r"\d+k", token))


def _record_resolution(labels: tuple[str, ...]) -> str:
    for label in labels[:-1]:
        for token in _tokens(label):
            if _is_resolution_token(token):
                return token
    if labels:
        for token in _tokens(labels[-1]):
            if _is_resolution_token(token):
                return token
    return ""


def _record_format(labels: tuple[str, ...]) -> str:
    if labels:
        suffix = Path(labels[-1]).suffix.lower().lstrip(".")
        if suffix:
            return suffix
    for label in labels:
        for token in _tokens(label):
            if token in _FORMAT_PREFERENCE:
                return token
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


def _requested_candidate_key(candidate: _TextureCandidate) -> tuple[int, str, str]:
    labels = "/".join(candidate.labels)
    return (_FORMAT_PREFERENCE.get(candidate.file_format, 99), labels, candidate.url)


def _fallback_resolution_key(resolution_label: str) -> tuple[int, int | str]:
    if resolution_label == "4k":
        return (0, 0)
    if resolution_label == "8k":
        return (1, 0)
    match = re.fullmatch(r"(\d+)k", resolution_label)
    if match:
        return (2, -int(match.group(1)))
    if resolution_label == "test":
        return (3, 0)
    return (4, resolution_label)


def _fallback_candidate_key(candidate: _TextureCandidate) -> tuple[tuple[int, int | str], int, str, str]:
    labels = "/".join(candidate.labels)
    return (
        _fallback_resolution_key(candidate.resolution_label),
        _FORMAT_PREFERENCE.get(candidate.file_format, 99),
        labels,
        candidate.url,
    )


def select_texture_files(
    file_tree: object,
    resolution: TextureResolution,
    allow_resolution_fallback: bool,
) -> TextureFileSelection:
    requested_label = _resolution_label(resolution)
    requested_candidates: dict[str, list[_TextureCandidate]] = {}
    fallback_candidates: dict[str, list[_TextureCandidate]] = {}

    for labels, record in _iter_file_records(file_tree):
        url = str(record["url"])
        role = match_texture_role(labels[-1] if labels else "")
        if not role:
            continue
        candidate = _TextureCandidate(
            resolution_label=_record_resolution(labels),
            file_format=_record_format(labels),
            url=url,
            labels=labels,
        )
        fallback_candidates.setdefault(role, []).append(candidate)
        if candidate.resolution_label == requested_label:
            requested_candidates.setdefault(role, []).append(candidate)

    selected: dict[str, _TextureCandidate] = {}
    for role in REQUIRED_ROLES:
        if role in requested_candidates:
            selected[role] = min(requested_candidates[role], key=_requested_candidate_key)

    missing = [role for role in REQUIRED_ROLES if role not in selected]
    warnings: list[str] = []
    if missing and allow_resolution_fallback:
        for role in missing:
            if role in fallback_candidates:
                selected[role] = min(fallback_candidates[role], key=_fallback_candidate_key)
                warnings.append(f"{role} used available fallback resolution")
        missing = [role for role in REQUIRED_ROLES if role not in selected]

    if missing:
        joined_missing = ", ".join(missing)
        raise PolyhavenError(f"missing required texture roles: {joined_missing}")

    return TextureFileSelection(
        resolution=resolution,
        urls={role: selected[role].url for role in REQUIRED_ROLES},
        warnings=warnings,
    )


def _validate_download_label(label: str, label_name: str) -> str:
    clean = label.strip()
    if not clean:
        raise PolyhavenError(f"{label_name} cannot be empty")
    if "/" in clean or "\\" in clean:
        raise PolyhavenError(f"{label_name} must not contain path separators")
    if clean in (".", "..") or ":" in clean:
        raise PolyhavenError(f"{label_name} must be a simple file name segment")
    return clean


def _safe_download_path(cache_dir: Path, cache_root: Path, prefix: str, role: str) -> Path:
    safe_prefix = _validate_download_label(prefix, "download prefix")
    safe_role = _validate_download_label(role, "texture role")
    path = cache_dir / f"{safe_prefix}_{safe_role}.png"
    resolved_path = path.resolve()
    try:
        resolved_path.relative_to(cache_root)
    except ValueError as exc:
        raise PolyhavenError("download path escaped the Poly Haven cache directory") from exc
    return path


def _validate_download_url(role: str, url: str) -> str:
    clean = url.strip()
    try:
        parts = urlsplit(clean)
    except ValueError as exc:
        raise PolyhavenError(f"invalid download URL for texture role {role}: {exc}") from exc
    if parts.scheme not in _DOWNLOAD_URL_SCHEMES:
        raise PolyhavenError(f"invalid download URL for texture role {role}: unsupported scheme")
    if not parts.netloc:
        raise PolyhavenError(f"invalid download URL for texture role {role}: missing host")
    return clean


class PolyhavenClient:
    def __init__(
        self,
        user_agent: str,
        fetcher: Callable[..., object] | None = None,
        timeout_seconds: float = _DEFAULT_TIMEOUT_SECONDS,
    ):
        if not user_agent.strip():
            raise PolyhavenError("Poly Haven requests require a unique User-Agent")
        if timeout_seconds <= 0:
            raise PolyhavenError("Poly Haven request timeout must be positive")
        self.user_agent = user_agent.strip()
        self.fetcher = fetcher or urlopen
        self.timeout_seconds = timeout_seconds

    def _request_json(self, path: str, query: dict[str, str] | None = None) -> object:
        url = f"{API_ROOT}{path}"
        if query:
            url = f"{url}?{urlencode(query)}"
        request = Request(url, headers={"User-Agent": self.user_agent})
        try:
            with self.fetcher(request, timeout=self.timeout_seconds) as response:
                payload = response.read()
        except Exception as exc:
            raise PolyhavenError(f"Poly Haven request failed for {path}: {exc}") from exc
        try:
            return json.loads(payload.decode("utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError) as exc:
            raise PolyhavenError(f"Poly Haven returned invalid JSON for {path}: {exc}") from exc

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
        return self._request_json(f"/files/{quote(asset_id, safe='')}")

    def download_selection(self, selection: TextureFileSelection, cache_dir: Path, prefix: str) -> dict[str, Path]:
        cache_dir.mkdir(parents=True, exist_ok=True)
        cache_root = cache_dir.resolve()
        paths = {role: _safe_download_path(cache_dir, cache_root, prefix, role) for role in selection.urls}
        urls = {role: _validate_download_url(role, url) for role, url in selection.urls.items()}
        written: dict[str, Path] = {}
        for role, url in urls.items():
            path = paths[role]
            try:
                request = Request(url, headers={"User-Agent": self.user_agent})
                with self.fetcher(request, timeout=self.timeout_seconds) as response:
                    path.write_bytes(response.read())
            except Exception as exc:
                raise PolyhavenError(f"failed to download texture role {role} from {url}: {exc}") from exc
            written[role] = path
        return written
