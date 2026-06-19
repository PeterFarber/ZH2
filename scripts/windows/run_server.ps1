$ErrorActionPreference = "Stop"
. "$PSScriptRoot\Env.ps1"
& (Join-Path $script:HockeyRoot "build\windows-debug\apps\dedicated_server\HockeyDedicatedServer.exe") --root $script:HockeyRoot @args
