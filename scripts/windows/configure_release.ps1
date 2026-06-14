$ErrorActionPreference = "Stop"
. "$PSScriptRoot\Env.ps1"
Remove-StaleVcpkgCMakeCache -BuildDir "build/windows-release"
cmake --preset windows-release
