# test_animated.spr - Animated Sprite Asset

## Purpose
Multi-frame animated sprite for animation system testing.

## Specifications
- **Dimensions**: 32x32 pixels per frame
- **Format**: Wisp Sprite (.spr) with animation data
- **Color Depth**: 8-bit indexed color (256 colors)
- **Transparency**: Yes, color index 0 = transparent
- **Animation**: 4-8 frames, looping
- **Frame Rate**: 8-12 FPS recommended

## Visual Design
This should be a simple animated sequence:
- Consistent character/object across all frames
- Clear animation motion (walk cycle, idle animation, spinning, etc.)
- Smooth frame transitions
- Examples: Walking character, rotating coin, flickering flame, bouncing ball

## Animation Frames
Suggested 6-frame sequence:
1. **Frame 1**: Base pose/position
2. **Frame 2**: Slight movement/change
3. **Frame 3**: Mid-animation pose
4. **Frame 4**: Peak movement
5. **Frame 5**: Return movement
6. **Frame 6**: Almost back to base (smooth loop)

## Usage in Test
- Animation frame cycling
- Frame timing validation
- Animation loop testing
- Memory usage with multiple frames
- Frame interpolation testing

## Expected Content
A simple but clear animated sequence that demonstrates smooth animation capabilities. Should be easily recognizable when animating and provide good visual feedback for testing frame timing and animation systems.
