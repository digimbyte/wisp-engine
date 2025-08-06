# anim_frames.spr - Frame-Based Animation Sprite

## Purpose
Multi-frame sprite for testing frame-based animation systems.

## Specifications
- **Dimensions**: 32x32 pixels per frame
- **Format**: Wisp Sprite (.spr) with frame data
- **Color Depth**: 8-bit indexed color (256 colors)
- **Transparency**: Yes, color index 0 = transparent
- **Animation**: 8 frames, seamless loop
- **Frame Rate**: 12 FPS default

## Animation Sequence
8-frame walking or idle animation:
1. **Frame 1**: Base pose
2. **Frame 2**: Step forward (left foot)
3. **Frame 3**: Mid-stride
4. **Frame 4**: Step forward (right foot)
5. **Frame 5**: Return to center
6. **Frame 6**: Step back (left foot)
7. **Frame 7**: Mid-stride back
8. **Frame 8**: Return to base pose

## Visual Design
Character animation with clear frame progression:
- Consistent character design across all frames
- Clear movement indication between frames
- Smooth transitions for seamless looping
- Distinct pose changes for easy frame identification

## Usage in Test
- Frame cycling validation
- Animation timing tests
- Frame interpolation testing
- Memory usage with frame data
- Loop seamlessness validation

## Expected Content
A clear character walk cycle or idle animation that demonstrates smooth frame-based animation. Each frame should be distinctly different enough to validate proper frame timing and cycling.
