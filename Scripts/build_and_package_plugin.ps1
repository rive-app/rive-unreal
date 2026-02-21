<#
    Script: build_and_package_plugin.ps1
    Purpose: Packages the Rive Unreal plugin into one or more zip archives,
             tailored for distribution via GitHub and/or Fab.

    1) Original Rive.uplugin is a dev canonical file:
       - MUST NOT contain FabURL or EngineVersion (we strip them in-place).
    2) If building for Fab (Fab or Both), -UnrealEngineVersion is REQUIRED.
       - AND it MUST be base engine version only (e.g. 5.7), NOT a patch (e.g. 5.7.3).
    3) Original "Version" (integer) MUST be monotonically incremented once per run,
       and BOTH output zips MUST use the same incremented "Version".

    Example usage:
    .\build_and_package_plugin.ps1 -Version 0.4.20 -Distribution Both -UnrealEngineVersion 5.7 -UEPath "C:\Program Files\Epic Games\UE_5.7\"
#>

param (
    [string]$Directory = ".",
    [string]$Output = "Rive",
    [string]$Version = "",
    [string]$UnrealEngineVersion = "",
    [ValidateSet("Fab", "Github", "Both")]
    [string]$Distribution = "Both",
    [switch]$ForceZipOverwrite,
    [switch]$Help,

    # --- Build validation (UAT BuildPlugin) ---
    [string]$UEPath = "",                 # e.g. C:\Program Files\Epic Games\UE_5.7
    [switch]$ValidateBuild,               # run UAT BuildPlugin on staged plugin before zipping
    [ValidateSet("Win64")]
    [string]$ValidatePlatform = "Win64",
    [switch]$KeepUATPackageOutput         # keep the UAT BuildPlugin -package output folder
)

function Show-Help {
@"
Usage: ./build_and_package_plugin.ps1 [-Directory <path>] [-Output <name>] [-Version <ver>] [-Distribution <Fab|Github|Both>] [-Help]
                                      [-UnrealEngineVersion <ver>] [-ValidateBuild] [-UEPath <path>] [-ValidatePlatform <Win64>]
                                      [-KeepUATPackageOutput] [-ForceZipOverwrite]

  -Directory             Root directory of the plugin (default: current dir)
  -Output                Output file base name (default: Rive)
  -Version               Version string like 0.4.20 (updates VersionName in packaged uplugin)
  -UnrealEngineVersion   REQUIRED for Fab/Both. Must be base version only (e.g. 5.7, NOT 5.7.3).
                         Sets EngineVersion in Fab package uplugin.
  -Distribution          Target distribution channel: Fab, Github, Both (default: Both)
  -ForceZipOverwrite     Do not prompt to overwrite existing zip files
  -Help                  Show this help message

Validation:
  -ValidateBuild         Runs RunUAT.bat BuildPlugin on the staged plugin before zipping
  -UEPath                Unreal Engine root folder (required for -ValidateBuild)
  -ValidatePlatform      Platform to validate (default: Win64)
  -KeepUATPackageOutput  Keeps the UAT BuildPlugin -package output folder (for inspection)

Examples:
  .\build_and_package_plugin.ps1 -Version 0.4.20 -Distribution Both -UnrealEngineVersion 5.7
  .\build_and_package_plugin.ps1 -Version 0.4.20 -Distribution Fab  -UnrealEngineVersion 5.7 -ValidateBuild -UEPath "C:\Program Files\Epic Games\UE_5.7"
"@
}

if ($Help) { Show-Help; exit 0 }

Push-Location $Directory

