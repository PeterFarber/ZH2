#!/usr/bin/env bash
set -euo pipefail

preset="linux-release"
output_dir="out/packages/client"
build_mode="Release"
include_debug_symbols=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --preset) preset="$2"; shift 2 ;;
        --output-dir) output_dir="$2"; shift 2 ;;
        --build-mode) build_mode="$2"; shift 2 ;;
        --include-debug-symbols) include_debug_symbols=1; shift ;;
        *) echo "unknown argument: $1" >&2; exit 1 ;;
    esac
done

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
build_preset="build-${preset}"
build_dir="${root}/build/linux-release"
if [[ "${preset}" != "linux-release" ]]; then
    build_dir="${root}/build/${preset/linux-/linux-}"
fi
if [[ "${output_dir}" != /* ]]; then
    output_dir="${root}/${output_dir}"
fi
package_path="${build_dir}/packages/client_runtime.hkpack"
app_dir="${build_dir}/apps/game_client"

if [[ ! -f "${build_dir}/CMakeCache.txt" ]]; then
    cmake --preset "${preset}"
fi

cmake --build --preset "${build_preset}" --target HockeyAssetTool
"${build_dir}/apps/asset_tool/HockeyAssetTool" --root "${root}" package-runtime --target client --config data/config/editor.toml --output "${package_path}"
cmake --build --preset "${build_preset}" --target HockeyGameClient

mkdir -p "${output_dir}"
find "${output_dir}" -mindepth 1 -maxdepth 1 -exec rm -rf {} +
cp "${build_dir}/apps/game_client/HockeyGameClient" "${output_dir}/HockeyGameClient"
find "${app_dir}" -maxdepth 1 -type f -name '*.so*' -exec cp -P {} "${output_dir}/" \;
cp "${root}/data/config/editor.toml" "${output_dir}/HockeyGameClient.toml"

if [[ "${include_debug_symbols}" -eq 1 && -f "${build_dir}/apps/game_client/HockeyGameClient.debug" ]]; then
    cp "${build_dir}/apps/game_client/HockeyGameClient.debug" "${output_dir}/HockeyGameClient.debug"
fi

if [[ -d "${output_dir}/data" ]]; then
    echo "Package output must not contain a data directory: ${output_dir}" >&2
    exit 1
fi

echo "Packaged client (${build_mode}) to ${output_dir}"
