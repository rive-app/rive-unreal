<#
    Script: build_and_package_plugin.ps1
    Purpose: Packages the Rive Unreal plugin into one or more zip archives,
             tailored for distribution via GitHub and/or Fab.

    Steps:
    1. Parse command-line parameters (e.g. Output name, Version, Distribution type).
    2. Validate working directory and locate the Rive.uplugin file.
    3. Check for required platform library folders (Win64, Android, IOS, Mac).
    4. Recursively collect plugin files, excluding:
        - Hidden files
        - .zip files
        - Root-level folders like Scripts, Intermediate, Binaries
    5. Copy filtered plugin content into a temp staging folder ($TempDir).
    6. For each distribution target:
        a. Modify Rive.uplugin inside $TempDir:
            - Add or remove the "FabURL" key
            - Add or remove the "EngineVersion" key (if provided)
            - Update the "Version" and "VersionName" fields
        b. Confirm whether a zip file with the same name already exists.
        c. Optionally delete the old zip or prompt the user.
        d. Zip the contents of $TempDir into Rive-<version>.zip or Rive-<version>-gh.zip.
    7. Clean up the temporary folder.
    8. Output a success message for each built package.

    Supports:
    - -Distribution Fab      adds FabURL and EngineVersion
    - -Distribution Github   removes FabURL and EngineVersion
    - -Distribution Both     builds both variants

    Example usage:
    .\build_and_package_plugin.ps1 -Version 0.3.5a -Distribution Both -UnrealEngineVersion 5.4.2
#>

param (
    [string]$Directory = ".",
    [string]$Output = "Rive",
    [string]$Version = "",
	[string]$UnrealEngineVersion = "",
    [ValidateSet("Fab", "Github", "Both")]
    [string]$Distribution = "Both",
    [switch]$ForceZipOverwrite,
    [switch]$Help
)

function Show-Help {
    @"
Usage: ./package_plugin.ps1 [-Directory <path>] [-Output <name>] [-Version <ver>] [-Distribution <Fab|Github|Both>] [-Help]

  -Directory            Root directory of the plugin (default: current dir)
  -Output               Output file base name (default: Rive)
  -Version              Version string like 0.3.5a (updates uplugin)
  -UnrealEngineVersion  Specifies target Unreal Engine version for Fab packaging
  -Distribution         Target distribution channel: Fab, Github, Both (default: Both)
  -ForceZipOverwrite    Do not prompt to overwrite existing zip files
  -Help                 Show this help message
"@
}

if ($Help) {
    Show-Help
    exit 0
}

Set-Location $Directory

if (-not (Test-Path "Rive.uplugin")) {
    Write-Error "Rive.uplugin not found in '$Directory'"
    exit 1
}

Write-Host "Checking for missing platforms..."
$RequiredPlatforms = @("Win64", "Android", "IOS", "Mac")
$Missing = $RequiredPlatforms | Where-Object { -not (Test-Path "Source/ThirdParty/RiveLibrary/Libraries/$_") }
if ($Missing.Count -gt 0) {
    Write-Warning "Missing Libraries for platform(s): $($Missing -join ', ')"
}

