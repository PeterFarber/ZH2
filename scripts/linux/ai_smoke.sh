#!/usr/bin/env bash
set -uo pipefail

mode="Text"
frames=5
ticks=300
scene="data/raw/scenes/main_rink.scene.yaml"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --mode|-Mode|-m)
            mode="$2"
            shift 2
            ;;
        --frames|-Frames)
            frames="$2"
            shift 2
            ;;
        --ticks|-Ticks)
            ticks="$2"
            shift 2
            ;;
        --scene|-Scene)
            scene="$2"
            shift 2
            ;;
        --help|-h)
            cat <<'USAGE'
Usage: scripts/linux/ai_smoke.sh [--mode Text|Visual|Full] [--frames N] [--ticks N] [--scene PATH]

Writes diagnostics under out/ai_diagnostics/<timestamp>/.
USAGE
            exit 0
            ;;
        *)
            echo "Unknown argument: $1" >&2
            exit 2
            ;;
    esac
done

case "${mode,,}" in
    text) mode="Text" ;;
    visual) mode="Visual" ;;
    full) mode="Full" ;;
    *)
        echo "Invalid mode: $mode" >&2
        exit 2
        ;;
esac

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
stamp="$(date +"%Y%m%d_%H%M%S_%3N")"
bundle="$root/out/ai_diagnostics/$stamp"
log_dir="$bundle/logs"
screenshot_out_dir="$bundle/screenshots"
summary="$bundle/summary.txt"
failures=0

mkdir -p "$log_dir" "$screenshot_out_dir"

add_summary() {
    echo "$*" | tee -a "$summary" >/dev/null
}

add_failure() {
    failures=$((failures + 1))
    add_summary "$*"
}

run_step() {
    local name="$1"
    local log_name="$2"
    shift 2

    local log_file="$log_dir/$log_name"
    add_summary ""
    add_summary "=== $name ==="

    if [[ "$1" == */* && ! -x "$1" ]]; then
        echo "Missing executable or script: $1" >"$log_file"
        add_failure "FAILED missing: $1"
        return
    fi

    if "$@" >"$log_file" 2>&1; then
        add_summary "OK; log: $log_file"
    else
        local exit_code=$?
        add_failure "FAILED exit $exit_code; log: $log_file"
    fi
}

copy_runtime_logs() {
    local runtime_log_dir="$root/data/logs"
    [[ -d "$runtime_log_dir" ]] || return

    mkdir -p "$bundle/runtime_logs"
    find "$runtime_log_dir" -maxdepth 1 -type f -name '*.log' -exec cp -f {} "$bundle/runtime_logs/" \;
}

move_ai_screenshots() {
    local engine_screenshot_dir="$root/_save/screenshots"
    [[ -d "$engine_screenshot_dir" ]] || {
        add_summary "No engine screenshot directory found."
        return
    }

    local moved=0
    local prefix file
    shopt -s nullglob
    for prefix in "$@"; do
        for file in "$engine_screenshot_dir/${prefix}"*.png; do
            mv -f -- "$file" "$screenshot_out_dir/"
            moved=$((moved + 1))
        done
    done
    shopt -u nullglob

    add_summary "Moved $moved AI screenshot(s) to $screenshot_out_dir"
}

write_inventory() {
    {
        echo "Diagnostics bundle: $bundle"
        echo
        echo "data/logs:"
        if [[ -d "$root/data/logs" ]]; then
            find "$root/data/logs" -maxdepth 1 -type f -printf '%TY-%Tm-%Td %TH:%TM %s %p\n' | sort -r
        else
            echo "missing"
        fi
        echo
        echo "_save/screenshots:"
        if [[ -d "$root/_save/screenshots" ]]; then
            find "$root/_save/screenshots" -maxdepth 1 -type f -printf '%TY-%Tm-%Td %TH:%TM %s %p\n' | sort -r
        else
            echo "missing"
        fi
    } >"$log_dir/runtime_inventory.txt"
    add_summary "OK; log: $log_dir/runtime_inventory.txt"
}

text_diagnostics() {
    run_step "Git status" "git_status.txt" git status --short

    add_summary ""
    add_summary "=== Runtime inventory ==="
    write_inventory

    run_step \
        "Gameplay tests" \
        "gameplay_tests.log" \
        "$root/build/linux-debug/apps/gameplay_tests/HockeyGameplayTests" \
        --root "$root"

    run_step \
        "Dedicated server bounded run" \
        "server_bounded_run.log" \
        "$root/build/linux-debug/apps/dedicated_server/HockeyDedicatedServer" \
        --root "$root" \
        --max-ticks "$ticks" \
        --log "$log_dir/server_runtime.log"

    copy_runtime_logs
}

visual_diagnostics() {
    local capture_frames="$frames"
    local editor_frames="$frames"
    (( capture_frames < 5 )) && capture_frames=5
    (( editor_frames < 5 )) && editor_frames=5

    local client_prefix="ai_client_$stamp"
    local editor_window_prefix="ai_editor_window_$stamp"
    local editor_view_prefix="ai_editor_view_$stamp"

    run_step \
        "Client screenshot" \
        "client_screenshot.log" \
        "$root/build/linux-debug/apps/game_client/HockeyGameClient" \
        --root "$root" \
        --screenshot "$client_prefix" \
        --max-frames "$capture_frames" \
        --vk-validation \
        --log "$log_dir/client_runtime.log"
    move_ai_screenshots "$client_prefix"

    run_step \
        "Editor full-window screenshot" \
        "editor_window_screenshot.log" \
        "$root/build/linux-debug/apps/map_editor/HockeyMapEditor" \
        --root "$root" \
        --scene "$scene" \
        --screenshot \
        --screenshot-prefix "$editor_window_prefix" \
        --screenshot-frame 3 \
        --max-frames "$editor_frames" \
        --vk-validation \
        --log "$log_dir/editor_window_runtime.log"
    move_ai_screenshots "$editor_window_prefix"

    run_step \
        "Editor viewport screenshots" \
        "editor_viewport_screenshots.log" \
        "$root/build/linux-debug/apps/map_editor/HockeyMapEditor" \
        --root "$root" \
        --scene "$scene" \
        --capture-viewports \
        --capture-prefix "$editor_view_prefix" \
        --capture-width 1920 \
        --capture-height 1080 \
        --max-frames "$editor_frames" \
        --vk-validation \
        --log "$log_dir/editor_viewports_runtime.log"
    move_ai_screenshots "$editor_view_prefix"

    copy_runtime_logs
}

add_summary "AI diagnostics bundle: $bundle"
add_summary "Mode: $mode"
add_summary "Frames: $frames"
add_summary "Ticks: $ticks"
add_summary "Scene: $scene"

case "$mode" in
    Text)
        text_diagnostics
        ;;
    Visual)
        visual_diagnostics
        ;;
    Full)
        run_step "Build debug" "build_debug.log" bash "$root/scripts/linux/build_debug.sh"
        run_step "Linux test helper" "linux_tests.log" bash "$root/scripts/linux/test.sh"
        text_diagnostics
        visual_diagnostics
        ;;
esac

add_summary ""
if (( failures > 0 )); then
    add_summary "Result: FAILED with $failures failure(s)."
    add_summary "Bundle: $bundle"
    exit 1
fi

add_summary "Result: OK"
add_summary "Bundle: $bundle"
