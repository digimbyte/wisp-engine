# test_palette.wlut - Standard Color Palette LUT

## Purpose
Standard color lookup table for testing basic palette operations and color mapping.

## Specifications
- **Format**: Wisp LUT (.wlut)
- **Size**: 256 entries (8-bit palette)
- **Color Format**: RGB888 (24-bit color)
- **Type**: Standard game palette
- **Usage**: Basic color mapping and palette swapping

## Palette Structure
Organized color ranges for comprehensive testing:

### Colors 0-15: System Colors
- Color 0: Transparent (R:0, G:0, B:0)
- Colors 1-7: Basic colors (Red, Green, Blue, Yellow, Cyan, Magenta, White)
- Colors 8-15: Dark variants of basic colors

### Colors 16-31: Grayscale Ramp
- Smooth gradient from black to white
- Used for testing smooth color transitions
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

## Usage in Test
- Basic palette application to sprites
- Color cycling animations
- Palette swapping effects
- Color accuracy validation
- Performance with full palette usage

## Expected Content
A well-balanced color palette that provides good coverage of all color ranges needed for a typical 2D game, with smooth gradients and logical color groupings for easy testing of palette operations.
