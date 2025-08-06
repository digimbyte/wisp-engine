# anim_scale.spr - Scale Animation Test Sprite

## Purpose
Sprite designed for testing scale/size animation and transformation effects.

## Specifications
- **Dimensions**: 48x48 pixels (base size)
- **Format**: Wisp Sprite (.spr)
- **Color Depth**: 8-bit indexed color (256 colors)
- **Transparency**: Yes, color index 0 = transparent
- **Animation**: Static sprite (scaling handled by engine)
- **Scale Range**: 0.25x to 3.0x for testing

## Visual Design
Sprite optimized for scaling effects:
- Symmetrical design that scales well
- Clear details visible at small scales
- Clean lines that remain crisp when enlarged
- Central focus point for scale origin
- Examples: Circle with rings, star, geometric pattern

## Scale Testing
Used for validating various scaling animations:
- Uniform scaling (proportional width/height)
- Non-uniform scaling (different X/Y ratios)
- Scale easing (bounce, elastic effects)
- Scale interpolation smoothness
- Pixel-perfect scaling at integer multipliers

## Usage in Test
- Scale transformation accuracy
- Scaling algorithm validation
- Performance with various scale factors
- Visual quality at different sizes
- Scale animation smoothness

## Expected Content
A symmetrical, detailed sprite that looks good at both very small (12x12) and large (144x144) sizes. Should have concentric elements or clear geometric patterns that make scaling effects obvious and easy to evaluate for correctness.
