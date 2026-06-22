"""Poly Haven API helpers for texture material discovery and download."""

from __future__ import annotations

import json
import math
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Iterator
from urllib.parse import quote, unquote, urlencode, urlsplit
from urllib.request import Request, urlopen

from .types import TextureResolution


API_ROOT = "https://api.polyhaven.com"
REQUIRED_ROLES = ("basecolor", "normal", "roughness", "metallic", "ao")
_DEFAULT_TIMEOUT_SECONDS = 30.0
_DOWNLOAD_URL_SCHEMES = {"http", "https"}
_DOWNLOAD_FORMAT_TOKENS = {"png", "jpg", "jpeg"}
_NORMAL_BASE_TOKENS = {"nor", "normal", "nrm"}
_NORMAL_GL_TOKENS = {"gl", "opengl"}
_NORMAL_DX_TOKENS = {"dx", "directx"}
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
    normal_preference: int


def _tokens(value: str) -> list[str]:
    return [token for token in re.split(r"[^a-z0-9]+", value.lower()) if token]


def _is_resolution_token(token: str) -> bool:
    return token == "test" or bool(re.fullmatch(r"\d+k", token))


def _filename_role_tokens(filename: str) -> list[str]:
    tokens = _tokens(Path(filename).name)
    while tokens and (
        tokens[-1].isdigit() or _is_resolution_token(tokens[-1]) or tokens[-1] in _DOWNLOAD_FORMAT_TOKENS
    ):
        tokens.pop()
    return tokens


def _tokens_end_with(tokens: list[str], sequence: tuple[str, ...]) -> bool:
    sequence_length = len(sequence)
    return len(tokens) >= sequence_length and tuple(tokens[-sequence_length:]) == sequence


def match_texture_role(filename: str) -> str:
    tokens = _filename_role_tokens(filename)
    normal_direction_tokens = _NORMAL_GL_TOKENS | _NORMAL_DX_TOKENS
    for role, sequence in _ROLE_TOKEN_SEQUENCES:
        if (
            role == "normal"
            and tokens
            and tokens[-1] in normal_direction_tokens
            and _tokens_end_with(tokens[:-1], sequence)
        ):
            return role
        if _tokens_end_with(tokens, sequence):
            return role
    return ""


def _is_container_label(label: str) -> bool:
    tokens = _tokens(label)
    return not tokens or all(token.isdigit() or _is_resolution_token(token) or token in _DOWNLOAD_FORMAT_TOKENS for token in tokens)


def _role_label_tokens(label: str) -> list[str]:
    return [
        token
        for token in _tokens(label)
        if not token.isdigit() and not _is_resolution_token(token) and token not in _DOWNLOAD_FORMAT_TOKENS
    ]


def _match_role_label(label: str) -> str:
    tokens = _role_label_tokens(label)
    for role, sequence in _ROLE_TOKEN_SEQUENCES:
        sequence_length = len(sequence)
        if tuple(tokens[:sequence_length]) != sequence:
            continue
        remaining = tokens[sequence_length:]
        if not remaining or (role == "normal" and all(token in _NORMAL_GL_TOKENS | _NORMAL_DX_TOKENS for token in remaining)):
            return role
    return ""


def _url_basename(url: str) -> str:
    try:
        path = urlsplit(url).path
    except ValueError:
        return ""
    return Path(unquote(path)).name


def _record_role(labels: tuple[str, ...], url: str) -> str:
    if labels and not _is_container_label(labels[-1]):
        if Path(labels[-1]).suffix:
            role = match_texture_role(labels[-1])
        else:
            role = _match_role_label(labels[-1])
        if role:
            return role

    for label in reversed(labels[:-1]):
        if _is_container_label(label):
            continue
        role = _match_role_label(label)
        if role:
            return role

    basename = _url_basename(url)
    if basename and not _is_container_label(basename):
        return match_texture_role(basename)
    return ""


