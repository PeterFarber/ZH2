#!/usr/bin/env bash
set -uo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
build_dir="$root/build/linux-debug"
failed=0

tests=(
    "apps/core_tests/HockeyCoreTests"
    "apps/ecs_tests/HockeyECSTests"
    "apps/asset_tests/HockeyAssetTests"
    "apps/renderer_tests/HockeyRendererTests"
    "apps/editor_tests/HockeyEditorTests"
    "apps/physics_tests/HockeyPhysicsTests"
    "apps/gameplay_tests/HockeyGameplayTests"
    "apps/game_client_tests/HockeyGameClientTests"
)

for relative_path in "${tests[@]}"; do
    exe="$build_dir/$relative_path"
    if [[ ! -x "$exe" ]]; then
        echo "SKIP  $relative_path (not built yet)" >&2
        failed=$((failed + 1))
        continue
    fi

    echo
    echo "=== $relative_path ==="
    "$exe" --root "$root"
    exit_code=$?
    if [[ $exit_code -ne 0 ]]; then
        echo "FAILED (exit $exit_code)" >&2
        failed=$((failed + 1))
    fi
done

if [[ $failed -gt 0 ]]; then
    echo "$failed test executable(s) missing or failed." >&2
    exit 1
fi
