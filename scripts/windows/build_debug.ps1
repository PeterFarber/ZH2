$ErrorActionPreference = "Stop"
. "$PSScriptRoot\Env.ps1"
cmake --build --preset build-windows-debug
