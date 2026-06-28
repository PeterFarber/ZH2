#!/usr/bin/env bash
set -euo pipefail

preset="linux-release"
output_dir="out/packages/server"
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
package_path="${build_dir}/packages/server_runtime.hkpack"

if [[ ! -f "${build_dir}/CMakeCache.txt" ]]; then
    cmake --preset "${preset}"
fi

cmake --build --preset "${build_preset}" --target HockeyAssetTool
"${build_dir}/apps/asset_tool/HockeyAssetTool" --root "${root}" package-runtime --target server --config data/config/server.toml --output "${package_path}"
cmake --build --preset "${build_preset}" --target HockeyDedicatedServer

mkdir -p "${output_dir}"
find "${output_dir}" -mindepth 1 -maxdepth 1 -exec rm -rf {} +
cp "${build_dir}/apps/dedicated_server/HockeyDedicatedServer" "${output_dir}/HockeyDedicatedServer"
cp "${root}/data/config/server.toml" "${output_dir}/HockeyDedicatedServer.toml"

if [[ "${include_debug_symbols}" -eq 1 && -f "${build_dir}/apps/dedicated_server/HockeyDedicatedServer.debug" ]]; then
    cp "${build_dir}/apps/dedicated_server/HockeyDedicatedServer.debug" "${output_dir}/HockeyDedicatedServer.debug"
fi

if [[ -d "${output_dir}/data" ]]; then
    echo "Package output must not contain a data directory: ${output_dir}" >&2
    exit 1
fi

echo "Packaged server (${build_mode}) to ${output_dir}"
