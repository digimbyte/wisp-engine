# test_32x32.png - Source Sprite Asset

## Purpose
Source PNG file for the medium 32x32 pixel sprite test asset.

## PNG Requirements
- **Filename**: `test_32x32.png`
- **Dimensions**: 32x32 pixels
- **Color Mode**: Indexed color (8-bit) with 256 colors maximum
- **Transparency**: PNG alpha channel or transparent color index
- **Format**: PNG with palette

## Visual Design
This should be a detailed character or object sprite:
- Well-defined character or recognizable object
- Multiple colors with shading/highlights
- Clear silhouette and readable details
- Examples: Character face, detailed enemy, item, vehicle

## Color Palette Guidelines
- Use a moderate palette (32-64 colors for good detail)
- Include shading and highlight colors
- Maintain good contrast between elements
- Group similar colors together in palette

## Build Process
This PNG file will be converted to `test_32x32.spr` using:
```
tools/png_to_spr.exe assets/src/test_32x32.png assets/test_32x32.spr
```

## Creation Tips
1. Design with clear, readable details at 32x32 size
2. Use solid colors rather than gradients for clean conversion
3. Include 2-3 shades per color for basic depth
4. Test readability at actual pixel size
5. Consider the sprite's use in gameplay contexts