# === Collect plugin files (skip dotfiles, Intermediate, Binaries, Scripts) ===
$ExcludeRootFolders = @("Scripts", "Intermediate", "Binaries")
$ExcludeFilePatterns = @("*.zip", ".*")
$PluginRoot = (Get-Location).Path.TrimEnd('\')

$AllItems = Get-ChildItem -Recurse -File | Where-Object {
    $fullPath = $_.FullName
    $relativePath = $fullPath.Substring($PluginRoot.Length + 1)
    $pathParts = $relativePath -split "[/\\]"

    if ($pathParts.Length -ge 2 -and $ExcludeRootFolders -contains $pathParts[0]) {
        return $false
    }

    foreach ($pattern in $ExcludeFilePatterns) {
        if ($_.Name -like $pattern) {
            return $false
        }
    }

    return $true
}

Add-Type -AssemblyName System.IO.Compression.FileSystem
$TempDir = Join-Path $env:TEMP ([System.Guid]::NewGuid().ToString())
New-Item -ItemType Directory -Path $TempDir | Out-Null

$AllItems | ForEach-Object {
    $RelativePath = $_.FullName.Substring($PluginRoot.Length + 1)
    $Dest = Join-Path $TempDir $RelativePath
    $DestDir = Split-Path $Dest
    if (-not (Test-Path $DestDir)) {
        New-Item -ItemType Directory -Path $DestDir -Force | Out-Null
    }
    Copy-Item $_.FullName -Destination $Dest -Force
}

function Update-UPlugin {
    param (
        [string]$UPluginPath,
        [string]$Version,
        [string]$VersionNameSuffix,
        [bool]$AddFabUrl = $false,
        [string]$EngineVersion = ""
    )

    $Raw = Get-Content $UPluginPath -Raw

    if ($AddFabUrl) {
        if ($Raw -notmatch '"FabURL"\s*:') {
            Write-Host "Inserting FabURL into .uplugin..."
            $FabLine = '  "FabURL" : "com.epicgames.launcher://ue/Fab/product/4a79a0af-7614-42bd-8857-5375c2668706",'
            $Raw = $Raw -replace '(?m)^\s*"MarketplaceURL"\s*:', "$FabLine`n  `"MarketplaceURL`":"
            Set-Content -Path $UPluginPath -Value $Raw -Encoding UTF8
        }

        if ($EngineVersion -and $Raw -notmatch '"EngineVersion"\s*:') {
            Write-Host "Inserting EngineVersion: $EngineVersion"
            $EngineLine = "  `"EngineVersion`" : `"$EngineVersion`","
            $Raw = $Raw -replace '(?m)^\s*"MarketplaceURL"\s*:', "$EngineLine`n  `"MarketplaceURL`":"
            Set-Content -Path $UPluginPath -Value $Raw -Encoding UTF8
        }
    }
    else {
        if ($Raw -match '"FabURL"\s*:') {
            Write-Host "Removing FabURL from .uplugin..."
            $Raw = $Raw -replace '(?m)^\s*"FabURL"\s*:\s*".*?",?\s*(\r?\n)?', ''
        }
        if ($Raw -match '"EngineVersion"\s*:') {
            Write-Host "Removing EngineVersion from .uplugin..."
            $Raw = $Raw -replace '(?m)^\s*"EngineVersion"\s*:\s*".*?",?\s*(\r?\n)?', ''
        }
        Set-Content -Path $UPluginPath -Value $Raw -Encoding UTF8
    }

    $Json = Get-Content $UPluginPath | ConvertFrom-Json
    $Json.Version = ($Json.Version + 1)
    $Json.VersionName = "$Version$VersionNameSuffix"
    $Json | ConvertTo-Json -Depth 10 | Set-Content $UPluginPath -Encoding UTF8
}

function Package-Zip {
    param (
        [string]$Suffix,
        [bool]$IsFabVersion
    )

    $Name = "$Output"
    if ($Version) { $Name += "-$Version" }
    $Name += $Suffix
    $ZipPath = "$Name.zip"

    $UPluginPath = Join-Path $TempDir "Rive.uplugin"
    Update-UPlugin -UPluginPath $UPluginPath -Version $Version -VersionNameSuffix $Suffix -AddFabUrl:$IsFabVersion -EngineVersion $UnrealEngineVersion

    if (Test-Path $ZipPath) {
        if (-not $ForceZipOverwrite) {
            Write-Host "`nZip file '$ZipPath' already exists."
            $confirmation = Read-Host "Do you want to overwrite it? (y/N)"
            if ($confirmation -notin @('y', 'Y')) {
                Write-Host "Aborted by user."
                exit 1
            }
        }
        Remove-Item $ZipPath -Force
        Write-Host "Deleted existing zip: $ZipPath"
    }

    Write-Host "Compressing plugin folder to $ZipPath..."
    $script:spinnerRunning = $true
    $spinner = @('|', '/', '-', '\\')
    $i = 0

    $runspace = [runspacefactory]::CreateRunspace()
    $runspace.Open()
    $ps = [powershell]::Create()
    $ps.Runspace = $runspace
    $ps.AddScript({
        while ($script:spinnerRunning) {
            Write-Host -NoNewline ("`rCompressing plugin... " + $spinner[$i % $spinner.Length])
            Start-Sleep -Milliseconds 100
            $i++
        }
        Write-Host "`rCompression complete.            "
    }) | Out-Null
    $null = $ps.BeginInvoke()

    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    Compress-Archive -Path "$TempDir\*" -DestinationPath $ZipPath -Force
    $sw.Stop()

    $script:spinnerRunning = $false
    Start-Sleep -Milliseconds 200
    $ps.Dispose()
    $runspace.Close()
    $runspace.Dispose()

    Write-Host "Compressed to $ZipPath in $($sw.Elapsed.TotalSeconds.ToString("0.00")) seconds"
}

switch ($Distribution) {
    "Fab"    { Package-Zip -Suffix "" -IsFabVersion $true }
    "Github" { Package-Zip -Suffix "-gh" -IsFabVersion $false }
    "Both" {
        Package-Zip -Suffix "-gh" -IsFabVersion $false
        Package-Zip -Suffix "" -IsFabVersion $true
    }
    default {
        Write-Error "Invalid value for -Distribution: '$Distribution'. Use 'Fab', 'Github', or 'Both'."
        exit 1
    }
}

Remove-Item $TempDir -Recurse -Force
Write-Host "Plugin packaged successfully."
