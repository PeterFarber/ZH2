# Installs prerequisites, bootstraps vcpkg, and verifies the Windows build environment.
param(
    [switch]$SkipInstall,
    [switch]$Elevated
)

$ErrorActionPreference = "Stop"

Write-Host "== HockeyGame Windows setup =="

. "$PSScriptRoot\Prerequisites.ps1"

if (-not $SkipInstall) {
    $status = Get-HockeyPrerequisiteStatus
    if (-not $status.Ready) {
        if (-not $Elevated -and -not (Test-IsAdministrator)) {
            Request-SetupElevation -SetupScript $PSCommandPath
        }
        Install-HockeyPrerequisites | Out-Null
    } else {
        Write-Host "Prerequisites already satisfied."
    }
}

. "$PSScriptRoot\Env.ps1"

Write-Host "  repo:    $script:HockeyRoot"
Write-Host "  vcpkg:   $env:VCPKG_ROOT"
Write-Host "  cmake:   $(Get-Command cmake | Select-Object -ExpandProperty Source)"
Write-Host "  ninja:   $(Get-Command ninja | Select-Object -ExpandProperty Source)"
Write-Host "== Setup complete =="