def _normal_preference(labels: tuple[str, ...], url: str) -> int:
    tokens: list[str] = []
    for label in labels:
        if not _is_container_label(label):
            tokens.extend(_role_label_tokens(label))
    basename = _url_basename(url)
    if basename and not _is_container_label(basename):
        tokens.extend(_role_label_tokens(basename))

    preference = 1
    for index, token in enumerate(tokens[:-1]):
        if token not in _NORMAL_BASE_TOKENS:
            continue
        next_token = tokens[index + 1]
        if next_token in _NORMAL_GL_TOKENS:
            return 0
        if next_token in _NORMAL_DX_TOKENS:
            preference = 2
    return preference


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


def _record_format(labels: tuple[str, ...], url: str) -> str:
    if labels:
        suffix = Path(labels[-1]).suffix.lower().lstrip(".")
        if suffix:
            return suffix
    for label in labels:
        for token in _tokens(label):
            if token in _FORMAT_PREFERENCE:
                return token
    basename = _url_basename(url)
    if basename:
        suffix = Path(basename).suffix.lower().lstrip(".")
        if suffix:
            return suffix
        for token in _tokens(basename):
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
    return (candidate.normal_preference, labels, candidate.url)


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
        candidate.normal_preference,
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
        file_format = _record_format(labels, url)
        if file_format != "png":
            continue
        role = _record_role(labels, url)
        if not role:
            continue
        candidate = _TextureCandidate(
            resolution_label=_record_resolution(labels),
            file_format=file_format,
            url=url,
            labels=labels,
            normal_preference=_normal_preference(labels, url),
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
    return resolved_path


def _contains_whitespace_or_control(value: str) -> bool:
    return any(character.isspace() or ord(character) < 32 or ord(character) == 127 for character in value)


def _raw_url_authority(url: str) -> str:
    scheme_separator = url.find("://")
    if scheme_separator < 0:
        return ""
    return re.split(r"[/?#]", url[scheme_separator + 3 :], maxsplit=1)[0]


def _validate_download_url(role: str, url: str) -> str:
    clean = url.strip()
    raw_authority = _raw_url_authority(clean)
    try:
        parts = urlsplit(clean)
    except ValueError as exc:
        raise PolyhavenError(f"invalid download URL for texture role {role}: {exc}") from exc
    if parts.scheme not in _DOWNLOAD_URL_SCHEMES:
        raise PolyhavenError(f"invalid download URL for texture role {role}: unsupported scheme")
    if "\\" in raw_authority:
        raise PolyhavenError(f"invalid download URL for texture role {role}: invalid authority")
    if parts.username is not None or parts.password is not None or "@" in raw_authority:
        raise PolyhavenError(f"invalid download URL for texture role {role}: userinfo not allowed")
    try:
        hostname = parts.hostname
        _ = parts.port
    except ValueError as exc:
        raise PolyhavenError(f"invalid download URL for texture role {role}: {exc}") from exc
    if not hostname or hostname.strip() != hostname:
        raise PolyhavenError(f"invalid download URL for texture role {role}: missing or blank host")
    if "%" in hostname:
        raise PolyhavenError(f"invalid download URL for texture role {role}: invalid host")
    decoded_hostname = unquote(hostname)
    if (
        not decoded_hostname
        or decoded_hostname.strip() != decoded_hostname
        or _contains_whitespace_or_control(hostname)
        or _contains_whitespace_or_control(decoded_hostname)
        or _contains_whitespace_or_control(raw_authority)
    ):
        raise PolyhavenError(f"invalid download URL for texture role {role}: invalid host")
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
        if not math.isfinite(timeout_seconds) or timeout_seconds <= 0:
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
        cache_root = cache_dir.resolve()
        cache_root.mkdir(parents=True, exist_ok=True)
        paths = {role: _safe_download_path(cache_root, cache_root, prefix, role) for role in selection.urls}
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
