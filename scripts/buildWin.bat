@echo off

:: Configure build
cmake -G "Visual Studio 17 2022" -S . -B build

:: Build Windows app
set SCRIPT_DIR=%~dp0

powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%\modify_vcxproj.ps1" -projectName "CabbageApp" -buildDir "./build"
cmake --build build --config Debug --target CabbageApp
if %errorlevel% neq 0 (
    echo "Build Windows app step failed, but continuing..."
)

:: Build VST Effect Plugin
echo "The plugin copying/install phase breaks this step, but the plugin still builds fine. So we'll just ignore the error for now"
cmake --build build --config Debug --target CabbageVST3Effect
if %errorlevel% neq 0 (
    echo "Build VST Effect Plugin step failed, but continuing..."
)

:: Build VST Effect Plugin
echo "The plugin copying/install phase breaks this step, but the plugin still builds fine. So we'll just ignore the error for now"
cmake --build build --config Debug --target CabbageVST3Synth
if %errorlevel% neq 0 (
    echo "Build VST Effect Plugin step failed, but continuing..."
)