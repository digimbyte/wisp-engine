# depth_layer_fg.spr - Foreground Layer Sprite

## Purpose
Foreground layer sprite for depth testing - appears closest to camera.

## Specifications
- **Dimensions**: 48x48 pixels
- **Format**: Wisp Sprite (.spr)
- **Color Depth**: 8-bit indexed color (256 colors)
- **Transparency**: Significant (50%+ transparent for overlay effect)
- **Animation**: Static
- **Intended Depth**: Layer 6-9 (closest to camera)

## Visual Design
Foreground overlay elements:
- UI overlays, particle effects, or atmospheric elements
- High contrast, bright colors
- Significant transparency to create overlay effect
- Should not obscure critical game elements entirely

## Usage in Test
- Tests highest depth layer rendering
- Validates transparency blending over other layers
- Ensures foreground elements render on top
- Tests partial transparency depth sorting

## Expected Content
A semi-transparent overlay element like leaves, smoke, rain effects, or UI elements that should appear in front of all other game elements. Should be mostly transparent with bright accent colors to clearly show when it's rendering in front of other sprites.
