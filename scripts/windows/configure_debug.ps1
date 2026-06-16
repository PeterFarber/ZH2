$ErrorActionPreference = "Stop"
. "$PSScriptRoot\Env.ps1"
Remove-StaleVcpkgCMakeCache -BuildDir "build/windows-debug"
cmake --preset windows-debug
