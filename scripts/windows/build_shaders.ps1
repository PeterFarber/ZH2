$ErrorActionPreference = "Stop"
. "$PSScriptRoot\Env.ps1"

cmake --build --preset build-windows-debug --target HockeyShaderTool

$tool = Join-Path $script:HockeyRoot "build/windows-debug/apps/shader_tool/HockeyShaderTool.exe"
if (-not (Test-Path -LiteralPath $tool)) {
    throw "Shader tool was not built at $tool"
}

& $tool --root $script:HockeyRoot
