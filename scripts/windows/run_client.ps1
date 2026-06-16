$ErrorActionPreference = "Stop"
. "$PSScriptRoot\Env.ps1"
& (Join-Path $script:HockeyRoot "build\windows-debug\apps\game_client\HockeyGameClient.exe") --root $script:HockeyRoot
