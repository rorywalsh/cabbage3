# modify_vcxproj.ps1

param(
    [string]$projectName,
    [string]$buildDir = "."
)

if (-not $projectName) {
    Write-Host "Usage: .\modify_vcxproj.ps1 -projectName <ProjectName> [-buildDir <BuildDir>]"
    exit 1
}

$vcxprojPath = Join-Path $buildDir "$projectName.vcxproj"

if (-not (Test-Path $vcxprojPath)) {
    Write-Host "Project file not found: $vcxprojPath"
    exit 1
}

Write-Host "Modifying $vcxprojPath"

(Get-Content $vcxprojPath) -replace '<RuntimeLibrary>MultiThreadedDebug</RuntimeLibrary>', '<RuntimeLibrary>MultiThreadedDebugDLL</RuntimeLibrary>' | Set-Content $vcxprojPath