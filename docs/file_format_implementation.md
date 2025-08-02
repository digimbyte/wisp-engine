# Wisp Engine File Format Implementation Guide

## Summary of Changes

We have successfully implemented a comprehensive file format system for the Wisp Engine with proper extensions and specifications.

## File Format Hierarchy

### Master Bundle Format
- **Extension:** `.wisp`
- **Purpose:** Complete application packages
- **Contains:** Multiple assets bundled together
- **Magic:** `WISP` (0x50534957)

### Asset-Specific Formats

#### 1. Palette/LUT Files
- **Extension:** `.wlut` (Wisp Lookup Table)
- **Purpose:** Color palettes and lookup tables  
- **Magic:** `WLUT` (0x54554C57)
- **Converter:** `wisp_palette_converter.py`
- **Verifier:** `wlut_verifier.py`

#### 2. Sprite Graphics
- **Extension:** `.art`
- **Purpose:** Sprite graphics with palette references
- **Magic:** `WART` (0x54524157)
- **Cross-references:** `.wlut` files for colors

#### 3. Audio Files
- **Extension:** `.sfx`
- **Purpose:** Sound effects and music
- **Magic:** `WSFX` (0x58465357)

#### 4. Source Code
- **Extension:** `.ash` (uncompiled)
- **Extension:** `.wash` (compiled)
- **Magic:** `WASH` / `WBIN`

## Tools and Workflow

### 1. Palette Conversion
```bash
# Convert PNG to WLUT format
py tools\wisp_palette_converter.py input.png

# Verify WLUT file
py tools\wlut_verifier.py output.wlut

# Batch conversion
.\convert_to_wlut.bat input.png [output_name]
```

### 2. Asset Bundling
```bash
# Bundle assets into WISP file
py tools\wisp_builder.py app_folder/
```

### 3. Supported Palette Formats

| Format | Dimensions | Memory | Use Case |
|--------|-----------|--------|----------|
| PAL_16 | 16 colors | 32 bytes | MINIMAL profile |
| PAL_64 | 64 colors | 128 bytes | BALANCED profile |
| PAL_256 | 256 colors | 512 bytes | Rich graphics |
| LUT_32x32 | 32×32 grid | 2KB | BALANCED profile |
| LUT_64x64 | 64×64 grid | 8KB | FULL profile |

## Memory Optimization Benefits

### Before (Old System)
- ❌ Single 32KB LUT for all graphics
- ❌ Inconsistent file extensions
- ❌ No format verification
- ❌ Memory waste for simple graphics

### After (New System)
- ✅ Multiple optimized formats (32 bytes to 8KB)
- ✅ Consistent `.wlut` extension for palettes
- ✅ Proper format verification tools
- ✅ Memory profiles matching game complexity
- ✅ 75-99% memory savings possible

## File Cross-References

### Sprite → Palette References
Sprites (`.art`) reference palettes (`.wlut`) by ID:
```cpp
// In sprite header
uint8_t paletteID;  // 0-15: system palettes, 16+: custom palettes
```

### Bundle Composition
A complete game `.wisp` bundle contains:
```
game.wisp
├── config.json         (game configuration)
├── main_palette.wlut   (primary colors)
├── sprites.art         (graphics data)
├── sounds.sfx          (audio data)
└── main.wash           (compiled game logic)
```

## Integration Examples

### Loading a Palette
```cpp
#include "my_palette_data.h"
#include "src/system/wisp_asset_types.h"

OptimizedPaletteSystem palette;
palette.loadPalette(0, my_palette_palette, MY_PALETTE_PALETTE_SIZE);
```

### Loading a LUT
```cpp
#include "my_lut_data.h"

HybridPaletteSystem lut;
lut.loadColorLUT(my_lut_lut);
```

### Checking Compatibility
```cpp
using namespace WispAssets;

if (isCompatibleWithProfile(ASSET_PALETTE, LUT_64x64, PROFILE_BALANCED)) {
    // Load high-quality LUT
} else {
    // Fall back to smaller palette
}
```

## File Format Verification

All generated files can be verified:
```bash
# Check WLUT format
py tools\wlut_verifier.py palette.wlut

# Check WISP bundle
py tools\wisp_verifier.py game.wisp  # (future tool)
```

## Development Workflow

1. **Create Palettes:** Convert PNG files to `.wlut` format
2. **Design Sprites:** Create `.art` files referencing `.wlut` palettes
3. **Add Audio:** Convert WAV/OGG to `.sfx` format
4. **Bundle Assets:** Combine everything into `.wisp` application
5. **Deploy:** Single `.wisp` file contains complete game

This system provides a clean, efficient, and memory-optimized asset pipeline for the Wisp Engine while maintaining clear separation of concerns and easy verification tools.
