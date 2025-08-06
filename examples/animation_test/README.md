# Animation Test Application

## Purpose
Comprehensive testing of animation systems including frame-based animation, interpolation, easing functions, and timing control.

## Files
- `animation_test_app.cpp` - Main test application
- `assets/` - Animation sprites and documentation

## Asset Requirements
The application expects the following sprite files in the `assets/` folder:
- `anim_frames.spr` - Multi-frame sprite for frame-based animation
- `anim_movement.spr` - Simple sprite for movement testing
- `anim_scale.spr` - Sprite optimized for scaling tests

## Controls
- **Up/Down**: Select animation type
- **A Button**: Play/Pause animation
- **B Button**: Reset animation to start
- **Left/Right**: Adjust animation speed
- **Start**: Toggle all animations on/off

## Animation Types

### 1. Frame Animation
- Tests frame cycling and timing
- Uses multi-frame sprite assets
- Validates frame interpolation

### 2. Position Animation  
- Tests movement interpolation
- Linear and curved path following
- Easing function validation

### 3. Scale Animation
- Tests size transformation
- Uniform and non-uniform scaling
- Scale easing effects

### 4. Color Animation
- Tests color interpolation
- HSV color space cycling
- Smooth color transitions

### 5. Rotation Animation
- Tests rotation transforms
- Smooth rotation interpolation
- Multiple rotation speeds

## Features Tested
- ✅ Frame-based sprite animation
- ✅ Position interpolation (linear, bezier)
- ✅ Scale transformation animation
- ✅ Color cycling and interpolation
- ✅ Rotation animation
- ✅ Easing functions (ease-in, ease-out, bounce)
- ✅ Animation timing and speed control
- ✅ Play/pause/reset controls
- ✅ Multiple simultaneous animations

## Easing Functions
Tests various easing curves:
- Linear interpolation
- Ease-in (slow start)
- Ease-out (slow end)
- Ease-in-out (slow start and end)
- Bounce effects
- Elastic effects

## Usage
This test validates the animation system's ability to smoothly interpolate between values, maintain consistent timing, and provide flexible control over animation playback.
