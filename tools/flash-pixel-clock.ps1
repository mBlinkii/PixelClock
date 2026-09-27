param(
  [ValidateSet("All", "Firmware", "Web")]
  [string]$Target = "All",

  [string]$Port = "",

  [switch]$Monitor,
  [switch]$SkipBuild,
  [switch]$Clean,
  [switch]$ListPorts,
  [switch]$ShowHelp
)

$ErrorActionPreference = "Stop"

function Write-Step($Text) {
  Write-Host ""
  Write-Host "==> $Text" -ForegroundColor Cyan
}

function Write-Usage {
  Write-Host "Pixel Clock Flash Tool"
  Write-Host ""
  Write-Host "Usage:"
  Write-Host "  .\tools\flash-pixel-clock.ps1"
  Write-Host "  .\tools\flash-pixel-clock.ps1 -Port COM5"
  Write-Host "  .\tools\flash-pixel-clock.ps1 -Target Firmware -Port COM5"
  Write-Host "  .\tools\flash-pixel-clock.ps1 -Target Web"
  Write-Host "  .\tools\flash-pixel-clock.ps1 -ListPorts"
  Write-Host ""
  Write-Host "Options:"
  Write-Host "  -Target All|Firmware|Web   Flash both firmware and LittleFS, firmware only, or web UI only."
  Write-Host "  -Port COMx                 Optional serial port. If omitted, PlatformIO auto-detects."
  Write-Host "  -Monitor                   Open serial monitor after flashing."
  Write-Host "  -SkipBuild                 Skip the separate build step."
  Write-Host "  -Clean                     Clean before building/flashing."
}

function Find-Pio {
  $commands = @("pio", "platformio")
  foreach ($command in $commands) {
    $found = Get-Command $command -ErrorAction SilentlyContinue
    if ($found) {
      return $found.Source
    }
  }

  $userPio = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\pio.exe"
  if (Test-Path $userPio) {
    return $userPio
  }

  throw "PlatformIO wurde nicht gefunden. Installiere PlatformIO oder starte einmal die PlatformIO IDE."
}

function Show-Ports {
  Write-Step "Serielle Ports"
  $ports = [System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object
  if ($ports.Count -eq 0) {
    Write-Host "Keine COM-Ports gefunden."
    return
  }

  foreach ($portName in $ports) {
    Write-Host "  $portName"
  }
}

function Invoke-Pio($Arguments) {
  Write-Host "pio $($Arguments -join ' ')" -ForegroundColor DarkGray
  & $script:PioPath @Arguments
  if ($LASTEXITCODE -ne 0) {
    throw "PlatformIO-Befehl fehlgeschlagen: pio $($Arguments -join ' ')"
  }
}

if ($ShowHelp) {
  Write-Usage
  exit 0
}

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $repoRoot

if ($ListPorts) {
  Show-Ports
  exit 0
}

$script:PioPath = Find-Pio

Write-Host "Pixel Clock Flash Tool" -ForegroundColor Green
Write-Host "Projekt: $repoRoot"
Write-Host "PlatformIO: $script:PioPath"
if ($Port) {
  Write-Host "Port: $Port"
} else {
  Write-Host "Port: automatisch"
}
Write-Host "Ziel: $Target"

if ($Clean) {
  Write-Step "Clean"
  Invoke-Pio -Arguments @("run", "--target", "clean")
}

if (-not $SkipBuild) {
  Write-Step "Build"
  Invoke-Pio -Arguments @("run")
}

$uploadArgs = @("run")
if ($Port) {
  $uploadArgs += @("--upload-port", $Port)
}

if ($Target -eq "All" -or $Target -eq "Firmware") {
  Write-Step "Firmware flashen"
  Invoke-Pio ($uploadArgs + @("--target", "upload"))
}

if ($Target -eq "All" -or $Target -eq "Web") {
  Write-Step "Weboberflaeche nach LittleFS flashen"
  Invoke-Pio ($uploadArgs + @("--target", "uploadfs"))
}

if ($Monitor) {
  Write-Step "Seriellen Monitor starten"
  $monitorArgs = @("device", "monitor")
  if ($Port) {
    $monitorArgs += @("--port", $Port)
  }
  Invoke-Pio $monitorArgs
}

Write-Host ""
Write-Host "Fertig." -ForegroundColor Green
