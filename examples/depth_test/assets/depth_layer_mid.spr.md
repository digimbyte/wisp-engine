# depth_layer_mid.spr - Midground Layer Sprite

## Purpose
Midground layer sprite for depth testing - appears in middle depth.

## Specifications
- **Dimensions**: 64x64 pixels
- **Format**: Wisp Sprite (.spr)
- **Color Depth**: 8-bit indexed color (256 colors)
- **Transparency**: Partial (some transparent areas)
- **Animation**: Static
- **Intended Depth**: Layer 3-5 (middle ground)

## Visual Design
Midground game elements:
- Interactive objects (trees, rocks, buildings)
- Environmental decorations
- Medium saturation colors
- Clear edges and recognizable shapes

## Usage in Test
- Tests depth sorting between background and foreground
- Validates proper layering of game world elements
- Ensures midground objects appear correctly between layers

## Expected Content
An environmental object like a tree, large rock, fence, or building that would naturally appear in the middle ground of a game scene. Should use medium-bright colors and have some transparent areas to test alpha blending with depth.
