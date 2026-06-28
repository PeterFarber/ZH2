param(
    [string]$Preset = "windows-release",
    [string]$OutputDir = "out/packages/server",
    [string]$BuildMode = "Release",
    [switch]$IncludeDebugSymbols
)

$ErrorActionPreference = "Stop"
. "$PSScriptRoot\Env.ps1"

$root = (Resolve-Path (Join-Path $PSScriptRoot "../..")).Path
$buildPreset = "build-$Preset"
$buildDir = Join-Path $root "build/windows-release"
if ($Preset -ne "windows-release") {
    $buildDir = Join-Path $root ("build/" + ($Preset -replace '^windows-', 'windows-'))
}
$outputPath = if ([System.IO.Path]::IsPathRooted($OutputDir)) { $OutputDir } else { Join-Path $root $OutputDir }
$packagePath = Join-Path $buildDir "packages/server_runtime.hkpack"

if (-not (Test-Path (Join-Path $buildDir "CMakeCache.txt"))) {
    cmake --preset $Preset
    if ($LASTEXITCODE -ne 0) { throw "cmake configure failed for preset $Preset" }
}

cmake --build --preset $buildPreset --target HockeyAssetTool
if ($LASTEXITCODE -ne 0) { throw "HockeyAssetTool build failed" }
& (Join-Path $buildDir "apps/asset_tool/HockeyAssetTool.exe") --root $root package-runtime --target server --config data/config/server.toml --output $packagePath
if ($LASTEXITCODE -ne 0) { throw "server runtime package generation failed" }
cmake --build --preset $buildPreset --target HockeyDedicatedServer
if ($LASTEXITCODE -ne 0) { throw "HockeyDedicatedServer build failed" }

if (Test-Path $outputPath) {
    Get-ChildItem -LiteralPath $outputPath -Force | Remove-Item -Recurse -Force
} else {
    New-Item -ItemType Directory -Path $outputPath | Out-Null
}

Copy-Item -LiteralPath (Join-Path $buildDir "apps/dedicated_server/HockeyDedicatedServer.exe") -Destination (Join-Path $outputPath "HockeyDedicatedServer.exe")
Copy-Item -LiteralPath (Join-Path $root "data/config/server.toml") -Destination (Join-Path $outputPath "HockeyDedicatedServer.toml")

if ($IncludeDebugSymbols) {
    $pdb = Join-Path $buildDir "apps/dedicated_server/HockeyDedicatedServer.pdb"
    if (Test-Path $pdb) {
        Copy-Item -LiteralPath $pdb -Destination (Join-Path $outputPath "HockeyDedicatedServer.pdb")
    }
}

if (Test-Path (Join-Path $outputPath "data")) {
    throw "Package output must not contain a data directory: $outputPath"
}

Write-Host "Packaged server ($BuildMode) to $outputPath"
