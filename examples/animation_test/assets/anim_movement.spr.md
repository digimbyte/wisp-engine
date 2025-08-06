# anim_movement.spr - Movement Animation Test Sprite

## Purpose
Simple sprite for testing position-based animation and movement interpolation.

## Specifications
- **Dimensions**: 24x24 pixels
- **Format**: Wisp Sprite (.spr)
- **Color Depth**: 8-bit indexed color (256 colors)
- **Transparency**: Yes, color index 0 = transparent
- **Animation**: Static sprite (movement handled by engine)
- **Usage**: Position interpolation testing

## Visual Design
Simple, easily trackable sprite:
- High contrast colors for visibility during movement
- Directional indicator (arrow, pointed shape)
- Simple geometric design
- Clear center point for position tracking

## Movement Patterns
Used for testing various movement animations:
- Linear interpolation (straight lines)
- Curved paths (bezier, circular)
- Easing functions (ease-in, ease-out, bounce)
- Speed variations
- Direction changes

## Usage in Test
- Position interpolation validation
- Movement smoothness testing
- Path following accuracy
- Easing function verification
- Speed control testing

## Expected Content
A simple arrow, diamond, or pointed geometric shape with bright colors that's easy to track as it moves across the screen. Should have a clear orientation to show rotation during movement and high contrast colors to remain visible against various backgrounds.
