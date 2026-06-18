param(
    [ValidateSet("Text", "Visual", "Full")]
    [string]$Mode = "Text",

    [int]$Frames = 5,
    [int]$Ticks = 300,
    [string]$Scene = "data/raw/scenes/main_rink.scene.yaml"
)

$ErrorActionPreference = "Stop"

. "$PSScriptRoot\Env.ps1"

$stamp = Get-Date -Format "yyyyMMdd_HHmmss_fff"
$bundle = Join-Path $script:HockeyRoot "out\ai_diagnostics\$stamp"
$logDir = Join-Path $bundle "logs"
$screenshotOutDir = Join-Path $bundle "screenshots"
$summary = Join-Path $bundle "summary.txt"
$script:Failures = 0

New-Item -ItemType Directory -Force -Path $logDir, $screenshotOutDir | Out-Null

function Add-Summary {
    param([string]$Message)
    $Message | Tee-Object -FilePath $summary -Append | Out-Null
}

function Add-Failure {
    param([string]$Message)
    $script:Failures++
    Add-Summary $Message
}

function Invoke-ExternalStep {
    param(
        [string]$Name,
        [string]$LogName,
        [string]$FilePath,
        [string[]]$Arguments
    )

    $logFile = Join-Path $logDir $LogName
    Add-Summary ""
    Add-Summary "=== $Name ==="

    $isPath = $FilePath.Contains("\") -or $FilePath.Contains("/")
    if ($isPath -and -not (Test-Path -LiteralPath $FilePath)) {
        "Missing executable or script: $FilePath" | Out-File -FilePath $logFile -Encoding utf8
        Add-Failure "FAILED missing: $FilePath"
        return
    }

    if (-not $isPath -and -not (Get-Command $FilePath -ErrorAction SilentlyContinue)) {
        "Missing command: $FilePath" | Out-File -FilePath $logFile -Encoding utf8
        Add-Failure "FAILED missing command: $FilePath"
        return
    }

    try {
        $global:LASTEXITCODE = 0
        $output = & $FilePath @Arguments 2>&1
        $exitCode = $LASTEXITCODE
        $output | Out-File -FilePath $logFile -Encoding utf8

        if ($exitCode -ne 0) {
            Add-Failure "FAILED exit $exitCode; log: $logFile"
        } else {
            Add-Summary "OK; log: $logFile"
        }
    } catch {
        $_ | Out-String | Out-File -FilePath $logFile -Encoding utf8
        Add-Failure "FAILED exception; log: $logFile"
    }
}

function Invoke-LocalStep {
    param(
        [string]$Name,
        [string]$LogName,
        [scriptblock]$Action
    )

    $logFile = Join-Path $logDir $LogName
    Add-Summary ""
    Add-Summary "=== $Name ==="
    try {
        & $Action | Out-File -FilePath $logFile -Encoding utf8
        Add-Summary "OK; log: $logFile"
    } catch {
        $_ | Out-String | Out-File -FilePath $logFile -Encoding utf8
        Add-Failure "FAILED exception; log: $logFile"
    }
}

function Copy-RuntimeLogs {
    $runtimeLogDir = Join-Path $script:HockeyRoot "data\logs"
    if (-not (Test-Path -LiteralPath $runtimeLogDir)) {
        return
    }

    $target = Join-Path $bundle "runtime_logs"
    New-Item -ItemType Directory -Force -Path $target | Out-Null
    Get-ChildItem -LiteralPath $runtimeLogDir -File -Filter "*.log" -ErrorAction SilentlyContinue |
        Copy-Item -Destination $target -Force
}

function Move-AiScreenshots {
    param([string[]]$Prefixes)

    $engineScreenshotDir = Join-Path $script:HockeyRoot "_save\screenshots"
    if (-not (Test-Path -LiteralPath $engineScreenshotDir)) {
        Add-Summary "No engine screenshot directory found."
        return
    }

    $moved = 0
    foreach ($file in Get-ChildItem -LiteralPath $engineScreenshotDir -File -Filter "*.png") {
        $baseName = $file.BaseName.ToLowerInvariant()
        foreach ($prefix in $Prefixes) {
            if ($baseName.StartsWith($prefix.ToLowerInvariant())) {
                Move-Item -LiteralPath $file.FullName -Destination (Join-Path $screenshotOutDir $file.Name) -Force
                $moved++
                break
            }
        }
    }

    Add-Summary "Moved $moved AI screenshot(s) to $screenshotOutDir"
}

function Invoke-TextDiagnostics {
    Invoke-ExternalStep `
        -Name "Git status" `
        -LogName "git_status.txt" `
        -FilePath "git" `
        -Arguments @("status", "--short")

    Invoke-LocalStep `
        -Name "Runtime inventory" `
        -LogName "runtime_inventory.txt" `
        -Action {
            "Diagnostics bundle: $bundle"
            ""
            "data/logs:"
            $runtimeLogDir = Join-Path $script:HockeyRoot "data\logs"
            if (Test-Path -LiteralPath $runtimeLogDir) {
                Get-ChildItem -LiteralPath $runtimeLogDir -File | Sort-Object LastWriteTime -Descending |
                    Select-Object LastWriteTime, Length, FullName
            } else {
                "missing"
            }
            ""
            "_save/screenshots:"
            $engineScreenshotDir = Join-Path $script:HockeyRoot "_save\screenshots"
            if (Test-Path -LiteralPath $engineScreenshotDir) {
                Get-ChildItem -LiteralPath $engineScreenshotDir -File | Sort-Object LastWriteTime -Descending |
                    Select-Object LastWriteTime, Length, FullName
            } else {
                "missing"
            }
        }

    $gameplayTests = Join-Path $script:HockeyRoot "build\windows-debug\apps\gameplay_tests\HockeyGameplayTests.exe"
    Invoke-ExternalStep `
        -Name "Gameplay tests" `
        -LogName "gameplay_tests.log" `
        -FilePath $gameplayTests `
        -Arguments @("--root", $script:HockeyRoot)

    $server = Join-Path $script:HockeyRoot "build\windows-debug\apps\dedicated_server\HockeyDedicatedServer.exe"
    Invoke-ExternalStep `
        -Name "Dedicated server bounded run" `
        -LogName "server_bounded_run.log" `
        -FilePath $server `
        -Arguments @(
            "--root", $script:HockeyRoot,
            "--max-ticks", $Ticks.ToString(),
            "--log", (Join-Path $logDir "server_runtime.log")
        )

    Copy-RuntimeLogs
}

function Invoke-VisualDiagnostics {
    $captureFrames = [Math]::Max($Frames, 5)
    $editorFrames = [Math]::Max($Frames, 5)

    $clientPrefix = "ai_client_$stamp"
    $editorWindowPrefix = "ai_editor_window_$stamp"
    $editorViewportPrefix = "ai_editor_view_$stamp"

    $client = Join-Path $script:HockeyRoot "build\windows-debug\apps\game_client\HockeyGameClient.exe"
    Invoke-ExternalStep `
        -Name "Client screenshot" `
        -LogName "client_screenshot.log" `
        -FilePath $client `
        -Arguments @(
            "--root", $script:HockeyRoot,
            "--screenshot", $clientPrefix,
            "--max-frames", $captureFrames.ToString(),
            "--vk-validation",
            "--log", (Join-Path $logDir "client_runtime.log")
        )
    Move-AiScreenshots -Prefixes @($clientPrefix)

    $editor = Join-Path $script:HockeyRoot "build\windows-debug\apps\map_editor\HockeyMapEditor.exe"
    Invoke-ExternalStep `
        -Name "Editor full-window screenshot" `
        -LogName "editor_window_screenshot.log" `
        -FilePath $editor `
        -Arguments @(
            "--root", $script:HockeyRoot,
            "--scene", $Scene,
            "--screenshot",
            "--screenshot-prefix", $editorWindowPrefix,
            "--screenshot-frame", "3",
            "--max-frames", $editorFrames.ToString(),
            "--vk-validation",
            "--log", (Join-Path $logDir "editor_window_runtime.log")
        )
    Move-AiScreenshots -Prefixes @($editorWindowPrefix)

    Invoke-ExternalStep `
        -Name "Editor viewport screenshots" `
        -LogName "editor_viewport_screenshots.log" `
        -FilePath $editor `
        -Arguments @(
            "--root", $script:HockeyRoot,
            "--scene", $Scene,
            "--capture-viewports",
            "--capture-prefix", $editorViewportPrefix,
            "--capture-width", "1920",
            "--capture-height", "1080",
            "--max-frames", $editorFrames.ToString(),
            "--vk-validation",
            "--log", (Join-Path $logDir "editor_viewports_runtime.log")
        )
    Move-AiScreenshots -Prefixes @($editorViewportPrefix)

    Copy-RuntimeLogs
}

Add-Summary "AI diagnostics bundle: $bundle"
Add-Summary "Mode: $Mode"
Add-Summary "Frames: $Frames"
Add-Summary "Ticks: $Ticks"
Add-Summary "Scene: $Scene"

switch ($Mode) {
    "Text" {
        Invoke-TextDiagnostics
    }
    "Visual" {
        Invoke-VisualDiagnostics
    }
    "Full" {
        Invoke-ExternalStep `
            -Name "Build debug" `
            -LogName "build_debug.log" `
            -FilePath (Join-Path $script:HockeyRoot "scripts\windows\build_debug.ps1") `
            -Arguments @()
        Invoke-ExternalStep `
            -Name "Windows test suite" `
            -LogName "windows_tests.log" `
            -FilePath (Join-Path $script:HockeyRoot "scripts\windows\test.ps1") `
            -Arguments @()
        Invoke-TextDiagnostics
        Invoke-VisualDiagnostics
    }
}

Add-Summary ""
if ($script:Failures -gt 0) {
    Add-Summary "Result: FAILED with $script:Failures failure(s)."
    Add-Summary "Bundle: $bundle"
    exit 1
}

Add-Summary "Result: OK"
Add-Summary "Bundle: $bundle"
