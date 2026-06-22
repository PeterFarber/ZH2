"""Path planning helpers for Blender PBR exports."""

from __future__ import annotations

import re
import unicodedata
from pathlib import Path

from .types import AssetPaths


TEXTURE_ROLES = ("basecolor", "normal", "roughness", "metallic", "ao")


def sanitize_asset_name(name: str) -> str:
    ascii_name = unicodedata.normalize("NFKD", name).encode("ascii", "ignore").decode("ascii")
    safe_name = re.sub(r"[^a-z0-9]+", "_", ascii_name.lower()).strip("_")
    return safe_name or "asset"


def build_asset_paths(output_root: Path, object_name: str) -> AssetPaths:
    asset_name = sanitize_asset_name(object_name)
    asset_dir = Path(output_root) / asset_name
    texture_dir = asset_dir / "textures"
    textures = {
        role: texture_dir / f"{asset_name}_{role}.png"
        for role in TEXTURE_ROLES
    }

    return AssetPaths(
        asset_name=asset_name,
        asset_dir=asset_dir,
        glb_path=asset_dir / f"{asset_name}.glb",
        texture_dir=texture_dir,
        textures=textures,
        manifest_path=asset_dir / "manifest.json",
    )
