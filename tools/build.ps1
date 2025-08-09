# Wisp Engine Build Script
# Usage: .\tools\build.ps1 -board "waveshare-c6-lcd" [-action build|upload|monitor|clean]
# Example: .\tools\build.ps1 -board "custom-board" -action upload

param(
    [Parameter(Mandatory=$true)]
    [string]$board,
    
    [Parameter(Mandatory=$false)]
    [ValidateSet("build", "upload", "monitor", "clean", "buildupload")]
    [string]$action = "build",
    
    [Parameter(Mandatory=$false)]
    [switch]$verbose,
    
    [Parameter(Mandatory=$false)]
    [switch]$help
)

# Help message
if ($help) {
    Write-Host @"
Wisp Engine Build Script
========================

USAGE:
    .\tools\build.ps1 -board <board-name> [-action <action>] [-verbose]

PARAMETERS:
    -board      Board configuration name (required)
                Looks for configs/boards/<board-name>.config
    
    -action     Build action (optional, default: build)
                build       - Compile the project
                upload      - Compile and upload to device
                monitor     - Open serial monitor
                clean       - Clean build files
                buildupload - Build then upload
    
    -verbose    Enable verbose output
    
    -help       Show this help message

EXAMPLES:
    .\tools\build.ps1 -board "waveshare-c6-lcd"
    .\tools\build.ps1 -board "waveshare-s3-amoled" -action upload
    .\tools\build.ps1 -board "custom-board" -action buildupload -verbose

AVAILABLE BOARDS:
    Run: .\tools\build.ps1 -board "list" to see all available board configs

CUSTOM BOARDS:
    Create a new file: configs/boards/my-board.config
    Then run: .\tools\build.ps1 -board "my-board"
"@
    exit 0
}

# Colors for output
$ErrorColor = "Red"
$WarningColor = "Yellow"
$InfoColor = "Cyan"
$SuccessColor = "Green"

function Write-ColorOutput {
    param($Message, $Color = "White")
    Write-Host $Message -ForegroundColor $Color
}

function Show-Header {
    Write-ColorOutput "============================================" $InfoColor
    Write-ColorOutput "       Wisp Engine Build System" $InfoColor
    Write-ColorOutput "============================================" $InfoColor
    Write-ColorOutput "Board: $board" $InfoColor
    Write-ColorOutput "Action: $action" $InfoColor
    Write-ColorOutput ""
}

# List available boards
if ($board -eq "list") {
    Write-ColorOutput "Available Board Configurations:" $InfoColor
    Write-ColorOutput "===============================" $InfoColor
    
    $configPath = "configs/boards"
    if (Test-Path $configPath) {
        $configs = Get-ChildItem "$configPath/*.config" -ErrorAction SilentlyContinue
        if ($configs) {
            foreach ($config in $configs) {
                $boardName = $config.BaseName
                Write-ColorOutput "  $boardName" $SuccessColor
                
                # Try to read board description from config
                $firstLine = Get-Content $config.FullName -TotalCount 1 -ErrorAction SilentlyContinue
                if ($firstLine -and $firstLine.StartsWith("#") -and $firstLine.Contains("Description:")) {
                    $description = $firstLine -replace "^#.*Description:\s*", ""
                    Write-ColorOutput "    $description" "Gray"
                }
            }
        } else {
            Write-ColorOutput "  No .config files found in $configPath" $WarningColor
        }
        
        # Also list .h files as fallback
        $hConfigs = Get-ChildItem "$configPath/*.h" -ErrorAction SilentlyContinue
        if ($hConfigs) {
            Write-ColorOutput ""
            Write-ColorOutput "Legacy .h configurations:" $WarningColor
            foreach ($config in $hConfigs) {
                $boardName = $config.BaseName -replace "_config$", ""
                Write-ColorOutput "  $boardName (legacy)" "Gray"
            }
        }
    } else {
        Write-ColorOutput "Error: configs/boards directory not found!" $ErrorColor
    }
    exit 0
}

Show-Header

# Validate board configuration exists
$configFile = "configs/boards/$board.config"
$legacyConfigFile = "configs/boards/$board" + "_config.h"

$boardConfigExists = $false
$configContent = @{}

