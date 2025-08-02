# Wisp Engine File Formats Specification

## Overview
The Wisp Engine uses a collection of specialized file formats optimized for ESP32 memory constraints and performance.

## File Format Summary

| Extension | Type | Description | Magic | Content |
|-----------|------|-------------|-------|---------|
| `.wisp` | **Master Bundle** | Complete application package | `WISP` | Multiple assets bundled together |
| `.wlut` | **Palette/LUT** | Color lookup tables and palettes | `WLUT` | RGB565 color data |
| `.art` | **Sprites** | Compiled sprite graphics | `WART` | Sprite data with LUT references |
| `.sfx` | **Audio** | Sound effects and music | `WSFX` | Compressed audio data |
| `.ash` | **Source Code** | Uncompiled C++ source | `WASH` | Plain text C++ source |
| `.wash` | **Compiled Code** | Compiled bytecode/binary | `WBIN` | Compiled executable code |

## Detailed Format Specifications

### 1. WISP Files (Master Bundle)
**Extension:** `.wisp`  
**Magic:** `WISP` (0x50534957)  
**Purpose:** Application packaging and distribution

```
Header (16 bytes):
- Magic: 4 bytes ('WISP')
- Version: 2 bytes (major.minor)
- Entry Count: 2 bytes
- Reserved: 8 bytes

Entry Table (32 bytes per entry):
- Name: 32 bytes (null-terminated)
- Offset: 4 bytes (from start of file)
- Size: 4 bytes
- Type: 1 byte (asset type ID)
- Flags: 1 byte
- Reserved: 6 bytes

Data Section:
- Raw asset data concatenated
```

### 2. WLUT Files (Palette/LUT)
**Extension:** `.wlut`  
**Magic:** `WLUT` (0x54554C57)  
**Purpose:** Color palettes and lookup tables

```
Header (16 bytes):
- Magic: 4 bytes ('WLUT')
- Format: 4 bytes (LUT_64x64, PAL_16, etc.)
- Width: 2 bytes (for LUTs)
- Height: 2 bytes (for LUTs)
- Color Count: 2 bytes
- Reserved: 2 bytes

Data Section:
- RGB565 color values (2 bytes each)
- Little endian format
```

**Supported Formats:**
- `LUT_64x64` (0x4C555436) - 64×64 color lookup table (8KB)
- `LUT_32x32` (0x4C555433) - 32×32 color lookup table (2KB)
- `PAL_16` (0x50414C31) - 16 color palette (32 bytes)
- `PAL_64` (0x50414C36) - 64 color palette (128 bytes)
- `PAL_256` (0x50414C38) - 256 color palette (512 bytes)

### 3. ART Files (Sprites)
**Extension:** `.art`  
**Magic:** `WART` (0x54524157)  
**Purpose:** Sprite graphics with palette references

```
Header (24 bytes):
- Magic: 4 bytes ('WART')
- Width: 2 bytes
- Height: 2 bytes
- Depth: 1 byte (bits per pixel)
- Palette ID: 1 byte (references .wlut file)
- Frame Count: 2 bytes
- Animation Speed: 2 bytes (ms per frame)
- Flags: 1 byte
- Reserved: 7 bytes

Frame Data:
- Pixel data (indexed or direct)
- Compression depends on depth
```

### 4. SFX Files (Audio)
**Extension:** `.sfx`  
**Magic:** `WSFX` (0x58465357)  
**Purpose:** Audio data

```
Header (20 bytes):
- Magic: 4 bytes ('WSFX')
- Sample Rate: 4 bytes
- Channels: 1 byte
- Bits Per Sample: 1 byte
- Compression: 1 byte
- Loop Start: 4 bytes
- Loop End: 4 bytes
- Reserved: 1 byte

Audio Data:
- Raw or compressed audio samples
```

### 5. ASH Files (Source Code)
**Extension:** `.ash`  
**Magic:** `WASH` (0x48534157)  
**Purpose:** Uncompiled C++ source files

```
Header (16 bytes):
- Magic: 4 bytes ('WASH')
- Source Size: 4 bytes
- Language: 1 byte (0x01 = C++)
- Version: 1 byte
- Flags: 1 byte
- Reserved: 5 bytes

Source Data:
- UTF-8 encoded C++ source code
- Optional compression
```

### 6. WASH Files (Compiled Code)
**Extension:** `.wash`  
**Magic:** `WBIN` (0x4E494257)  
**Purpose:** Compiled executable code

```
Header (24 bytes):
- Magic: 4 bytes ('WBIN')
- Code Size: 4 bytes
- Entry Point: 4 bytes
- Architecture: 1 byte (0x01 = ESP32)
- Flags: 1 byte
- Reserved: 10 bytes

Code Data:
- Compiled machine code or bytecode
- Ready for execution
```

## Asset Type IDs

```cpp
enum WispAssetType : uint8_t {
    ASSET_UNKNOWN = 0x00,
    ASSET_PALETTE = 0x01,    // .wlut files
    ASSET_SPRITE = 0x02,     // .art files
    ASSET_TILEMAP = 0x03,    // Tile-based maps
    ASSET_AUDIO = 0x04,      // .sfx files
    ASSET_FONT = 0x05,       // Font data
    ASSET_CONFIG = 0x06,     // JSON configuration
    ASSET_SOURCE = 0x07,     // .ash files
    ASSET_BINARY = 0x08,     // .wash files
    ASSET_LEVEL = 0x09,      // Game level data
    ASSET_SCRIPT = 0x0A      // Runtime scripts
};
```

## Memory Usage Guidelines

| Format | Typical Size | Memory Profile |
|--------|--------------|----------------|
| PAL_16 | 32 bytes | MINIMAL |
| PAL_64 | 128 bytes | BALANCED |
| PAL_256 | 512 bytes | FULL |
| LUT_32x32 | 2KB | BALANCED |
| LUT_64x64 | 8KB | FULL |

## Usage Examples

### Loading a Palette
```cpp
#include "my_palette_data.h"

OptimizedPaletteSystem palette;
palette.loadPalette(0, my_palette_palette, MY_PALETTE_PALETTE_SIZE);
```

### Loading a LUT
```cpp
#include "my_lut_data.h"

HybridPaletteSystem lut;
lut.loadColorLUT(my_lut_lut);
```

### Cross-References
Sprites (`.art`) reference palettes (`.wlut`) by ID:
- Palette ID 0-15: Built-in system palettes
- Palette ID 16+: Custom application palettes

This enables efficient memory usage and consistent color schemes across sprites.
