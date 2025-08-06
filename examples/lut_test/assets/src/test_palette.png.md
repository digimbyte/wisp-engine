# test_palette.png - Source Palette Image

## Purpose
Source PNG containing the 256-color palette for standard game colors.

## PNG Requirements
- **Filename**: `test_palette.png`
- **Dimensions**: 256x1 pixels (256 colors in a single row)
- **Color Mode**: Indexed color (8-bit) with exactly 256 colors
- **Format**: PNG with palette
- **Layout**: Linear arrangement of all 256 palette colors

## Palette Organization
Organize the 256 colors as follows:

### Colors 0-15: System Colors
- Color 0: Transparent (R:0, G:0, B:0)
- Colors 1-7: Basic colors (Red, Green, Blue, Yellow, Cyan, Magenta, White)
- Colors 8-15: Dark variants of basic colors

### Colors 16-31: Grayscale Ramp
- Smooth gradient from black (16) to white (31)
- Even steps for smooth color transitions
- Critical for depth shading effects

### Colors 32-95: Game World Colors
- Browns and earth tones (32-47)
- Greens for vegetation (48-63)
- Blues for water and sky (64-79)
- Reds and oranges for fire/danger (80-95)

### Colors 96-159: Character Colors
- Skin tones (96-111)
- Hair colors (112-127)
- Clothing colors (128-143)
- Equipment colors (144-159)

### Colors 160-255: Special Effects
- Bright highlights and magical effects
- Transitional colors for animations
- UI accent colors
- Debug/testing colors

## Build Process
This PNG palette will be converted to `test_palette.wlut` using:
```
tools/png_to_lut.exe assets/src/test_palette.png assets/test_palette.wlut
```

## Creation Tips
1. Create as a 256x1 pixel image in indexed color mode
2. Arrange colors logically for easy palette management
3. Include smooth gradients within color families
4. Test colors against various backgrounds
5. Ensure good contrast between adjacent functional colors
