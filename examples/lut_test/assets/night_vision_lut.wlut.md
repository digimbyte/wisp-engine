# night_vision_lut.wlut - Night Vision Effect LUT

## Purpose
Green-tinted night vision color lookup table for testing monochromatic effects.

## Specifications
- **Format**: Wisp LUT (.wlut)
- **Size**: 256 entries (8-bit palette)
- **Color Format**: RGB888 (24-bit color)
- **Type**: Effect palette (night vision)
- **Usage**: Monochromatic effect testing and special vision modes

## Color Transformation
Night vision green tint with enhanced contrast:

### Green Monochrome Mapping
- **Luminance Calculation**: (R × 0.299) + (G × 0.587) + (B × 0.114)
- **Green Channel**: Enhanced luminance with green tint
- **Red/Blue Channels**: Minimal values for green-only effect
- **Contrast**: Boosted for better visibility

### Color Ranges
- **Dark (0-63)**: Very dark greens, almost black
- **Shadows (64-127)**: Medium dark greens
- **Midtones (128-191)**: Bright greens
- **Highlights (192-255)**: Very bright, almost white greens

### Special Effects
- Enhanced contrast for better night visibility
- Slightly boosted highlights for "electronic" look
- Reduced midtone compression for clarity

## Visual Effect
Creates night vision goggle appearance:
- Monochromatic green color scheme
- High contrast for visibility in dark conditions
- Electronic/military equipment aesthetic
- Retains detail while changing color entirely

## Usage in Test
- Monochromatic palette effects
- Extreme color transformation
- Contrast enhancement validation
- Special effect palette swapping
- Performance with limited color ranges

## Expected Content
A complete green-tinted monochromatic palette that transforms any sprite into a night vision appearance, maintaining detail and contrast while providing the characteristic green glow effect of night vision equipment.
