set windows-shell := ["powershell.exe", "-NoLogo", "-NoProfile", "-ExecutionPolicy", "Bypass", "-Command"]
set shell := ["sh", "-cu"]

platform := if os_family() == "windows" { "windows" } else { "linux" }
script_ext := if os_family() == "windows" { ".ps1" } else { ".sh" }
mode_flag := if os_family() == "windows" { "-Mode" } else { "--mode" }
python_cmd := if os_family() == "windows" { "python" } else { "python3" }

# List available recipes.
default:
    @just --list

# Check whether optional local AI/developer tools are on PATH.
[windows]
tools-check:
    @$tools = @("just", "clangd", "serena", "bd", "graphify", "dotagents", "uv", "npx"); foreach ($tool in $tools) { if (Get-Command $tool -ErrorAction SilentlyContinue) { Write-Host "OK      $tool" -ForegroundColor Green } else { Write-Host "MISSING $tool" -ForegroundColor Yellow } }; Write-Host "INFO    Superpowers is installed through the Codex plugin UI, not a required repo CLI."

# Check whether optional local AI/developer tools are on PATH.
[unix]
tools-check:
    @for tool in just clangd serena bd graphify dotagents uv npx; do if command -v "$tool" >/dev/null 2>&1; then echo "OK      $tool"; else echo "MISSING $tool"; fi; done
    @echo "INFO    Superpowers is installed through the Codex plugin UI, not a required repo CLI."

# Run Serena's project health check with UTF-8 output.
[windows]
serena-health:
    @$env:Path = $env:Path + ';' + [Environment]::GetEnvironmentVariable('Path','User'); $env:PYTHONIOENCODING='utf-8'; $env:PYTHONUTF8='1'; $output = serena project health-check 2>&1; $status = $LASTEXITCODE; if ($status -eq 0) { $output | Select-String -Pattern 'Using custom LS setting|Starting language server with language cpp|clangd version|Health check passed|Log saved to' } else { $output; exit $status }

# Run Serena's project health check with UTF-8 output.
[unix]
serena-health:
    @set +e; output="$(PYTHONIOENCODING=utf-8 PYTHONUTF8=1 serena project health-check 2>&1)"; status=$?; if [ "$status" -eq 0 ]; then printf "%s\n" "$output" | grep -E "Using custom LS setting|Starting language server with language cpp|clangd version|Health check passed|Log saved to" || true; else printf "%s\n" "$output"; exit "$status"; fi

# Quick local readiness check for AI-assisted coding.
ai-ready: tools-check serena-health

# Derive readable local names for Graphify communities without an LLM backend.
graphify-name-communities:
    @{{python_cmd}} scripts/tools/name_graphify_communities.py

# Configure the debug CMake preset for this host.
configure-debug:
    @./scripts/{{platform}}/configure_debug{{script_ext}}

# Configure the release CMake preset for this host.
configure-release:
    @./scripts/{{platform}}/configure_release{{script_ext}}

# Build the debug preset for this host.
build-debug:
    @./scripts/{{platform}}/build_debug{{script_ext}}

# Build the release preset for this host.
build-release:
    @./scripts/{{platform}}/build_release{{script_ext}}

# Run the full debug test suite for this host.
test:
    @./scripts/{{platform}}/test{{script_ext}}

# Alias for the full debug test suite.
test-all: test

# Run the bounded text diagnostics smoke pass.
smoke-text:
    @./scripts/{{platform}}/ai_smoke{{script_ext}} {{mode_flag}} Text

# Run the bounded visual diagnostics smoke pass.
smoke-visual:
    @./scripts/{{platform}}/ai_smoke{{script_ext}} {{mode_flag}} Visual

# Run build, tests, text diagnostics, and visual diagnostics.
smoke-full:
    @./scripts/{{platform}}/ai_smoke{{script_ext}} {{mode_flag}} Full

# Run the game client debug helper.
run-client:
    @./scripts/{{platform}}/run_client{{script_ext}}

# Run the map editor debug helper.
run-editor:
    @./scripts/{{platform}}/run_editor{{script_ext}}

# Run the dedicated server debug helper.
run-server:
    @./scripts/{{platform}}/run_server{{script_ext}}

# Completion contract for most changes.
verify: build-debug test smoke-text

# Optional local AI bootstrap recipes
import? '.ai/setup/justfile.ai'