try {
    if (-not (Test-Path "Rive.uplugin")) {
        Write-Error "Rive.uplugin not found in '$Directory'"
        exit 1
    }

    # If building for Fab, EngineVersion MUST be passed
    if (($Distribution -eq "Fab" -or $Distribution -eq "Both") -and [string]::IsNullOrWhiteSpace($UnrealEngineVersion)) {
        Write-Error "Fab packaging requires -UnrealEngineVersion (e.g. -UnrealEngineVersion 5.7)."
        exit 1
    }

    # Enforce ONLY base engine version (Major.Minor), NOT patch version.
    if (($Distribution -eq "Fab" -or $Distribution -eq "Both")) {
        # Enforce strictly Major.Minor (e.g. 5.7)
        if ($UnrealEngineVersion -notmatch '^\d+\.\d+$') {
            Write-Error "Invalid -UnrealEngineVersion '$UnrealEngineVersion'. Must be base engine version only (e.g. 5.7). Patch versions like 5.7.3 are NOT allowed."
            exit 1
        }
    }

    Write-Host "Checking for missing platforms..."
    $RequiredPlatforms = @("Win64", "Android", "IOS", "Mac")
    $Missing = $RequiredPlatforms | Where-Object { -not (Test-Path "Source/ThirdParty/RiveLibrary/Libraries/$_") }
    if ($Missing.Count -gt 0) {
        Write-Warning "Missing Libraries for platform(s): $($Missing -join ', ')"
    }

    $PluginRoot = (Get-Location).Path.TrimEnd('\')
    $OriginalUPluginPath = Join-Path $PluginRoot "Rive.uplugin"

    function Get-NewlineStyle {
        param([string]$Text)
        if ($Text -match "`r`n") { return "`r`n" }
        return "`n"
    }

    function Get-Utf8BomInfo {
        param([string]$Path)
        [byte[]]$bytes = [System.IO.File]::ReadAllBytes($Path)
        $hasBom = $false
        if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
            $hasBom = $true
        }
        return @{ Bytes = $bytes; HasBom = $hasBom }
    }

    function Read-TextPreserveBom {
        param([string]$Path)
        $info = Get-Utf8BomInfo -Path $Path
        # UTF8 decoding will handle BOM if present
        $text = [System.Text.Encoding]::UTF8.GetString($info.Bytes)
        return @{ Text = $text; HasBom = $info.HasBom }
    }

    function Write-TextPreserveBom {
        param(
            [string]$Path,
            [string]$Text,
            [bool]$HasBom
        )
        $enc = New-Object System.Text.UTF8Encoding($HasBom) # emit BOM if original had it
        [byte[]]$outBytes = $enc.GetBytes($Text)
        [System.IO.File]::WriteAllBytes($Path, $outBytes)
    }

    function Remove-TopLevelKeyLine {
        param(
            [string]$Raw,
            [string]$KeyName,
            [string]$NL
        )
        # Remove whole line containing "KeyName": ..., including its newline.
        # Preserves the rest of the file unchanged.
        $pattern = '(?m)^[^\S\r\n]*"' + [regex]::Escape($KeyName) + '"\s*:\s*.*?,?\s*(\r?\n)?'
        return [regex]::Replace($Raw, $pattern, '', 1)
    }

    function Get-VersionIntFromUPlugin {
        param([string]$Raw)
        $m = [regex]::Match($Raw, '(?m)^[^\S\r\n]*"Version"\s*:\s*(\d+)')
        if (-not $m.Success) { throw "Could not find `"Version`" integer in .uplugin." }
        return [int]$m.Groups[1].Value
    }

    function Set-VersionIntInUPlugin {
        param(
            [string]$Raw,
            [int]$NewVersionInt
        )
        return [regex]::Replace(
            $Raw,
            '(?m)^(?<lead>[^\S\r\n]*"Version"\s*:\s*)(?<num>\d+)',
            { param($m) $m.Groups['lead'].Value + $NewVersionInt },
            1
        )
    }

    function Set-VersionNameInUPlugin {
        param(
            [string]$Raw,
            [string]$NewVersionName
        )
        return [regex]::Replace(
            $Raw,
            '(?m)^(?<lead>[^\S\r\n]*"VersionName"\s*:\s*)".*?"',
            { param($m) $m.Groups['lead'].Value + '"' + $NewVersionName + '"' },
            1
        )
    }

    function Upsert-EngineVersionForFabPackage {
        param(
            [string]$Raw,
            [string]$EngineVersion,
            [string]$NL
        )

        # If EngineVersion exists: replace value in place, preserving indentation/prefix.
        if ($Raw -match '(?m)^(?<lead>[^\S\r\n]*"EngineVersion"\s*:\s*)".*?"(?<tail>\s*,?\s*)$') {
            $Raw = [regex]::Replace(
                $Raw,
                '(?m)^(?<lead>[^\S\r\n]*"EngineVersion"\s*:\s*)".*?"(?<tail>\s*,?\s*)$',
                { param($m) $m.Groups['lead'].Value + '"' + $EngineVersion + '"' + $m.Groups['tail'].Value },
                1
            )
            return $Raw
        }

        # Else insert EngineVersion immediately before MarketplaceURL using MarketplaceURL's indentation
        $Raw = [regex]::Replace(
            $Raw,
            '(?m)^(?<indent>[^\S\r\n]*)"MarketplaceURL"\s*:\s*',
            {
                param($m)
                $indent = $m.Groups['indent'].Value
                $insert = $indent + '"EngineVersion": "' + $EngineVersion + '",' + $NL
                return $insert + $m.Value  # DO NOT re-emit MarketplaceURL; preserve it
            },
            1
        )
        return $Raw
    }

    function Ensure-FabURLForFabPackage {
        param(
            [string]$Raw,
            [string]$NL
        )
        if ($Raw -match '(?m)^[^\S\r\n]*"FabURL"\s*:') {
            return $Raw
        }

        $FabValue = 'com.epicgames.launcher://ue/Fab/product/4a79a0af-7614-42bd-8857-5375c2668706'

        $Raw = [regex]::Replace(
            $Raw,
            '(?m)^(?<indent>[^\S\r\n]*)"MarketplaceURL"\s*:\s*',
            {
                param($m)
                $indent = $m.Groups['indent'].Value
                $insert = $indent + '"FabURL": "' + $FabValue + '",' + $NL
                return $insert + $m.Value  # preserve MarketplaceURL line untouched
            },
            1
        )
        return $Raw
    }

    # === Requirement #3: bump Version in ORIGINAL and strip FabURL/EngineVersion from ORIGINAL ===
    $origInfo = Read-TextPreserveBom -Path $OriginalUPluginPath
    $origRaw  = $origInfo.Text
    $origNL   = Get-NewlineStyle -Text $origRaw

    $currentVersionInt = Get-VersionIntFromUPlugin -Raw $origRaw
    $newVersionInt = $currentVersionInt + 1

    # Remove store-only keys from ORIGINAL (requirement #1)
    $origRaw = Remove-TopLevelKeyLine -Raw $origRaw -KeyName "FabURL" -NL $origNL
    $origRaw = Remove-TopLevelKeyLine -Raw $origRaw -KeyName "EngineVersion" -NL $origNL

    # Bump Version (requirement #3)
    $origRaw = Set-VersionIntInUPlugin -Raw $origRaw -NewVersionInt $newVersionInt

    # Optional but useful: keep VersionName in original aligned with -Version if provided (no suffix)
    if (-not [string]::IsNullOrWhiteSpace($Version)) {
        $origRaw = Set-VersionNameInUPlugin -Raw $origRaw -NewVersionName $Version
    }

    # Write ORIGINAL back now so it is the source of truth for staging/packages.
    Write-TextPreserveBom -Path $OriginalUPluginPath -Text $origRaw -HasBom $origInfo.HasBom
    Write-Host "Updated original Rive.uplugin: incremented Version to $newVersionInt; removed FabURL/EngineVersion."

    # === Collect plugin files (skip dotfiles, Intermediate, Binaries, Scripts) ===
    $ExcludeRootFolders = @("Scripts", "Intermediate", "Binaries")
    $ExcludeFilePatterns = @("*.zip", ".*")

    $AllItems = Get-ChildItem -Recurse -File | Where-Object {
        $fullPath = $_.FullName
        $relativePath = $fullPath.Substring($PluginRoot.Length + 1)
        $pathParts = $relativePath -split "[/\\]"

        if ($pathParts.Length -ge 2 -and $ExcludeRootFolders -contains $pathParts[0]) {
            return $false
        }

        foreach ($pattern in $ExcludeFilePatterns) {
            if ($_.Name -like $pattern) { return $false }
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

    $StagedUPluginPath = Join-Path $TempDir "Rive.uplugin"

    # Load staged baseline (should match the just-updated original, but we treat staged as its own base)
    $stagedInfo = Read-TextPreserveBom -Path $StagedUPluginPath
    $BaseStagedRaw = $stagedInfo.Text
    $StagedNL = Get-NewlineStyle -Text $BaseStagedRaw

    function Write-StagedBase {
        # Reset staged uplugin to baseline before each variant so Both doesn't compound edits
        Write-TextPreserveBom -Path $StagedUPluginPath -Text $BaseStagedRaw -HasBom $stagedInfo.HasBom
    }

    function Update-UPluginForVariant {
        param(
            [string]$UPluginPath,
            [string]$VersionString,
            [string]$VersionNameSuffix,
            [bool]$IsFabVersion,
            [string]$EngineVersion,
            [int]$FixedVersionInt
        )

        $info = Read-TextPreserveBom -Path $UPluginPath
        $Raw = $info.Text
        $NL  = Get-NewlineStyle -Text $Raw

        # Version int must be identical across variants within a single run
        $Raw = Set-VersionIntInUPlugin -Raw $Raw -NewVersionInt $FixedVersionInt

        # VersionName is driven by -Version + suffix (if provided). If not provided, leave untouched.
        if (-not [string]::IsNullOrWhiteSpace($VersionString)) {
            $Raw = Set-VersionNameInUPlugin -Raw $Raw -NewVersionName ($VersionString + $VersionNameSuffix)
        }

        if ($IsFabVersion) {
            # Fab requires EngineVersion be passed (enforced earlier)
            $Raw = Ensure-FabURLForFabPackage -Raw $Raw -NL $NL
            $Raw = Upsert-EngineVersionForFabPackage -Raw $Raw -EngineVersion $EngineVersion -NL $NL
        }
        else {
            # Github variant: ensure these are not present
            $Raw = Remove-TopLevelKeyLine -Raw $Raw -KeyName "FabURL" -NL $NL
            $Raw = Remove-TopLevelKeyLine -Raw $Raw -KeyName "EngineVersion" -NL $NL
        }

        Write-TextPreserveBom -Path $UPluginPath -Text $Raw -HasBom $info.HasBom
    }

    function Invoke-RunUATBuildPlugin {
        param(
            [string]$UEPath,
            [string]$PluginFile,
            [string]$Platform = "Win64",
            [switch]$KeepOutput
        )

        if (-not $UEPath) {
            Write-Error "-UEPath is required when using -ValidateBuild (e.g. C:\Program Files\Epic Games\UE_5.7)"
            exit 1
        }

        $UEPath = $UEPath.Trim().Trim('"').Trim("'")
        $RunUAT = Join-Path $UEPath "Engine\Build\BatchFiles\RunUAT.bat"
        if (-not (Test-Path $RunUAT)) {
            Write-Error "RunUAT.bat not found at: $RunUAT"
            exit 1
        }

        if (-not (Test-Path $PluginFile)) {
            Write-Error "Staged .uplugin not found at: $PluginFile"
            exit 1
        }

        $UATOutDir = Join-Path $env:TEMP ("Rive_UAT_" + [System.Guid]::NewGuid().ToString())
        New-Item -ItemType Directory -Path $UATOutDir | Out-Null

        Write-Host "Validating plugin build via UAT..."
        Write-Host "  UEPath:   $UEPath"
        Write-Host "  Plugin:   $PluginFile"
        Write-Host "  Package:  $UATOutDir"
        Write-Host "  Platform: $Platform"

        $p = Start-Process `
            -FilePath $RunUAT `
            -ArgumentList @(
                "BuildPlugin",
                "-plugin=$PluginFile",
                "-package=$UATOutDir",
                "-TargetPlatforms=$Platform"
            ) `
            -NoNewWindow `
            -Wait `
            -PassThru

        if ($p.ExitCode -ne 0) {
            Write-Error "UAT BuildPlugin failed with exit code $($p.ExitCode). See output above. Package output: $UATOutDir"
            exit 1
        }

        Write-Host "UAT BuildPlugin validation succeeded."

        if (-not $KeepOutput) {
            Remove-Item $UATOutDir -Recurse -Force
        } else {
            Write-Host "Kept UAT output at: $UATOutDir"
        }
    }

    function Package-Zip {
        param (
            [string]$Suffix,
            [bool]$IsFabVersion
        )

        # Reset staged uplugin to baseline for this variant
        Write-StagedBase

        # Apply variant edits
        Update-UPluginForVariant `
            -UPluginPath $StagedUPluginPath `
            -VersionString $Version `
            -VersionNameSuffix $Suffix `
            -IsFabVersion $IsFabVersion `
            -EngineVersion $UnrealEngineVersion `
            -FixedVersionInt $newVersionInt

        $Name = "$Output"
        if ($Version) { $Name += "-$Version" }
        $Name += $Suffix
        $ZipPath = "$Name.zip"

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
        $sw = [System.Diagnostics.Stopwatch]::StartNew()
        Compress-Archive -Path "$TempDir\*" -DestinationPath $ZipPath -Force
        $sw.Stop()

        Write-Host "Compressed to $ZipPath in $($sw.Elapsed.TotalSeconds.ToString("0.00")) seconds"
    }

    # Validate once on the baseline staged plugin before zipping any variants
    if ($ValidateBuild) {
        Invoke-RunUATBuildPlugin `
            -UEPath $UEPath `
            -PluginFile $StagedUPluginPath `
            -Platform $ValidatePlatform `
            -KeepOutput:$KeepUATPackageOutput
    }

    switch ($Distribution) {
        "Fab"    { Package-Zip -Suffix ""   -IsFabVersion $true }
        "Github" { Package-Zip -Suffix "-gh" -IsFabVersion $false }
        "Both" {
            Package-Zip -Suffix "-gh" -IsFabVersion $false
            Package-Zip -Suffix ""   -IsFabVersion $true
        }
        default {
            Write-Error "Invalid value for -Distribution: '$Distribution'. Use 'Fab', 'Github', or 'Both'."
            exit 1
        }
    }

    Remove-Item $TempDir -Recurse -Force
    Write-Host "Plugin packaged successfully."
}
finally {
    Pop-Location
}
