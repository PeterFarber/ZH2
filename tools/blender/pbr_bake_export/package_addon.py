"""Build a Blender-installable zip for the ZH2 PBR bake/export add-on."""

from __future__ import annotations

import argparse
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile


PACKAGE_NAME = "zh2_pbr_bake_export"
SCRIPT_DIR = Path(__file__).resolve().parent
DEFAULT_SOURCE_DIR = SCRIPT_DIR / "addon"
DEFAULT_OUTPUT_PATH = SCRIPT_DIR.parents[2] / "out" / "blender" / f"{PACKAGE_NAME}.zip"
_SKIPPED_SUFFIXES = {".pyc", ".pyo"}


def iter_addon_files(source_dir: Path) -> list[Path]:
    files: list[Path] = []
    for path in sorted(source_dir.rglob("*")):
        if not path.is_file():
            continue
        if "__pycache__" in path.parts:
            continue
        if path.suffix.lower() in _SKIPPED_SUFFIXES:
            continue
        files.append(path)
    return files


def build_addon_zip(
    output_path: Path,
    source_dir: Path = DEFAULT_SOURCE_DIR,
    package_name: str = PACKAGE_NAME,
) -> Path:
    output_path = output_path.resolve()
    source_dir = source_dir.resolve()
    output_path.parent.mkdir(parents=True, exist_ok=True)

    with ZipFile(output_path, "w", compression=ZIP_DEFLATED) as archive:
        for path in iter_addon_files(source_dir):
            relative_path = path.relative_to(source_dir)
            archive.write(path, f"{package_name}/{relative_path.as_posix()}")

    return output_path


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT_PATH,
        help=f"Zip path to write. Defaults to {DEFAULT_OUTPUT_PATH}",
    )
    args = parser.parse_args(argv)

    output_path = build_addon_zip(args.output)
    print(output_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
