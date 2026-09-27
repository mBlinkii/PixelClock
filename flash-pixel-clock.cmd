@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\flash-pixel-clock.ps1" %*
if errorlevel 1 (
  echo.
  pause
  exit /b %errorlevel%
)
endlocal
