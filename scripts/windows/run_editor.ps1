$ErrorActionPreference = "Stop"
. "$PSScriptRoot\Env.ps1"
& (Join-Path $script:HockeyRoot "build\windows-debug\apps\map_editor\HockeyMapEditor.exe") --root $script:HockeyRoot @args
