<#
    install_wsl.ps1
    Safe, interactive PowerShell script to enable WSL features and set WSL2 as default.
    Run as Administrator.
#>

param(
    [switch]$InstallDistro
)

function Prompt-YesNo($msg) {
    do {
        $r = Read-Host "$msg [y/n]"
    } while ($r -notmatch '^[yYnN]$')
    return $r -match '^[yY]$'
}

Write-Host "WSL install helper — run as Administrator" -ForegroundColor Cyan

if (-not ([bool] (Get-CimInstance -ClassName Win32_OperatingSystem).Caption)) {
    Write-Host "Unable to determine OS. Proceeding anyway..." -ForegroundColor Yellow
}

if (-not (Prompt-YesNo "This script will enable WSL and VirtualMachinePlatform features and set WSL2 as default. Continue?")) {
    Write-Host "Aborted by user." -ForegroundColor Yellow
    exit 1
}

Write-Host "Enabling WSL feature..." -ForegroundColor Green
try {
    dism.exe /online /enable-feature /featurename:Microsoft-Windows-Subsystem-Linux /all /norestart | Out-Null
    Write-Host "WSL feature enabled." -ForegroundColor Green
} catch {
    Write-Host "Failed to enable WSL feature: $_" -ForegroundColor Red
}

Write-Host "Enabling VirtualMachinePlatform..." -ForegroundColor Green
try {
    dism.exe /online /enable-feature /featurename:VirtualMachinePlatform /all /norestart | Out-Null
    Write-Host "VirtualMachinePlatform enabled." -ForegroundColor Green
} catch {
    Write-Host "Failed to enable VirtualMachinePlatform: $_" -ForegroundColor Red
}

Write-Host "Setting WSL2 as default version..." -ForegroundColor Green
try {
    wsl --set-default-version 2
    Write-Host "WSL default version set to 2." -ForegroundColor Green
} catch {
    Write-Host "Could not set default WSL version to 2 via wsl.exe. You may need to install the WSL2 kernel update package from Microsoft." -ForegroundColor Yellow
}

if ($InstallDistro) {
    Write-Host "Installing Ubuntu 22.04 via wsl --install -d ubuntu-22.04" -ForegroundColor Green
    try {
        wsl --install -d ubuntu-22.04
    } catch {
        Write-Host "Failed to install distro via wsl --install. Try installing from Microsoft Store." -ForegroundColor Yellow
    }
} else {
    if (Prompt-YesNo "Do you want to install Ubuntu 22.04 now? (If yes, the script will try 'wsl --install -d ubuntu-22.04')") {
        try {
            wsl --install -d ubuntu-22.04
        } catch {
            Write-Host "Failed to install distro via wsl --install. Try installing from Microsoft Store." -ForegroundColor Yellow
        }
    } else {
        Write-Host "Skipped distro install. Install a distro from the Microsoft Store or run 'wsl --install -d <distro>' later." -ForegroundColor Cyan
    }
}

Write-Host "If a reboot is required, please reboot now to complete feature installation." -ForegroundColor Cyan
Write-Host "After reboot, open the installed distro and complete user setup, then run: 'wsl -l -v' to verify." -ForegroundColor Cyan

Write-Host "Done." -ForegroundColor Green
