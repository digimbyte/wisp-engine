#!/usr/bin/env pwsh
# build_assets.ps1 - Convert PNG source assets to Wisp Engine formats

param(
    [string]$ExampleDir = "",
    [switch]$All = $false,
    [switch]$Force = $false,
    [switch]$Verbose = $false
)

# Asset conversion tools paths
$SPRITE_CONVERTER = "tools/png_to_spr.exe"
$AUDIO_CONVERTER = "tools/wav_to_wisp.exe"
$LUT_CONVERTER = "tools/png_to_lut.exe"

# Color for output
function Write-Info($msg) { Write-Host "INFO: $msg" -ForegroundColor Cyan }
function Write-Success($msg) { Write-Host "SUCCESS: $msg" -ForegroundColor Green }
function Write-Warning($msg) { Write-Host "WARNING: $msg" -ForegroundColor Yellow }
function Write-Error($msg) { Write-Host "ERROR: $msg" -ForegroundColor Red }

# Check if conversion tools exist
function Test-ConversionTools {
    $missing = @()
    
    if (!(Test-Path $SPRITE_CONVERTER)) {
        $missing += "Sprite converter: $SPRITE_CONVERTER"
    }
    if (!(Test-Path $AUDIO_CONVERTER)) {
        $missing += "Audio converter: $AUDIO_CONVERTER"
    }
    if (!(Test-Path $LUT_CONVERTER)) {
        $missing += "LUT converter: $LUT_CONVERTER"
    }
    
    if ($missing.Count -gt 0) {
        Write-Error "Missing conversion tools:"
        $missing | ForEach-Object { Write-Host "  - $_" -ForegroundColor Red }
        Write-Info "Please build the conversion tools first using: cmake --build build --target tools"
        return $false
    }
    
    return $true
}

# Convert PNG to SPR
function Convert-PngToSpr($pngFile, $sprFile) {
    if ($Verbose) { Write-Info "Converting PNG to SPR: $pngFile -> $sprFile" }
    
    $result = & $SPRITE_CONVERTER $pngFile $sprFile 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Success "Converted: $(Split-Path $sprFile -Leaf)"
        return $true
    } else {
        Write-Error "Failed to convert $pngFile`: $result"
        return $false
    }
}

# Convert WAV to Wisp audio formats
function Convert-WavToWisp($wavFile, $wispFile) {
    if ($Verbose) { Write-Info "Converting WAV to Wisp audio: $wavFile -> $wispFile" }
    
    $extension = [System.IO.Path]::GetExtension($wispFile).ToLower()
    $format = switch ($extension) {
        ".wbgm" { "bgm" }
        ".wsfx" { "sfx" }
        ".wcry" { "cry" }
        default { "sfx" }
    }
    
    $result = & $AUDIO_CONVERTER $wavFile $wispFile --format $format 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Success "Converted: $(Split-Path $wispFile -Leaf)"
        return $true
    } else {
        Write-Error "Failed to convert $wavFile`: $result"
        return $false
    }
}

# Convert PNG palette to LUT
function Convert-PngToLut($pngFile, $lutFile) {
    if ($Verbose) { Write-Info "Converting PNG palette to LUT: $pngFile -> $lutFile" }
    
    $result = & $LUT_CONVERTER $pngFile $lutFile 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Success "Converted: $(Split-Path $lutFile -Leaf)"
        return $true
    } else {
        Write-Error "Failed to convert $pngFile`: $result"
        return $false
    }
}

