# WISP Cartridge System - GBA-Like ROM Loader

The WISP Cartridge System provides a Game Boy Advance-inspired ROM loading and execution environment for the WISP engine. This system allows you to create, distribute, and run applications as self-contained ROM files.

## Features

- **GBA-Like Cartridge Interface**: Insert, boot, reset, and eject ROM cartridges
- **Asset Streaming**: Load assets on-demand from ROM files to manage memory efficiently
- **Save Data Management**: Persistent save data per ROM with automatic management
- **Performance Monitoring**: Real-time FPS, memory usage, and runtime tracking
- **ROM Validation**: Comprehensive ROM integrity and compatibility checking
- **Memory Management**: Configurable memory budgets with automatic asset cleanup

## ROM Format Specification

WISP ROM files (`.wisp` extension) use a structured binary format:

```
[Header - 12 bytes]
├── Magic Number (4 bytes): 0x50534957 ("WISP")
├── Version (2 bytes): Format version
├── Asset Count (2 bytes): Number of assets
└── Config Size (4 bytes): Size of embedded YAML config

[Embedded Config - Variable size]
├── YAML configuration data
└── Contains app metadata and requirements

[Asset Table - 48 bytes per asset]
├── Name (32 bytes): Null-terminated asset name
├── Offset (4 bytes): Offset from start of asset data
├── Size (4 bytes): Asset size in bytes
├── Type (1 byte): Asset type ID
├── Flags (1 byte): Asset flags
└── Reserved (6 bytes): For future use

[Asset Data - Variable size]
└── Concatenated binary asset data
```

## Configuration Format

ROM configuration is embedded as YAML:

```yaml
name: "My WISP App"
version: "1.0.0"
author: "Developer Name"
description: "Application description"

performance:
  fps: 16          # Target FPS (8-16)
  ram: 131072      # Required RAM in bytes (32KB-384KB)

system:
  wifi: false      # Requires WiFi
  bluetooth: false # Requires Bluetooth
  eeprom: true    # Requires EEPROM for saves

assets:
  preload:         # Assets to load immediately
    - "splash.art"
    - "main_palette.wlut"
  streaming:       # Assets loaded on-demand
    - "level1.dat"
    - "music.sfx"
```

## Asset Types

The system supports various asset types:

| Type | ID | Extension | Description |
|------|----|-----------| ------------|
| Palette | 1 | .wlut | Color palettes and LUTs |
| Sprite | 2 | .art | Sprite graphics |
| Audio | 4 | .sfx | Sound effects |
| Config | 6 | .json | Configuration data |
| Source | 7 | .ash | Uncompiled C++ source |
| Binary | 8 | .wash | Compiled executable |

## Using the Cartridge System

### Basic Usage

```cpp
#include "src/engine/wisp_cartridge_system.h"

// Initialize cartridge system
g_CartridgeSystem = new WispCartridgeSystem();
g_CartridgeSystem->setMemoryBudget(262144); // 256KB

// Insert a ROM cartridge
if (g_CartridgeSystem->insertCartridge("/roms/my_app.wisp")) {
    // Boot the ROM
    if (g_CartridgeSystem->bootROM()) {
        Serial.println("ROM running successfully");
    }
}
```

### Asset Management

```cpp
// Load asset from ROM
if (g_CartridgeSystem->loadAsset("sprite.art")) {
    const uint8_t* spriteData = g_CartridgeSystem->getAssetData("sprite.art");
    // Use sprite data...
}

// Unload when no longer needed
g_CartridgeSystem->unloadAsset("sprite.art");
```

### Cartridge Operations

```cpp
// Reset ROM (restart app)
g_CartridgeSystem->resetROM();

// Power off (stop app, keep ROM loaded)
g_CartridgeSystem->powerOff();

// Eject cartridge (unload ROM)
g_CartridgeSystem->ejectCartridge();

// Get cartridge information
const CartridgeInfo& info = g_CartridgeSystem->getCartridgeInfo();
Serial.println(info.title);
```

## Building ROM Files

Use the included ROM builder tool:

```bash
# Create sample files
python tools/wisp_rom_builder.py --create-sample

# Build ROM from assets
python tools/wisp_rom_builder.py \
  --config sample_config.yaml \
  --assets sample_assets/ \
  --output my_app.wisp
```

### Manual ROM Creation

1. **Create Configuration**: Write a YAML config file with app metadata
2. **Prepare Assets**: Organize sprites, audio, and data files
3. **Build ROM**: Use the ROM builder or implement custom build process
4. **Test ROM**: Load and test in the WISP engine

## Memory Management

The cartridge system implements intelligent memory management:

