# test_16x16.spr - Small Sprite Asset

## Purpose
Small 16x16 pixel sprite for basic rendering tests and UI elements.

## Source Requirements
- **Source Format**: PNG file with indexed color palette
- **Source Name**: `test_16x16.png`
- **Dimensions**: 16x16 pixels
- **Color Mode**: Indexed color (256 colors max)
- **Transparency**: PNG transparency (will map to color index 0)

## Output Specifications
- **Output Format**: Wisp Sprite (.spr)
- **Color Depth**: 8-bit indexed color
- **Transparency**: Color index 0 = transparent
- **Animation**: Static (single frame)
- **Build Process**: PNG → SPR conversion via build tools

## Visual Design
This should be a simple, recognizable icon or character:
- Centered design with clear edges
- High contrast colors for visibility
- Simple geometric shape or basic character sprite
- Examples: Small coin, gem, bullet, basic enemy, UI icon

## Usage in Test
- Basic sprite rendering validation
- Performance baseline for small sprites
- UI element testing
- Collision detection reference

## Expected Content
A small, colorful sprite that's easily recognizable when rendered multiple times on screen. Should use distinct colors to test palette mapping and be simple enough to verify correct rendering at small scale.
