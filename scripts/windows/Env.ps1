# Shared Windows build environment bootstrap for HockeyGame.
# Dot-source from scripts/windows/*.ps1 — no manual PATH, VCPKG_ROOT, or vcvars setup required.
$ErrorActionPreference = "Stop"

$script:HockeyRoot = (Resolve-Path (Join-Path $PSScriptRoot "../..")).Path
$script:HockeyEnvInitialized = $false

function Find-VisualStudioInstallation {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $path = & $vswhere -latest -products * `
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
            -property installationPath 2>$null
        if ($path) {
            return $path.Trim()
        }
    }

    foreach ($edition in @("Community", "Professional", "Enterprise", "BuildTools")) {
        $candidate = Join-Path ${env:ProgramFiles} "Microsoft Visual Studio\2022\$edition"
        $vcvars = Join-Path $candidate "VC\Auxiliary\Build\vcvars64.bat"
        if (Test-Path $vcvars) {
            return $candidate
        }
    }

    return $null
}

function Import-VisualStudioDevEnvironment {
    param([string]$VsInstallPath)

    $vcvars = Join-Path $VsInstallPath "VC\Auxiliary\Build\vcvars64.bat"
    if (-not (Test-Path $vcvars)) {
        throw "vcvars64.bat not found at $vcvars"
    }

    cmd /c "`"$vcvars`" >nul 2>&1 && set" | ForEach-Object {
        if ($_ -match '^([^=]+)=(.*)$') {
            Set-Item -Path "env:$($matches[1])" -Value $matches[2]
        }
    }
}

function Ensure-HockeyVcpkg {
    param([string]$RepoRoot)

    $vcpkgRoot = Join-Path $RepoRoot ".vcpkg"
    $env:VCPKG_ROOT = $vcpkgRoot

    $vcpkgExe = Join-Path $vcpkgRoot "vcpkg.exe"
    if (Test-Path $vcpkgExe) {
        return $vcpkgRoot
    }

    if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
        throw @"
git was not found on PATH.
Run: .\scripts\windows\setup.ps1
"@
    }

    if (-not (Test-Path $vcpkgRoot)) {
        Write-Host "Cloning vcpkg into $vcpkgRoot (first-time setup, may take a minute)..."
        & git clone --depth 1 https://github.com/microsoft/vcpkg $vcpkgRoot
    }

    $bootstrap = Join-Path $vcpkgRoot "bootstrap-vcpkg.bat"
    if (-not (Test-Path $bootstrap)) {
        throw "vcpkg checkout at $vcpkgRoot is incomplete (bootstrap-vcpkg.bat missing)."
    }

    Write-Host "Bootstrapping vcpkg..."
    cmd /c "`"$bootstrap`" -disableMetrics"
    if ($LASTEXITCODE -ne 0) {
        throw "vcpkg bootstrap failed with exit code $LASTEXITCODE."
    }

    if (-not (Test-Path $vcpkgExe)) {
        throw "vcpkg bootstrap failed; expected $vcpkgExe"
    }

    return $vcpkgRoot
}

function Remove-StaleVcpkgCMakeCache {
    param([string]$BuildDir)

    $buildPath = Join-Path $script:HockeyRoot $BuildDir
    $cacheFile = Join-Path $buildPath "CMakeCache.txt"
    if (-not (Test-Path $cacheFile)) {
        return
    }

    $cache = Get-Content $cacheFile -Raw
    $expectedToolchain = ($env:VCPKG_ROOT -replace '\\', '/') + "/scripts/buildsystems/vcpkg.cmake"
    if ($cache -match [regex]::Escape($expectedToolchain)) {
        return
    }

    Write-Host "Removing stale CMake cache at $buildPath (toolchain or vcpkg root changed)."
    Remove-Item $buildPath -Recurse -Force
}

function Initialize-HockeyWindowsEnv {
    if ($script:HockeyEnvInitialized) {
        return
    }

    Set-Location $script:HockeyRoot

    $vs = Find-VisualStudioInstallation
    if (-not $vs) {
        throw @"
Visual Studio 2022 with the 'Desktop development with C++' workload was not found.
Run: .\scripts\windows\setup.ps1
"@
    }

    Import-VisualStudioDevEnvironment -VsInstallPath $vs

    $cmakeBin = Join-Path $vs "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
    $ninjaDir = Join-Path $vs "Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"
    foreach ($dir in @($cmakeBin, $ninjaDir)) {
        if (Test-Path $dir) {
            $env:Path = "$dir;$env:Path"
        }
    }

    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
        throw "cmake not found. Run: .\scripts\windows\setup.ps1"
    }
    if (-not (Get-Command ninja -ErrorAction SilentlyContinue)) {
        throw "ninja not found. Run: .\scripts\windows\setup.ps1"
    }

    $vcpkgRoot = Ensure-HockeyVcpkg -RepoRoot $script:HockeyRoot
    $toolchain = Join-Path $vcpkgRoot "scripts\buildsystems\vcpkg.cmake"
    if (-not (Test-Path $toolchain)) {
        throw "vcpkg toolchain file missing: $toolchain"
    }

    $script:HockeyEnvInitialized = $true
}

Initialize-HockeyWindowsEnv
