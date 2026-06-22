"""Shared dataclasses and enums for the PBR bake/export addon."""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum
from pathlib import Path


class TextureResolution(Enum):
    FOUR_K = ("4K", 4096)
    EIGHT_K = ("8K", 8192)
    TEST = ("Test 128", 128)

    def __new__(cls, label: str, pixel_size: int):
        value = object.__new__(cls)
        value._value_ = label
        value.pixel_size = pixel_size
        return value


class AssetMode(Enum):
    HIGH_POLY = "High Poly"
    LOW_POLY = "Low Poly"


class MaterialSource(Enum):
    BLENDER = "Blender"
    POLY_HAVEN = "Poly Haven"


@dataclass(frozen=True)
class AssetPaths:
    asset_name: str
    asset_dir: Path
    glb_path: Path
    texture_dir: Path
    textures: dict[str, Path]
    manifest_path: Path
