# powershell.exe -NoProfile -ExecutionPolicy Bypass -File "./scripts/buildWin.ps1"

# Configure build
cmake -G "Visual Studio 17 2022" -S . -B build

# Build Windows app
$SCRIPT_DIR = Split-Path -Parent $MyInvocation.MyCommand.Definition

powershell -NoProfile -ExecutionPolicy Bypass -File "$SCRIPT_DIR\modify_vcxproj.ps1" -projectName "CabbageApp" -buildDir "./build" -Wait
powershell -NoProfile -ExecutionPolicy Bypass -File "$SCRIPT_DIR\modify_vcxproj.ps1" -projectName "CabbageApp" -buildDir "./build/CabbageApp" -Wait

cmake --build build --config Debug --target CabbageApp
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build Windows app step failed, but continuing..."
}

# Build VST Effect Plugin
cmake --build build --config Debug --target CabbageVST3Effect
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build VST Effect Plugin step failed, but continuing..."
    Write-Host "The plugin copying/install phase breaks this step, but the plugin still builds fine. So we'll just ignore the error for now"

}

# Build VST Effect Plugin
cmake --build build --config Debug --target CabbageVST3Synth
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build VST Effect Plugin step failed, but continuing..."
    Write-Host "The plugin copying/install phase breaks this step, but the plugin still builds fine. So we'll just ignore the error for now"

}