if (Test-Path $configFile) {
    Write-ColorOutput "✓ Found board config: $configFile" $SuccessColor
    $boardConfigExists = $true
    
    # Parse .config file
    try {
        $content = Get-Content $configFile -ErrorAction Stop
        foreach ($line in $content) {
            $line = $line.Trim()
            if ($line -and !$line.StartsWith("#") -and $line.Contains("=")) {
                $parts = $line -split "=", 2
                if ($parts.Length -eq 2) {
                    $key = $parts[0].Trim()
                    $value = $parts[1].Trim()
                    $configContent[$key] = $value
                }
            }
        }
        Write-ColorOutput "✓ Parsed $($configContent.Count) configuration options" $SuccessColor
    }
    catch {
        Write-ColorOutput "✗ Error parsing config file: $_" $ErrorColor
        exit 1
    }
}
elseif (Test-Path $legacyConfigFile) {
    Write-ColorOutput "✓ Found legacy config: $legacyConfigFile" $WarningColor
    Write-ColorOutput "  Consider converting to .config format" $WarningColor
    $boardConfigExists = $true
    
    # For legacy .h files, set minimal config
    $configContent["PLATFORM"] = "ESP32"  # Will be overridden by parsing
    $configContent["BOARD_NAME"] = $board
}
else {
    Write-ColorOutput "✗ Board config not found: $configFile" $ErrorColor
    Write-ColorOutput "✗ Legacy config not found: $legacyConfigFile" $ErrorColor
    Write-ColorOutput ""
    Write-ColorOutput "Available boards:" $InfoColor
    & $MyInvocation.MyCommand.Path -board "list"
    exit 1
}

# Build environment variables from config
$envVars = @{}
$buildFlags = @()

foreach ($key in $configContent.Keys) {
    $value = $configContent[$key]
    
    # Convert config values to environment variables and build flags
    if ($key.StartsWith("DEFINE_")) {
        # Direct defines: DEFINE_FEATURE=1 -> -DFEATURE=1
        $defineName = $key -replace "^DEFINE_", ""
        $buildFlags += "-D$defineName=$value"
    }
    elseif ($key.StartsWith("ENV_")) {
        # Environment variables: ENV_PLATFORM=ESP32C6 -> PLATFORM=ESP32C6
        $envName = $key -replace "^ENV_", ""
        $envVars[$envName] = $value
    }
    else {
        # Default: treat as define
        $buildFlags += "-D$key=$value"
    }
}

# Add board identification
$buildFlags += "-DWISP_BOARD_NAME=`"$board`""
$buildFlags += "-DWISP_BOARD_CONFIG=`"$board.config`""

# Set environment variables
foreach ($key in $envVars.Keys) {
    $env:$key = $envVars[$key]
    if ($verbose) {
        Write-ColorOutput "ENV: $key=$($envVars[$key])" "Gray"
    }
}

# Convert build flags to PlatformIO format
$platformioBuildFlags = ($buildFlags -join " ")

if ($verbose) {
    Write-ColorOutput "Build flags: $platformioBuildFlags" "Gray"
}

# Set PlatformIO build flags environment variable
$env:PLATFORMIO_BUILD_FLAGS = $platformioBuildFlags

Write-ColorOutput "✓ Environment configured for board: $board" $SuccessColor

# Execute PlatformIO command
$pioCommand = switch ($action) {
    "build" { "run" }
    "upload" { "run --target upload" }
    "monitor" { "device monitor" }
    "clean" { "run --target clean" }
    "buildupload" { "run --target upload" }
    default { "run" }
}

Write-ColorOutput ""
Write-ColorOutput "Executing: pio $pioCommand" $InfoColor
Write-ColorOutput "============================================" $InfoColor

try {
    if ($verbose) {
        & pio $pioCommand.Split(" ") --verbose
    } else {
        & pio $pioCommand.Split(" ")
    }
    
    if ($LASTEXITCODE -eq 0) {
        Write-ColorOutput ""
        Write-ColorOutput "============================================" $SuccessColor
        Write-ColorOutput "✓ Build completed successfully!" $SuccessColor
        Write-ColorOutput "============================================" $SuccessColor
    } else {
        Write-ColorOutput ""
        Write-ColorOutput "============================================" $ErrorColor
        Write-ColorOutput "✗ Build failed with code: $LASTEXITCODE" $ErrorColor
        Write-ColorOutput "============================================" $ErrorColor
        exit $LASTEXITCODE
    }
}
catch {
    Write-ColorOutput "✗ Error executing PlatformIO: $_" $ErrorColor
    exit 1
}
