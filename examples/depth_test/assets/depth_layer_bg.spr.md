# depth_layer_bg.spr - Background Layer Sprite

## Purpose
Background layer sprite for depth testing - appears furthest back.

## Specifications
- **Dimensions**: 128x128 pixels
- **Format**: Wisp Sprite (.spr)
- **Color Depth**: 8-bit indexed color (256 colors)
- **Transparency**: Minimal (mostly opaque background elements)
- **Animation**: Static
- **Intended Depth**: Layer 0-2 (furthest back)

## Visual Design
Background elements that should appear behind other sprites:
- Landscape elements (mountains, buildings, trees)
- Sky elements (clouds, distant objects)
- Muted, desaturated colors
- Large, simple shapes that won't interfere with foreground

## Usage in Test
- Validates depth sorting with background elements
- Tests rendering order with multiple overlapping sprites
- Ensures background elements stay behind interactive objects

## Expected Content
A large background element like a mountain range, city skyline, or forest backdrop. Should use muted colors (blues, grays, dark greens) and provide a clear backdrop that makes it obvious when depth sorting is working correctly.
