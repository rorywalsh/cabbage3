@echo off
rem Get the directory of the batch file
set SCRIPT_DIR=%~dp0

rem Call the PowerShell script with the correct full path
powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%\modify_vcxproj.ps1" -projectName "CabbageApp" -buildDir "./build"

rem Pause the terminal to view any output (optional)
pause
