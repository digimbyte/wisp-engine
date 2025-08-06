# test_64x64.png - Source Sprite Asset

## Purpose
Source PNG file for the large 64x64 pixel sprite test asset.

## PNG Requirements
- **Filename**: `test_64x64.png`
- **Dimensions**: 64x64 pixels
- **Color Mode**: Indexed color (8-bit) with 256 colors maximum
- **Transparency**: PNG alpha channel or transparent color index
- **Format**: PNG with palette

## Visual Design
This should be a highly detailed sprite:
- Complex character or object with fine details
- Rich color palette utilizing many available colors
- Intricate design with shading, highlights, and textures
- Examples: Detailed character portrait, boss enemy, complex machinery

## Color Palette Guidelines
- Use a rich palette (64-128 colors for high detail)
- Include multiple shades for smooth color transitions
- Use highlight and shadow colors effectively
- Organize palette logically for similar tones

## Build Process
This PNG file will be converted to `test_64x64.spr` using:
```
tools/png_to_spr.exe assets/src/test_64x64.png assets/test_64x64.spr
```

## Creation Tips
1. Take advantage of the larger canvas for detail work
2. Use multiple color shades for smooth shading
3. Include fine details that showcase sprite system capabilities
4. Consider this as a "showcase" sprite for the engine
5. Test performance with this larger, detailed sprite
