# eventually use this to find UE install
param (
    [string]$UnrealPath,
    [string]$OutputDir
)

$AbsUnrealPath = [System.IO.Path]::GetFullPath("$UnrealPath")
$AbsPluginPath = [System.IO.Path]::GetFullPath("$PSScriptRoot\..\Rive.uplugin")
$AbsOutputDir = [System.IO.Path]::GetFullPath($OutputDir)

if (-not (Test-Path "$AbsUnrealPath\RunUAT.bat")) {
    $AbsUnrealPath = [System.IO.Path]::GetFullPath("$UnrealPath\Engine\Build\BatchFiles")
    if (-not (Test-Path "$AbsUnrealPath\RunUAT.bat")) {
        Write-Host "Unreal Engine path not found: $AbsUnrealPath"
        exit 1
    }
}
Write-Host "Unreal Engine path: $AbsUnrealPath"
Write-Host "Packaging plugin $AbsPluginPath to $AbsOutputDir"
Write-Host "Running $AbsUnrealPath\RunUAT.bat -Verbose BuildPlugin -Plugin=$AbsPluginPath -Package=$AbsOutputDir -precompile $args"
& $AbsUnrealPath\RunUAT.bat -Verbose BuildPlugin -Plugin="$AbsPluginPath" -Package="$AbsOutputDir" -precompile $args