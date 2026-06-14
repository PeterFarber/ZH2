# One-time Windows setup: verify build tools and bootstrap vcpkg.
$ErrorActionPreference = "Stop"

Write-Host "== HockeyGame Windows setup =="

function Test-Tool($name) {
    if (Get-Command $name -ErrorAction SilentlyContinue) {
        Write-Host "  found: $name"
        return $true
    }
    Write-Warning "  missing: $name"
    return $false
}

Write-Host "Checking required tools..."
$ok = $true
$ok = (Test-Tool "cmake") -and $ok
$ok = (Test-Tool "ninja") -and $ok
$ok = (Test-Tool "git") -and $ok

if (-not $ok) {
    Write-Warning "Install the missing tools (Visual Studio 2022 C++ workload provides MSVC + CMake + Ninja)."
    Write-Warning "Standalone: https://cmake.org/download and https://github.com/ninja-build/ninja/releases"
}

# Bootstrap vcpkg if VCPKG_ROOT points at a checkout that is not yet bootstrapped.
if ($env:VCPKG_ROOT) {
    $vcpkgExe = Join-Path $env:VCPKG_ROOT "vcpkg.exe"
    $bootstrap = Join-Path $env:VCPKG_ROOT "bootstrap-vcpkg.bat"
    if (Test-Path $vcpkgExe) {
        Write-Host "vcpkg already bootstrapped at $env:VCPKG_ROOT."
    } elseif (Test-Path $bootstrap) {
        Write-Host "Bootstrapping vcpkg at $env:VCPKG_ROOT..."
        & $bootstrap -disableMetrics
    } else {
        Write-Warning "VCPKG_ROOT is set to '$env:VCPKG_ROOT' but no vcpkg checkout was found there."
    }
} else {
    Write-Warning "VCPKG_ROOT is not set. Clone vcpkg and set VCPKG_ROOT, for example:"
    Write-Host '  git clone https://github.com/microsoft/vcpkg $env:USERPROFILE\vcpkg'
    Write-Host '  & "$env:USERPROFILE\vcpkg\bootstrap-vcpkg.bat" -disableMetrics'
    Write-Host '  setx VCPKG_ROOT "$env:USERPROFILE\vcpkg"'
}

Write-Host "== Setup complete =="
