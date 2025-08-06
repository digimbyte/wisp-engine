# test_16x16.png - Source Sprite Asset

## Purpose
Source PNG file for the small 16x16 pixel sprite test asset.

## PNG Requirements
- **Filename**: `test_16x16.png`
- **Dimensions**: 16x16 pixels
- **Color Mode**: Indexed color (8-bit) with 256 colors maximum
- **Transparency**: PNG alpha channel or transparent color index
- **Format**: PNG with palette

## Visual Design
This should be a simple, recognizable icon or character:
- Centered design with clear edges
- High contrast colors for visibility
- Simple geometric shape or basic character sprite
- Examples: Small coin, gem, bullet, basic enemy, UI icon

## Color Palette Guidelines
- Use a limited palette (16-32 colors max for this small sprite)
- Ensure color index 0 is transparent (or will be mapped to transparent)
- Use distinct colors that will show well at small scale
- Avoid gradients or anti-aliasing that may not convert cleanly

## Build Process
This PNG file will be converted to `test_16x16.spr` using:
```
tools/png_to_spr.exe assets/src/test_16x16.png assets/test_16x16.spr
```

## Expected Output
- Wisp Sprite (.spr) format file
- Optimized for fast loading and rendering
- Maintains visual quality with indexed color mapping
- Proper transparency handling

## Creation Tips
1. Create in image editor with indexed color mode
2. Keep design simple and bold for small size
3. Test visibility at actual size (16x16 pixels)
4. Ensure clean edges without anti-aliasing
5. Use transparent background for irregular shapes
