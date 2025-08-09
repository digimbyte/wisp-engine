# Wisp Engine Dynamic Build System

## Overview

The Wisp Engine now uses a clean, environment-based build system that automatically configures board settings without cluttering platformio.ini. This is similar to how Node.js projects use `cross-env` or environment files.

## How It Works

### 1. **Board Configuration Files**
Board configurations are stored as simple `.config` files in `configs/boards/`:
```
configs/boards/
├── waveshare-c6-lcd.config        # Waveshare ESP32-C6-LCD-1.47
├── waveshare-s3-amoled.config     # Waveshare ESP32-S3-AMOLED-2.06
├── custom-board.config            # Your custom board
└── ...                            # More boards as needed
```

### 2. **Build Script**
The `tools\build.ps1` script reads the board configuration and sets up the build environment:
```powershell
.\tools\build.ps1 -board "waveshare-c6-lcd" -action build
```

### 3. **Auto-Configuration**
Your source code automatically gets the right configuration via `board_auto_config.h`.

## Usage Examples

### Basic Build
```powershell
# Build for Waveshare C6 LCD board
.\tools\build.ps1 -board "waveshare-c6-lcd"

# Build and upload to Waveshare S3 AMOLED watch
.\tools\build.ps1 -board "waveshare-s3-amoled" -action upload

# Clean build files
.\tools\build.ps1 -board "waveshare-c6-lcd" -action clean
```

### List Available Boards
```powershell
.\tools\build.ps1 -board "list"
```

### Verbose Output
```powershell
.\tools\build.ps1 -board "waveshare-c6-lcd" -verbose
```

## Board Configuration Format

Board configs use a simple key=value format:

```ini
# Description: My Custom ESP32-C6 Board
# Platform: ESP32-C6, Custom features

# Platform Configuration
ENV_TARGET=esp32c6
ENV_BOARD=esp32-c6-devkitm-1

# Board Identification  
WISP_PLATFORM_ESP32C6=1
WISP_TARGET="ESP32C6"
WISP_BOARD_FAMILY="CUSTOM"

# Hardware Specifications
CPU_FREQ_MHZ=160
FLASH_SIZE_MB=4
SRAM_HP_KB=512

# Display Configuration
DISPLAY_WIDTH=240
DISPLAY_HEIGHT=320
DISPLAY_DRIVER="ST7789"

# Feature Flags
HAS_WIFI=1
HAS_BLUETOOTH=1
HAS_DISPLAY=1
HAS_RGB_LED=1

# GPIO Configuration
LED_PIN=2
SPI_SCK_PIN=6
SPI_MOSI_PIN=7
```

## Creating Custom Boards

### 1. Create Configuration File
```powershell
# Create new board config
New-Item "configs/boards/my-board.config"
```

### 2. Define Your Board
```ini
# Description: My Custom Development Board
# Add your board specifications here
ENV_TARGET=esp32s3
ENV_BOARD=esp32-s3-devkitc-1
WISP_PLATFORM_ESP32S3=1
# ... add your specific configuration
```

### 3. Build Your Board
```powershell
.\tools\build.ps1 -board "my-board"
```

## Code Integration

Your C++ code automatically gets the right configuration:

```cpp
#include "system/board_auto_config.h"

void setup() {
    // Print board information
    wisp_print_board_info();
    
    // Check features at compile time
    #if WISP_HAS_DISPLAY
        setup_display();
    #endif
    
    #if WISP_HAS_WIFI
        setup_wifi();
    #endif
    
    // Or check at runtime
    wisp_board_info_t info = wisp_get_board_info();
    if (info.features.bluetooth) {
        setup_bluetooth();
    }
}
```

## Available Build Actions

| Action | Description |
|--------|-------------|
| `build` | Compile the project (default) |
| `upload` | Compile and upload to device |
| `monitor` | Open serial monitor |
| `clean` | Clean build files |
| `buildupload` | Build then upload |

## Environment Variables

The build script automatically sets these environment variables:

- `PLATFORMIO_BUILD_FLAGS` - All board-specific compiler flags
- `ESP_BOARD_TYPE` - PlatformIO board identifier  
- `ESP_CPU_FREQ` - CPU frequency setting
- `ESP_FLASH_SIZE` - Flash memory size
- `ESP_MEMORY_TYPE` - Memory configuration type

## Benefits

### ✅ **Clean & Simple**
- No cluttered platformio.ini with dozens of environments
- Easy to add new boards without touching platformio.ini
- Clean separation of concerns

### ✅ **Scalable**
- Support unlimited board configurations
- Easy to share board configs between projects
- Version control friendly

### ✅ **User-Friendly**
- Simple command-line interface
- Descriptive error messages
- Auto-discovery of available boards

### ✅ **Developer-Friendly**
- Code automatically gets right configuration
- Compile-time feature checking
- Runtime board information access

### ✅ **Maintainable**
- Single source of truth for board configuration
- No duplicate configuration in multiple places
- Easy to update and modify

## Migration from Old System

If you have existing code that references old board configs:

### Old Way (❌)
```cpp
#ifdef PLATFORM_C6
    #include "boards/esp32-c6_config.h"
#endif
```

### New Way (✅)
```cpp
#include "system/board_auto_config.h"
// Configuration is automatic!
```

## Troubleshooting

### Board Not Found
```powershell
# List available boards
.\tools\build.ps1 -board "list"

# Check if config file exists
Test-Path "configs/boards/my-board.config"
```

### Build Errors
```powershell
# Use verbose mode to see full build flags
.\tools\build.ps1 -board "my-board" -verbose
```

### Configuration Issues
```powershell
# Check what the build script parsed
.\tools\build.ps1 -board "my-board" -verbose
# Look for "Parsed X configuration options"
```

## Advanced Usage

### Custom Build Flags
Add to your board config:
```ini
ENV_PLATFORMIO_BUILD_FLAGS="-DCUSTOM_FLAG=1 -DDEBUG_MODE=1"
```

### Custom Source Filtering
Add to your board config:
```ini
ENV_PLATFORMIO_SRC_FILTER="-<src/optional/>"
```

### Multiple Configurations
Create variants of the same board:
```
configs/boards/
├── my-board-debug.config      # Debug version
├── my-board-release.config    # Release version  
└── my-board-minimal.config    # Minimal features
```

## Summary

This dynamic build system makes it easy to:

1. **Add new boards** - Just create a `.config` file
2. **Build any board** - Single command with board name  
3. **Share configurations** - Config files are portable
4. **Scale indefinitely** - No platformio.ini bloat
5. **Maintain easily** - Single source of truth per board

The system is inspired by modern web development practices (like `cross-env` in Node.js) and brings the same simplicity to ESP32 development.
