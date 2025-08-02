# Wisp Palette Converter Usage Guide

## Overview
The Wisp Palette Converter processes PNG images into optimized palette data formats for the Wisp Engine on ESP32-C6.

## Installation
```bash
# Install required dependencies
pip install Pillow numpy

# Make the script executable (Linux/Mac)
chmod +x tools/wisp_palette_converter.py
```

## Supported Input Formats

### Automatic Detection
The tool automatically detects the best format based on image dimensions:

| Image Size | Auto-Detected Format | Memory Usage | Use Case |
|------------|---------------------|--------------|----------|
| 64×64 pixels | LUT_64x64 | 8KB | Color gradients, lighting effects |
| 32×32 pixels | LUT_32x32 | 2KB | Simple color mixing |
| 16×1 pixels | PALETTE_16 | 32 bytes | Game Boy style (minimal) |
| 64×1 pixels | PALETTE_64 | 128 bytes | Balanced color palette |
| 256×1 pixels | PALETTE_256 | 512 bytes | Full color palette |
| Other sizes | Based on color count | Varies | Auto-optimized |

## Usage Examples

### Convert Your lut_palette.png
```bash
# Basic conversion (auto-detects format)
python tools/wisp_palette_converter.py lut_palette.png

# Preview colors before conversion
python tools/wisp_palette_converter.py lut_palette.png --preview

# Force specific format
python tools/wisp_palette_converter.py lut_palette.png -f lut64

# Custom output name
python tools/wisp_palette_converter.py lut_palette.png -o game_palette
```

### Format-Specific Conversions
```bash
# Force 64×64 LUT (even if image is different size)
python tools/wisp_palette_converter.py myimage.png -f lut64

# Force 16-color palette
python tools/wisp_palette_converter.py myimage.png -f pal16

# Force 256-color palette  
python tools/wisp_palette_converter.py myimage.png -f pal256
```

### Output Options
```bash
# Generate only C header file
python tools/wisp_palette_converter.py lut_palette.png --header-only

# Generate only binary file
python tools/wisp_palette_converter.py lut_palette.png --binary-only

# Generate both (default)
python tools/wisp_palette_converter.py lut_palette.png
```

## Output Files

### C Header File (.h)
Contains RGB565 color data ready for inclusion in your project:
```cpp
// lut_palette_data.h
const uint16_t lut_palette_lut[4096] = {
    0xF800, 0xF820, 0xF840, ...
};
```

### Binary File (.wisp)
Compact binary format for loading at runtime:
- 4 bytes: Magic "WISP" 
- 4 bytes: Format identifier
- 4 bytes: Data size
- N bytes: RGB565 color data (little endian)

## Integration Examples

### Using 64×64 LUT
```cpp
#include "lut_palette_data.h"

HybridPaletteSystem palette;
palette.loadColorLUT(lut_palette_lut);

// Use LUT for color mixing
uint16_t color = palette.getLutColor(32, 45);  // X=32, Y=45
uint16_t blended = palette.getBlendedColor(0, 5, 32, 45);  // Palette + LUT
```

### Using Standard Palette
```cpp
#include "game_palette_data.h"

OptimizedPaletteSystem palette;
palette.loadPalette(0, game_palette_palette, GAME_PALETTE_PALETTE_SIZE);

// Use palette colors
uint16_t color = palette.getColor(0, 5);  // Palette 0, color 5
```

## Creating Palette Images

### 64×64 LUT Image
- Create a 64×64 pixel image
- X-axis represents one parameter (e.g., hue, brightness)
- Y-axis represents another parameter (e.g., saturation, contrast)
- Each pixel = unique color combination

### 16-Color Palette Image  
- Create a 16×1 pixel image (or 4×4, 2×8, etc.)
- Each pixel = one palette color
- First pixel should be transparent (black: #000000)
- Remaining 15 pixels = your game colors

### 256-Color Palette Image
- Create a 256×1 pixel image (or 16×16)
- Each pixel = one palette color
- Organize colors logically (reds, greens, blues, etc.)

## Batch Processing

### Windows Batch File
```batch
@echo off
cd /d "%~dp0"

echo Converting all PNG files to Wisp palettes...
for %%f in (assets\palettes\*.png) do (
    echo Converting %%f...
    python tools\wisp_palette_converter.py "%%f"
)

echo Done!
pause
```

### Linux/Mac Shell Script
```bash
#!/bin/bash
echo "Converting all PNG files to Wisp palettes..."

for file in assets/palettes/*.png; do
    echo "Converting $file..."
    python3 tools/wisp_palette_converter.py "$file"
done

echo "Done!"
```

## Memory Profile Recommendations

### MINIMAL Profile (Game Boy Style)
- Use 16-color palettes (32 bytes each)
- Convert: `python tools/wisp_palette_converter.py image.png -f pal16`
- Total memory: ~128 bytes for 4 palettes

### BALANCED Profile (GBA Style)  
- Use 64-color palettes OR 32×32 LUT
- Convert: `python tools/wisp_palette_converter.py image.png -f pal64`
- Total memory: ~512 bytes or 2KB

### FULL Profile (Modern Indie)
- Use 256-color palettes OR 64×64 LUT
- Convert: `python tools/wisp_palette_converter.py image.png -f pal256`
- Total memory: ~2KB or 8KB

## Troubleshooting

### "Too many colors in image"
- Reduce colors in your image editor
- Use Image → Mode → Indexed Color in Photoshop/GIMP
- Or force a smaller palette: `-f pal16`

### "Error analyzing image"
- Check PNG file is valid
- Try converting to RGB mode first
- Ensure image dimensions are reasonable

### "Import numpy could not be resolved"
- Install numpy: `pip install numpy`
- Or use alternative installation: `conda install numpy`

## Tips for Best Results

1. **Design for your memory profile** - smaller palettes = more game memory
2. **Organize colors logically** - group similar colors together
3. **Use transparency wisely** - first color (index 0) is always transparent
4. **Test in-game** - preview colors on actual device if possible
5. **Version control palettes** - keep PNG sources in your repo

## Advanced Usage

### Custom Format Detection
The tool can analyze your image and suggest the optimal format:
```bash
python tools/wisp_palette_converter.py mystery_palette.png --preview
# Shows detected format, color count, and memory usage
```

### Batch Conversion with Different Formats
```bash
# Convert UI palettes to 16-color
for ui in ui_*.png; do
    python tools/wisp_palette_converter.py "$ui" -f pal16
done

# Convert gradient LUTs to 64×64
for lut in gradient_*.png; do  
    python tools/wisp_palette_converter.py "$lut" -f lut64
done
```
