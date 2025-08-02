# WISP ROM Format Specification

## Overview
WISP files are now complete "game ROM" bundles that embed everything needed to run an application, including configuration. This follows the data stack: **headers → config → database/map panels → logic → assets**.

## ROM Structure

```
[WISP ROM Layout]
+------------------+
| Header (12 bytes)|  Magic, version, entry count, config size
+------------------+
| Embedded Config  |  YAML configuration (variable size)
+------------------+
| Entry Table      |  Asset directory (48 bytes per entry)
+------------------+
| Asset Data       |  Raw asset payloads
+------------------+
```

## Header Format (12 bytes)

| Offset | Size | Type | Description |
|--------|------|------|-------------|
| 0x00   | 4    | char[4] | Magic: "WISP" |
| 0x04   | 2    | uint16 | Version (0x0100 = v1.0) |
| 0x06   | 2    | uint16 | Entry count |
| 0x08   | 4    | uint32 | Embedded config size |

## Embedded Config Section

- **Location**: Immediately after header (offset 12)
- **Format**: YAML text data
- **Size**: Variable, specified in header
- **Purpose**: Application configuration, metadata, requirements

### Config Contents:
```yaml
# Application metadata
name: "My Game"
version: "1.0.0" 
author: "Developer"

# Performance requirements  
performance:
  fps: 16          # App frame rate (8, 10, 12, 14, 16)
  ram: 131072      # RAM needed (32KB-384KB)
  storage: 0       # Persistent storage needed

# System features
system:
  wifi: false      # WiFi connectivity
  bluetooth: false # Bluetooth features
  eeprom: false    # Persistent settings

# Audio configuration
audio:
  outputs: ["piezo"]   # Hardware outputs
  sampleRate: 22050    # Audio sample rate
  channels: 4          # Audio channels
```

## Entry Table Format (48 bytes per entry)

| Offset | Size | Type | Description |
|--------|------|------|-------------|
| 0x00   | 32   | char[32] | Asset name (null-terminated) |
| 0x20   | 4    | uint32 | Data offset (relative to data section) |
| 0x24   | 4    | uint32 | Asset size in bytes |
| 0x28   | 1    | uint8 | Asset type |
| 0x29   | 1    | uint8 | Flags |
| 0x2A   | 6    | uint8[6] | Reserved |

### Asset Types:
```cpp
0x01  .wlut     // Wisp Lookup Tables (palettes)
0x02  .art      // Sprite graphics data  
0x03  .tilemap  // Tile-based maps
0x04  .sfx      // Audio files
0x05  .font     // Font data
0x07  .ash      // Uncompiled C++ source
0x08  .wash     // Compiled bytecode/binary
0x09  .level    // Game level data
0x0A  .depth    // Depth map data
```

## Data Section

Raw asset payloads concatenated in the order listed in the entry table.

## ROM Benefits

### ✅ Complete Self-Contained Package
- **Single File**: Everything needed to run the app
- **No External Dependencies**: Config embedded, not separate
- **ROM-like**: Similar to classic game cartridges

### ✅ Efficient Structure  
- **Embedded Config**: No need to search entry table for config
- **Direct Access**: Config at known offset (12 bytes from start)
- **Backward Compatible**: Can still handle legacy separate config files

### ✅ Development Workflow
```bash
# Build complete ROM
py tools/wisp_builder.py my_game my_game.wisp

# Verify ROM structure  
py tools/wisp_verifier.py my_game.wisp

# Deploy single file
cp my_game.wisp /sd_card/apps/
```

## Example ROM Creation

### Source Structure:
```
my_game/
├── config.yaml      # Embedded into ROM header
├── main.wash        # Game logic
├── sprites.art      # Graphics assets
├── audio.sfx        # Sound effects  
└── levels.level     # Game data
```

### Generated ROM:
```
my_game.wisp (Complete ROM)
├── [Header: 12 bytes]
├── [Config: ~1KB embedded YAML]
├── [Entry Table: 4 entries × 48 bytes = 192 bytes]  
└── [Assets: main.wash + sprites.art + audio.sfx + levels.level]
```

## App Loader Changes

The ESP32 app loader now:
1. **Checks for embedded config first** (modern ROM format)
2. **Falls back to entry table search** (legacy compatibility)
3. **Loads config from known offset** (more efficient)

```cpp
// Modern ROM format - config at offset 12
if (configSize > 0) {
    entry.configOffset = 12;
    entry.configSize = configSize;
}
```

## Storage Efficiency

**ROM Overhead**: 
- Header: 12 bytes
- Config: ~1KB (YAML with comments)
- Entry table: 48 bytes per asset

**Typical ROM**:
- Small game: ~90% efficiency (assets dominate)
- Large game: ~95%+ efficiency (overhead minimal)

## ROM Data Stack Compliance

✅ **Headers**: WISP magic + metadata
✅ **Config**: Embedded YAML configuration  
✅ **Database/Map panels**: Entry table asset directory
✅ **Logic**: .wash compiled bytecode
✅ **Assets**: Graphics, audio, data files

The WISP ROM format now perfectly matches your requested data stack architecture! 🎮
