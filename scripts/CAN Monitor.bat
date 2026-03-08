@echo off
setlocal
title Taktflow CAN Bus Monitor

rem Run from repository root regardless of where the .bat is started.
set "REPO_ROOT=%~dp0.."
pushd "%REPO_ROOT%" >nul 2>&1
if errorlevel 1 (
  echo [ERROR] Unable to enter repository root: "%REPO_ROOT%"
  pause
  exit /b 1
)

set "PYTHON_CMD="
where py >nul 2>&1 && set "PYTHON_CMD=py -3"
if not defined PYTHON_CMD (
  where python >nul 2>&1 && set "PYTHON_CMD=python"
)

if not defined PYTHON_CMD (
  echo [ERROR] Python was not found in PATH.
  echo Install Python 3 and retry.
  popd >nul
  pause
  exit /b 1
)

set "LAUNCH_SCRIPT=scripts\can_monitor_gui.pyw"
if not exist "%LAUNCH_SCRIPT%" (
  echo [ERROR] Launch script not found: "%LAUNCH_SCRIPT%"
  popd >nul
  pause
  exit /b 1
)

%PYTHON_CMD% "%LAUNCH_SCRIPT%" %*
set "EXIT_CODE=%ERRORLEVEL%"

if not "%EXIT_CODE%"=="0" (
  echo [ERROR] CAN Monitor exited with code %EXIT_CODE%.
  pause
)

popd >nul
endlocal & exit /b %EXIT_CODE%
