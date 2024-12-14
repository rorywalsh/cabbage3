# modify_vcxproj.ps1

param (
    [string]$projectName,
    [string]$buildDir = "."
)

if (-not $projectName) {
    Write-Host "Usage: .\modify_vcxproj.ps1 -projectName <ProjectName> [-buildDir <BuildDir>]"
    exit 1
}

# Resolve the .vcxproj file path
$vcxprojPath = Join-Path -Path $buildDir -ChildPath "$projectName.vcxproj"

# Check if the file exists
if (-not (Test-Path $vcxprojPath)) {
    Write-Host "Project file not found: $vcxprojPath"
    exit 1
}

Write-Host "Modifying $vcxprojPath"

try {
    # Load the .vcxproj file as XML, handling namespaces
    [xml]$xmlContent = Get-Content $vcxprojPath -ErrorAction Stop

    # Namespace handling (if required)
    $nsmgr = New-Object System.Xml.XmlNamespaceManager($xmlContent.NameTable)
    $nsmgr.AddNamespace("msb", $xmlContent.DocumentElement.NamespaceURI)

    # Find all RuntimeLibrary nodes, considering possible namespaces
    $runtimeLibraryNodes = $xmlContent.SelectNodes("//msb:ClCompile/msb:RuntimeLibrary[text()='MultiThreadedDebug']", $nsmgr)

    if (-not $runtimeLibraryNodes -or $runtimeLibraryNodes.Count -eq 0) {
        Write-Host "No <RuntimeLibrary>MultiThreadedDebug</RuntimeLibrary> nodes found in $vcxprojPath."
        Write-Host "Debugging XML structure..."
        # Display structure for debugging
        $xmlContent.SelectNodes("//msb:ItemDefinitionGroup", $nsmgr) | ForEach-Object {
            Write-Host ("Found ItemDefinitionGroup with Condition: {0}" -f $_.Condition)
        }
        exit 1
    }

    Write-Host "Found $($runtimeLibraryNodes.Count) nodes to modify."

    # Modify each matching node
    foreach ($node in $runtimeLibraryNodes) {
        $node.InnerText = "MultiThreadedDebugDLL"
    }

    # Save the updated XML back to the file
    $xmlContent.Save($vcxprojPath)

    Set-ItemProperty -Path $vcxprojPath -Name IsReadOnly -Value $true
    # Verify the modification
    [xml]$updatedXmlContent = Get-Content $vcxprojPath
    $updatedNodes = $updatedXmlContent.SelectNodes("//msb:ClCompile/msb:RuntimeLibrary[text()='MultiThreadedDebugDLL']", $nsmgr)

    if ($updatedNodes -and $updatedNodes.Count -eq $runtimeLibraryNodes.Count) {
        Write-Host "Modification successful: All nodes updated to MultiThreadedDebugDLL."
    } else {
        Write-Host "Modification partially failed: Not all nodes were updated."
        exit 1
    }

} catch {
    Write-Host "An error occurred while modifying the project file: $_"
    exit 1
}
