"""Path planning helpers for Blender PBR exports."""

from __future__ import annotations

import re
import unicodedata
from pathlib import Path

from .types import AssetPaths


TEXTURE_ROLES = ("basecolor", "normal", "roughness", "metallic", "ao")
WINDOWS_RESERVED_NAMES = {
    "con",
    "prn",
    "aux",
    "nul",
    *(f"com{index}" for index in range(1, 10)),
    *(f"lpt{index}" for index in range(1, 10)),
}


def sanitize_asset_name(name: str) -> str:
    ascii_name = unicodedata.normalize("NFKD", name).encode("ascii", "ignore").decode("ascii")
    safe_name = re.sub(r"[^a-z0-9]+", "_", ascii_name.lower()).strip("_")
    safe_name = safe_name or "asset"
    if safe_name in WINDOWS_RESERVED_NAMES:
        return f"asset_{safe_name}"
    return safe_name


def _occupied_asset_names(output_root: Path, reserved_names: set[str] | None) -> set[str]:
    occupied = {name.lower() for name in reserved_names or set()}
    root = Path(output_root)
    if root.exists():
        occupied.update(child.name.lower() for child in root.iterdir() if child.is_dir())
    return occupied


def _deduplicate_asset_name(asset_name: str, occupied_names: set[str]) -> str:
    candidate = asset_name
    suffix = 2
    while candidate.lower() in occupied_names:
        candidate = f"{asset_name}_{suffix}"
        suffix += 1
    return candidate


def build_asset_paths(output_root: Path, object_name: str, reserved_names: set[str] | None = None) -> AssetPaths:
    asset_name = _deduplicate_asset_name(sanitize_asset_name(object_name), _occupied_asset_names(output_root, reserved_names))
    asset_dir = Path(output_root) / asset_name
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
