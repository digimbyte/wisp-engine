# Wisp Engine Sprite Format Documentation

## Overview
Wisp Engine uses a sophisticated sprite format optimized for memory efficiency and hardware-accelerated rendering on ESP32. The format consists of two main components:

### 1. Color Data (2D Array)
- Each pixel stores a **color index** (0-255)
- Color indices reference a **128x128 Color LUT** (lookup table)
- The LUT is loaded from `assets/lut3d_flat.png` as a 16384-color palette
- LUT coordinates: `lutX = colorIndex % 128`, `lutY = colorIndex / 128`
- **LUT Index 0,0** (color index 0): Reserved for invisible pixels (no transparency, just skipped)

### 2. Depth Channel (Run-Length Encoded)
- Separate depth information for each pixel (0-12 depth levels)
- **Run-length encoding**: `(depth, distance)` pairs
- Format: "Set depth to X for the next Y pixels"
- Dramatically reduces memory usage for sprites with consistent depth regions

### 3. Sprite Sheets/Frame Grids
- **Frame-based sprites**: Support for multi-frame sprite sheets
- **Grid layout**: Defined by rows × columns structure
- **Individual frame access**: Draw specific frames from sheet
- **Application-defined usage**: Apps determine what frames represent (animation, variations, states, etc.)

## Binary Format Structure

```cpp
struct SpriteHeader {
    uint16_t width;           // Full sprite sheet width in pixels
    uint16_t height;          // Full sprite sheet height in pixels
    uint16_t colorDataSize;   // Size of color index array (width * height)
    uint16_t depthDataSize;   // Size of depth run-length data
    uint8_t paletteId;        // Which palette slot to use (0-3)
    uint8_t flags;            // Sprite flags (animation, etc.)
    uint8_t frameRows;        // Number of frame rows in sprite sheet
    uint8_t frameCols;        // Number of frame columns in sprite sheet
    uint16_t frameWidth;      // Width of individual frame
    uint16_t frameHeight;     // Height of individual frame
};

struct DepthRun {
    uint8_t depth;      // Depth value (0-12, where 0=front, 12=back)
    uint16_t distance;  // How many pixels this depth applies to
};
```

## Memory Layout
```
[SpriteHeader]
[Color Data: width * height bytes]
[Depth Runs: depthDataSize bytes]
```

## Color LUT System

### 128x128 Color LUT
- **Total Colors**: 16,384 unique RGB565 colors
- **Storage**: 32KB lookup table loaded from PNG
- **Access Pattern**: `color = colorLUT[lutY * 128 + lutX]`
- **Benefits**: 
  - Massive color variety with 1-byte indices
  - Fast hardware-accelerated lookups
  - Palette animation support

### Example Color Index Mapping
```
Color Index 0:    LUT[0, 0]     - INVISIBLE PIXELS (always skipped)
Color Index 1:    LUT[0, 1]     - First visible color
Color Index 128:  LUT[1, 0]     - Second row, first column
Color Index 255:  LUT[1, 127]   - Maximum color index
```

## Sprite Sheets & Frame Grids

### Frame Layout
```
Sprite Sheet (128x64, 4x2 frames):
┌─────┬─────┬─────┬─────┐
│ 0,0 │ 0,1 │ 0,2 │ 0,3 │
├─────┼─────┼─────┼─────┤
│ 1,0 │ 1,1 │ 1,2 │ 1,3 │
└─────┴─────┴─────┴─────┘
Each frame: 32x32 pixels
```

### Frame Access
- **Frame coordinates**: (row, col) starting from (0,0)
- **Linear frame index**: `frameIndex = row * frameCols + col`
- **Memory layout**: Frames stored sequentially in sprite data
- **Rendering**: Extract frame region from full sprite sheet
- **Usage definition**: Application determines what each frame represents

## Depth System

### Depth Levels
- **Range**: 0-12 (13 total depth layers)
- **0**: Foreground (closest to camera)
- **12**: Background (farthest from camera)
- **Usage**: Automatic depth sorting, parallax effects, collision layers

