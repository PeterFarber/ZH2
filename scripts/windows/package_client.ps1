param(
    [string]$Preset = "windows-release",
    [string]$OutputDir = "out/packages/client",
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
$packagePath = Join-Path $buildDir "packages/client_runtime.hkpack"
$appDir = Join-Path $buildDir "apps/game_client"

function Copy-AppRuntimeDlls {
    param(
        [string]$SourceDir,
        [string]$DestinationDir
    )

    Get-ChildItem -LiteralPath $SourceDir -Filter "*.dll" -File | ForEach-Object {
        Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $DestinationDir $_.Name)
    }
}

if (-not (Test-Path (Join-Path $buildDir "CMakeCache.txt"))) {
    cmake --preset $Preset
    if ($LASTEXITCODE -ne 0) { throw "cmake configure failed for preset $Preset" }
}

cmake --build --preset $buildPreset --target HockeyAssetTool
if ($LASTEXITCODE -ne 0) { throw "HockeyAssetTool build failed" }
& (Join-Path $buildDir "apps/asset_tool/HockeyAssetTool.exe") --root $root package-runtime --target client --config data/config/client.toml --output $packagePath
if ($LASTEXITCODE -ne 0) { throw "client runtime package generation failed" }
cmake --build --preset $buildPreset --target HockeyGameClient
if ($LASTEXITCODE -ne 0) { throw "HockeyGameClient build failed" }

if (Test-Path $outputPath) {
    Get-ChildItem -LiteralPath $outputPath -Force | Remove-Item -Recurse -Force
} else {
    New-Item -ItemType Directory -Path $outputPath | Out-Null
}

Copy-Item -LiteralPath (Join-Path $appDir "HockeyGameClient.exe") -Destination (Join-Path $outputPath "HockeyGameClient.exe")
Copy-AppRuntimeDlls -SourceDir $appDir -DestinationDir $outputPath
Copy-Item -LiteralPath (Join-Path $root "data/config/client.toml") -Destination (Join-Path $outputPath "HockeyGameClient.toml")

if ($IncludeDebugSymbols) {
    $pdb = Join-Path $buildDir "apps/game_client/HockeyGameClient.pdb"
    if (Test-Path $pdb) {
        Copy-Item -LiteralPath $pdb -Destination (Join-Path $outputPath "HockeyGameClient.pdb")
    }
}

if (Test-Path (Join-Path $outputPath "data")) {
    throw "Package output must not contain a data directory: $outputPath"
}

Write-Host "Packaged client ($BuildMode) to $outputPath"
