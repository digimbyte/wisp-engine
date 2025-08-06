# sepia_lut.wlut - Sepia Tone Effect LUT

## Purpose
Sepia-toned color lookup table for testing color effect transformations.

## Specifications
- **Format**: Wisp LUT (.wlut)
- **Size**: 256 entries (8-bit palette)
- **Color Format**: RGB888 (24-bit color)
- **Type**: Effect palette (sepia tone)
- **Usage**: Color effect testing and vintage styling

## Color Transformation
Sepia tone mapping with warm brown tones:

### Sepia Formula Applied
For each original color, converted using sepia coefficients:
- **Red Output** = (R × 0.393) + (G × 0.769) + (B × 0.189)
- **Green Output** = (R × 0.349) + (G × 0.686) + (B × 0.168)
- **Blue Output** = (R × 0.272) + (G × 0.534) + (B × 0.131)

### Color Ranges
- **Shadows (0-63)**: Deep browns and dark sepia tones
- **Midtones (64-191)**: Classic sepia brown range
- **Highlights (192-255)**: Light cream and beige tones

### Transparency Handling
- Color 0 remains transparent
- All other colors mapped to sepia equivalents
- Maintains relative brightness relationships

## Visual Effect
Creates vintage photograph appearance:
- Warm, nostalgic brown color scheme
- Removes color saturation while maintaining contrast
- Suitable for retro/vintage game aesthetics
- Good for testing dramatic palette swapping

## Usage in Test
- Real-time palette swapping
- Color effect validation
- Performance with transformed palettes
- Visual effect accuracy testing
- Dramatic color change testing

## Expected Content
A complete sepia-toned palette that transforms any colorful sprite into a vintage, brown-toned appearance while maintaining visual clarity and contrast relationships.