# Process assets in a directory
function Process-AssetDirectory($assetsDir) {
    Write-Info "Processing assets in: $assetsDir"
    
    if (!(Test-Path $assetsDir)) {
        Write-Warning "Assets directory not found: $assetsDir"
        return
    }
    
    $sourceDir = Join-Path $assetsDir "src"
    if (!(Test-Path $sourceDir)) {
        Write-Warning "No 'src' directory found in $assetsDir - creating it"
        New-Item -ItemType Directory -Path $sourceDir -Force | Out-Null
        Write-Info "Created source directory: $sourceDir"
        Write-Info "Place your PNG/WAV source files in this directory"
        return
    }
    
    $converted = 0
    $failed = 0
    
    # Convert PNG files to SPR
    Get-ChildItem -Path $sourceDir -Filter "*.png" | ForEach-Object {
        $pngFile = $_.FullName
        $sprFile = Join-Path $assetsDir ($_.BaseName + ".spr")
        
        # Check if conversion needed
        if (!$Force -and (Test-Path $sprFile)) {
            $pngTime = (Get-Item $pngFile).LastWriteTime
            $sprTime = (Get-Item $sprFile).LastWriteTime
            if ($sprTime -gt $pngTime) {
                if ($Verbose) { Write-Info "Skipping up-to-date: $($_.Name)" }
                return
            }
        }
        
        if (Convert-PngToSpr $pngFile $sprFile) {
            $converted++
        } else {
            $failed++
        }
    }
    
    # Convert WAV files to Wisp audio formats
    Get-ChildItem -Path $sourceDir -Filter "*.wav" | ForEach-Object {
        $wavFile = $_.FullName
        $baseName = $_.BaseName
        
        # Determine output format based on filename pattern
        $wispFile = if ($baseName -match "bgm") {
            Join-Path $assetsDir ($baseName + ".wbgm")
        } elseif ($baseName -match "cry") {
            Join-Path $assetsDir ($baseName + ".wcry")
        } else {
            Join-Path $assetsDir ($baseName + ".wsfx")
        }
        
        # Check if conversion needed
        if (!$Force -and (Test-Path $wispFile)) {
            $wavTime = (Get-Item $wavFile).LastWriteTime
            $wispTime = (Get-Item $wispFile).LastWriteTime
            if ($wispTime -gt $wavTime) {
                if ($Verbose) { Write-Info "Skipping up-to-date: $($_.Name)" }
                return
            }
        }
        
        if (Convert-WavToWisp $wavFile $wispFile) {
            $converted++
        } else {
            $failed++
        }
    }
    
    # Convert palette PNG files to LUT
    Get-ChildItem -Path $sourceDir -Filter "*_palette.png" | ForEach-Object {
        $pngFile = $_.FullName
        $lutFile = Join-Path $assetsDir ($_.BaseName.Replace("_palette", "") + ".wlut")
        
        # Check if conversion needed
        if (!$Force -and (Test-Path $lutFile)) {
            $pngTime = (Get-Item $pngFile).LastWriteTime
            $lutTime = (Get-Item $lutFile).LastWriteTime
            if ($lutTime -gt $pngTime) {
                if ($Verbose) { Write-Info "Skipping up-to-date: $($_.Name)" }
                return
            }
        }
        
        if (Convert-PngToLut $pngFile $lutFile) {
            $converted++
        } else {
            $failed++
        }
    }
    
    if ($converted -gt 0) {
        Write-Success "Converted $converted assets in $(Split-Path $assetsDir -Leaf)"
    }
    if ($failed -gt 0) {
        Write-Error "Failed to convert $failed assets in $(Split-Path $assetsDir -Leaf)"
    }
}

# Main execution
function Main {
    Write-Info "Wisp Engine Asset Builder"
    Write-Info "Converting source assets to engine formats..."
    
    # Check tools
    if (!(Test-ConversionTools)) {
        exit 1
    }
    
    # Process specific example or all examples
    if ($ExampleDir) {
        $targetDir = Join-Path "examples" $ExampleDir
        if (Test-Path $targetDir) {
            $assetsDir = Join-Path $targetDir "assets"
            Process-AssetDirectory $assetsDir
        } else {
            Write-Error "Example directory not found: $targetDir"
            exit 1
        }
    } elseif ($All) {
        # Process all example directories
        Get-ChildItem -Path "examples" -Directory | ForEach-Object {
            $assetsDir = Join-Path $_.FullName "assets"
            if (Test-Path $assetsDir) {
                Process-AssetDirectory $assetsDir
            }
        }
    } else {
        Write-Info "Usage examples:"
        Write-Info "  .\build_assets.ps1 -ExampleDir sprite_test    # Build assets for sprite_test"
        Write-Info "  .\build_assets.ps1 -All                       # Build all example assets"
        Write-Info "  .\build_assets.ps1 -All -Force                # Force rebuild all assets"
        Write-Info "  .\build_assets.ps1 -All -Verbose              # Verbose output"
        Write-Info ""
        Write-Info "Available example directories:"
        Get-ChildItem -Path "examples" -Directory | ForEach-Object {
            $assetsDir = Join-Path $_.FullName "assets"
            $hasAssets = if (Test-Path $assetsDir) { " (has assets)" } else { "" }
            Write-Info "  - $($_.Name)$hasAssets"
        }
    }
}

# Run main function
Main