- **Memory Budget**: Set maximum memory allocation for assets
- **Automatic Cleanup**: Unload least-recently-used assets when memory is full
- **Performance Monitoring**: Track memory pressure and usage statistics
- **Asset Streaming**: Load assets on-demand to minimize memory usage

```cpp
// Set memory budget (default: 256KB)
g_CartridgeSystem->setMemoryBudget(131072); // 128KB

// Check memory status
float pressure = g_CartridgeSystem->getMemoryPressure(); // 0.0-1.0
uint32_t used = g_CartridgeSystem->getMemoryUsed();
```

## Save Data System

Each ROM has automatic save data management:

```cpp
// Save data is automatically managed per ROM
// Based on ROM checksum for unique identification

// Check if ROM has save data
if (g_CartridgeSystem->hasSaveData()) {
    // Load save data automatically when ROM boots
}

// Save data is automatically saved when:
// - ROM is reset
// - ROM is powered off
// - Cartridge is ejected
```

## Performance Monitoring

Real-time performance statistics:

```cpp
// Print performance stats
g_CartridgeSystem->printPerformanceStats();

// Output example:
// === PERFORMANCE STATS ===
// FPS: 15.8
// Memory: 98304/262144 (37%)
// Runtime: 45230ms
// Assets Loaded: 3/8
// ==========================
```

## Error Handling

The system provides comprehensive error handling:

- **ROM Validation**: Magic number, format version, and integrity checks
- **Memory Validation**: Ensure asset offsets don't exceed ROM boundaries
- **Compatibility Checks**: Verify system requirements can be met
- **Asset Validation**: Check asset types and accessibility

## Integration with Bootloader

The cartridge system integrates seamlessly with the WISP bootloader:

1. **Initialization**: Cartridge system is initialized during boot
2. **ROM Discovery**: Automatic scanning for ROM files in `/roms/` directory
3. **Menu Integration**: ROM selection through boot menu
4. **Hot-Swapping**: Support for ejecting and inserting ROMs during runtime

## Development Workflow

### Creating a WISP Application

1. **Develop Application**: Create your app using the WISP App Interface
2. **Prepare Assets**: Create sprites, audio, and configuration files
3. **Write Configuration**: Define app metadata and requirements
4. **Build ROM**: Use the ROM builder to create a `.wisp` file
5. **Test**: Load ROM in WISP engine for testing
6. **Distribute**: Share the ROM file for others to run

### Testing and Debugging

```cpp
// Enable cartridge system debugging
#define WISP_CARTRIDGE_DEBUG 1

// Use the test application
#include "src/apps/test_cartridge_app.h"

// Run the cartridge example
// See examples/cartridge_system_example.cpp
```

## Advanced Features

### Custom Asset Types

Define custom asset types for specialized data:

```cpp
enum CustomAssetType : uint8_t {
    ASSET_CUSTOM_SCRIPT = 100,
    ASSET_CUSTOM_MAP = 101
};
```

### Memory Optimization

- **Asset Compression**: Future support for compressed assets
- **Streaming Buffers**: Configurable buffer sizes for streaming
- **Preload Strategies**: Intelligent asset preloading based on usage patterns

### ROM Security

- **Asset Encryption**: Optional encryption for protected content
- **Integrity Checks**: CRC validation for asset data
- **Version Compatibility**: Ensure ROM compatibility with engine version

## Example Applications

See the included examples:

- `examples/cartridge_system_example.cpp`: Basic cartridge system usage
- `src/apps/test_cartridge_app.h`: Sample application for testing
- `tools/wisp_rom_builder.py`: ROM building utility

## Future Enhancements

Planned features for future versions:

- **Hot-Reload**: Live asset reloading during development
- **Network ROMs**: Loading ROMs from network sources
- **ROM Signing**: Digital signatures for verified ROMs
- **Asset Compression**: Automatic compression/decompression
- **Multiplayer ROMs**: Multi-device ROM synchronization

## Technical Notes

### Performance Considerations

- **Asset Loading**: I/O operations are the primary bottleneck
- **Memory Fragmentation**: Use consistent asset sizes when possible
- **FPS Targeting**: Respect the 8-16 FPS range for optimal performance
- **Power Management**: Consider battery impact of frequent asset loading

### Platform Support

- **ESP32-S3**: Full feature support with external PSRAM
- **ESP32-C6**: Limited memory budget, optimize asset usage
- **Storage**: Supports SPIFFS and SD card storage

The WISP Cartridge System brings the nostalgic feel of cartridge-based gaming to modern embedded applications, providing a robust and flexible foundation for WISP application development and distribution.
