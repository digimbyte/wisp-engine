# test_animated.png - Source Animated Sprite Asset

## Purpose
Source PNG file containing multiple frames for animated sprite testing.

## PNG Requirements
- **Filename**: `test_animated.png`
- **Dimensions**: 192x32 pixels (6 frames of 32x32 each, arranged horizontally)
- **Color Mode**: Indexed color (8-bit) with 256 colors maximum
- **Transparency**: PNG alpha channel or transparent color index
- **Format**: PNG with palette
- **Layout**: Horizontal sprite sheet with 6 frames

## Frame Layout
```
[Frame 1][Frame 2][Frame 3][Frame 4][Frame 5][Frame 6]
 32x32    32x32    32x32    32x32    32x32    32x32
```

## Animation Sequence
Suggested 6-frame walking or idle animation:
1. **Frame 1**: Base pose/position
2. **Frame 2**: Slight movement/change
3. **Frame 3**: Mid-animation pose
4. **Frame 4**: Peak movement
5. **Frame 5**: Return movement
6. **Frame 6**: Almost back to base (smooth loop)

## Visual Design
- Consistent character/object across all frames
- Clear animation motion (walk cycle, idle animation, spinning, etc.)
- Smooth frame transitions for seamless looping
- Examples: Walking character, rotating coin, flickering flame

## Color Palette Guidelines
- Use consistent palette across all frames
- Maintain character colors throughout animation
- Ensure smooth transitions between frames
- Keep palette efficient (32-64 colors)

## Build Process
This PNG file will be converted to `test_animated.spr` using:
```
tools/png_to_spr.exe assets/src/test_animated.png assets/test_animated.spr --frames 6 --frame_width 32 --frame_height 32
```

## Creation Tips
1. Design all frames in a single PNG file
2. Ensure consistent character placement across frames
3. Test animation by quickly flipping between frames
4. Make frame 6 transition smoothly back to frame 1
5. Use onion skinning to see previous/next frames while drawing
