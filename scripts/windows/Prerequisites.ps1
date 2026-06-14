# Installs Windows build prerequisites via winget and the Visual Studio Installer.
$ErrorActionPreference = "Stop"

$script:VsCppWorkload = "Microsoft.VisualStudio.Workload.NativeDesktop"
$script:VsCmakeComponent = "Microsoft.VisualStudio.Component.VC.CMake.Project"
$script:VsWingetId = "Microsoft.VisualStudio.2022.Community"
$script:GitWingetId = "Git.Git"

function Test-IsAdministrator {
    $identity = [Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()
    return $identity.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Test-WingetAvailable {
    return [bool](Get-Command winget -ErrorAction SilentlyContinue)
}

function Get-VsWhereExe {
    $path = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $path) { return $path }
    return $null
}

function Find-VisualStudioWithCpp {
    $vswhere = Get-VsWhereExe
    if ($vswhere) {
        $path = & $vswhere -latest -products * `
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
            -property installationPath 2>$null
        if ($path) { return $path.Trim() }
    }

    foreach ($edition in @("Community", "Professional", "Enterprise", "BuildTools")) {
        $candidate = Join-Path ${env:ProgramFiles} "Microsoft Visual Studio\2022\$edition"
        $vcvars = Join-Path $candidate "VC\Auxiliary\Build\vcvars64.bat"
        if (Test-Path $vcvars) { return $candidate }
    }

    return $null
}

function Find-AnyVisualStudio2022 {
    $vswhere = Get-VsWhereExe
    if ($vswhere) {
        $path = & $vswhere -latest -products * -version "[17.0,18.0)" -property installationPath 2>$null
        if ($path) { return $path.Trim() }
    }

    foreach ($edition in @("Community", "Professional", "Enterprise", "BuildTools")) {
        $candidate = Join-Path ${env:ProgramFiles} "Microsoft Visual Studio\2022\$edition"
        if (Test-Path $candidate) { return $candidate }
    }

    return $null
}

function Test-VisualStudioCmakeTools {
    param([string]$VsInstallPath)

    $cmake = Join-Path $VsInstallPath "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    $ninja = Join-Path $VsInstallPath "Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
    return (Test-Path $cmake) -and (Test-Path $ninja)
}

function Refresh-GitPath {
    foreach ($dir in @(
            (Join-Path ${env:ProgramFiles} "Git\cmd"),
            (Join-Path ${env:ProgramFiles(x86)} "Git\cmd")
        )) {
        if (Test-Path $dir) {
            $env:Path = "$dir;$env:Path"
        }
    }
}

function Test-GitAvailable {
    Refresh-GitPath
    return [bool](Get-Command git -ErrorAction SilentlyContinue)
}

function Get-HockeyPrerequisiteStatus {
    $vs = Find-VisualStudioWithCpp
    $status = [ordered]@{
        WingetAvailable = Test-WingetAvailable
        GitInstalled    = Test-GitAvailable
        VsCppReady      = [bool]$vs
        VsCmakeReady    = if ($vs) { Test-VisualStudioCmakeTools -VsInstallPath $vs } else { $false }
        VsInstallPath   = $vs
    }
    $status.Ready = $status.GitInstalled -and $status.VsCppReady -and $status.VsCmakeReady
    return [pscustomobject]$status
}

function Invoke-WingetInstall {
    param(
        [string]$PackageId,
        [string]$DisplayName,
        [string[]]$ExtraArgs = @()
    )

    Write-Host "Installing $DisplayName via winget..."
    $args = @(
        "install", "-e", "--id", $PackageId,
        "--accept-source-agreements", "--accept-package-agreements",
        "--disable-interactivity"
    ) + $ExtraArgs
    & winget @args
    if ($LASTEXITCODE -ne 0) {
        throw "winget failed to install $DisplayName (exit $LASTEXITCODE)."
    }
}

function Install-HockeyGit {
    if (Test-GitAvailable) { return }

    if (-not (Test-WingetAvailable)) {
        throw "git is not installed and winget is unavailable. Run setup.ps1 on Windows 10/11 with App Installer."
    }

    Invoke-WingetInstall -PackageId $script:GitWingetId -DisplayName "Git for Windows"
    Refresh-GitPath

    if (-not (Test-GitAvailable)) {
        throw "Git was installed but is not on PATH. Open a new terminal and re-run setup.ps1."
    }
}

function Install-HockeyVisualStudio {
    $existing = Find-AnyVisualStudio2022
    $vsInstaller = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vs_installer.exe"

    if ($existing -and (Test-Path $vsInstaller)) {
        if (-not (Test-VisualStudioCmakeTools -VsInstallPath $existing)) {
            Write-Host "Adding C++ and CMake components to Visual Studio 2022..."
            $args = @(
                "modify",
                "--installPath", $existing,
                "--add", $script:VsCppWorkload,
                "--add", $script:VsCmakeComponent,
                "--includeRecommended",
                "--passive", "--norestart", "--wait"
            )
            & $vsInstaller @args
            if ($LASTEXITCODE -ne 0) {
                throw "Visual Studio Installer failed to add C++ workloads (exit $LASTEXITCODE)."
            }
        }
        return
    }

    if (-not (Test-WingetAvailable)) {
        throw "Visual Studio 2022 is not installed and winget is unavailable."
    }

    Write-Host "Installing Visual Studio 2022 Community with C++ workloads (large download, may take a while)..."
    $override = @(
        "--wait", "--passive",
        "--add", $script:VsCppWorkload,
        "--add", $script:VsCmakeComponent,
        "--includeRecommended"
    ) -join " "

    Invoke-WingetInstall -PackageId $script:VsWingetId -DisplayName "Visual Studio 2022 Community" -ExtraArgs @(
        "--override", $override
    )
}

function Install-HockeyPrerequisites {
    $status = Get-HockeyPrerequisiteStatus
    if ($status.Ready) {
        Write-Host "Prerequisites already satisfied."
        return $status
    }

    if (-not (Test-IsAdministrator)) {
        throw "Administrator approval is required to install prerequisites. Re-run setup.ps1 and accept the UAC prompt."
    }

    if (-not $status.GitInstalled) {
        Install-HockeyGit
    }

    if (-not $status.VsCppReady -or -not $status.VsCmakeReady) {
        Install-HockeyVisualStudio
    }

    $status = Get-HockeyPrerequisiteStatus
    if (-not $status.Ready) {
        throw @"
Prerequisite installation finished but the toolchain is still incomplete.
Open a new terminal and re-run: .\scripts\windows\setup.ps1
"@
    }

    return $status
}

function Request-SetupElevation {
    param([string]$SetupScript)

    Write-Host "Administrator approval is required to install missing prerequisites."
    Write-Host "Approve the UAC prompt to continue..."
    $argList = "-NoProfile -ExecutionPolicy Bypass -File `"$SetupScript`" -Elevated"
    $process = Start-Process -FilePath "powershell.exe" -ArgumentList $argList -Verb RunAs -Wait -PassThru
    exit $process.ExitCode
}
