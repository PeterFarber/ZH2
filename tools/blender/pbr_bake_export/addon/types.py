"""Shared dataclasses and enums for the PBR bake/export addon."""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum
from pathlib import Path


class TextureResolution(Enum):
    FOUR_K = ("4K", 4096)
    EIGHT_K = ("8K", 8192)
    TEST = ("Test 128", 128)

    def __init__(self, label: str, pixel_size: int):
        self._label = label
        self._pixel_size = pixel_size

    @property
    def label(self) -> str:
        return self._label

    @property
    def pixel_size(self) -> int:
        return self._pixel_size


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
