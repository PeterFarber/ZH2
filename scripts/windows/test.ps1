$ErrorActionPreference = "Stop"
. "$PSScriptRoot\Env.ps1"

$tests = @(
    "apps\core_tests\HockeyCoreTests.exe",
    "apps\ecs_tests\HockeyECSTests.exe",
    "apps\asset_tests\HockeyAssetTests.exe",
    "apps\renderer_tests\HockeyRendererTests.exe",
    "apps\editor_tests\HockeyEditorTests.exe",
    "apps\physics_tests\HockeyPhysicsTests.exe",
    "apps\gameplay_tests\HockeyGameplayTests.exe"
)

$buildDir = Join-Path $script:HockeyRoot "build\windows-debug"
$failed = 0

foreach ($relativePath in $tests) {
    $exe = Join-Path $buildDir $relativePath
    if (-not (Test-Path $exe)) {
        Write-Host "SKIP  $relativePath (not built yet)" -ForegroundColor Yellow
        $failed++
        continue
    }

    Write-Host "`n=== $relativePath ===" -ForegroundColor Cyan
    & $exe --root $script:HockeyRoot
    if ($LASTEXITCODE -ne 0) {
        Write-Host "FAILED (exit $LASTEXITCODE)" -ForegroundColor Red
        $failed++
    }
}

if ($failed -gt 0) {
    throw "$failed test executable(s) missing or failed."
}