### Run-Length Encoding Example
```
Raw depth data: [2,2,2,2,2,5,5,5,1,1,1,1]
RLE encoded:    [(2,5), (5,3), (1,4)]
Memory saved:   12 bytes → 6 bytes (50% reduction)
```

## Sprite Flags
```cpp
#define SPRITE_TILEABLE       0x01  // Can be tiled seamlessly
#define SPRITE_COLLISION      0x02  // Has collision data
#define SPRITE_MIRRORED       0x04  // Can be horizontally mirrored
#define SPRITE_RESERVED       0x08  // Reserved for future use
```

## Performance Characteristics

### Memory Usage
- **Small sprites** (16x16): ~300 bytes (including depth)
- **Medium sprites** (64x64): ~4KB 
- **Large sprites** (128x128): ~16KB
- **LUT overhead**: 32KB (shared across all sprites)

### Rendering Performance
- **Color lookup**: 1 memory access per pixel
- **Depth testing**: Hardware-accelerated per-pixel comparison
- **Invisible pixels**: Zero-cost (skip pixel if index 0)
- **Palette swapping**: Real-time color modification without re-encoding
- **Frame access**: Direct frame extraction from sprite sheet

## Integration with WPACK

Sprites are stored in WPACK files with asset type `0x02`:

```python
# In wisp_builder.py
ASSET_TYPE = {
    '.sprite': 0x02,   # Compiled sprite data
    '.png': 0x02,      # Convert PNG to sprite format
    # ...
}
```

## Example Usage in Lua

```lua
-- Clear screen and set up rendering
graphics.clear(graphics.colors.BLACK)
graphics.setPalette(0)  -- Use palette slot 0
graphics.setDepthTest(true)

-- Draw static sprite (defaults to frame 0,0)
graphics.drawSprite(0, 10, 20)

-- Draw specific frame from sprite sheet
graphics.drawSpriteFrame(1, 50, 30, 0, 2)  -- Row 0, Column 2

-- Application-defined frame usage examples:
-- Character facing directions
local facing_down = 0
local facing_right = 1
graphics.drawSpriteFrame(player_sprite, x, y, facing_down, 0)

-- Animation frames (app manages timing)
local walk_frames = {0, 1, 2, 3}
local current_frame = walk_frames[frameIndex % 4 + 1]
graphics.drawSpriteFrame(player_sprite, x, y, 0, current_frame)

-- Item variations
local coin_types = {gold = 0, silver = 1, bronze = 2}
graphics.drawSpriteFrame(coin_sprite, x, y, coin_types.gold, 0)

-- Get sprite information
local frame_count = graphics.getSpriteFrameCount(0)
local frame_w, frame_h = graphics.getSpriteFrameSize(0)
local sheet_rows, sheet_cols = graphics.getSpriteSheetLayout(0)

-- Present frame to display
graphics.present()
```

## Artist Workflow

1. **Create sprite sheet in any image editor** (PNG format)
2. **Organize frames in grid layout** (app-specific arrangement)
3. **Generate depth map** (separate grayscale image, 0-12 values)
4. **Run through sprite compiler** (converts PNG + depth → binary format)
5. **Pack into WPACK** using wisp_builder.py
6. **Load in app** via graphics.loadSprite() or automatic WPACK loading

## Technical Benefits

1. **Memory Efficient**: RLE depth + indexed color dramatically reduces RAM usage
2. **Hardware Accelerated**: Depth testing done by graphics engine, not CPU
3. **Artist Friendly**: Standard PNG workflow with powerful depth capabilities  
4. **Flexible**: 16K color palette supports rich artistic expression
5. **Performance**: Single-pass rendering with automatic depth sorting
6. **Scalable**: System works equally well for pixel art and detailed artwork

This format allows Wisp Engine to achieve Game Boy Advance-level sprite capabilities while running efficiently on ESP32 hardware. The sprite system provides the foundation - applications define how frames are used for animation, state representation, or any other purpose.
