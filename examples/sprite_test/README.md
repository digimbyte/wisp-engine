# Sprite Test Application

## Purpose
Comprehensive testing of the Wisp Engine sprite system including loading, rendering, scaling, movement, and depth layering.

## Files
- `sprite_test_app.cpp` - Main test application
- `assets/` - Sprite assets and documentation

## Asset Requirements

### Source Assets (Create These)
Place the following PNG files in the `assets/src/` folder:
- `test_16x16.png` - Small sprite source (16x16 pixels, indexed color)
- `test_32x32.png` - Medium sprite source (32x32 pixels, indexed color)
- `test_64x64.png` - Large sprite source (64x64 pixels, indexed color)
- `test_animated.png` - Animated sprite source (192x32 pixels, 6 frames)

### Engine Assets (Auto-Generated)
The build process converts these to `.spr` format:
- `test_16x16.spr` - Small sprite for basic rendering tests
- `test_32x32.spr` - Medium sprite for standard game objects
- `test_64x64.spr` - Large sprite for performance and detail testing
- `test_animated.spr` - Multi-frame animated sprite

## Building Assets

To convert your PNG files to Wisp sprite format:

```powershell
# From project root directory
.\build_assets.ps1 -ExampleDir sprite_test
```

Or build all examples at once:
```powershell
.\build_assets.ps1 -All
```

## Controls
- **A Button**: Add a new sprite (up to 16 total)
- **B Button**: Remove a sprite (minimum 1)

## Features Tested
- ✅ Sprite loading from file
- ✅ Basic sprite rendering
- ✅ Multiple sprites with different sizes
- ✅ Depth layering (Z-ordering)
- ✅ Animated sprites with frame cycling
- ✅ Movement physics with screen boundary collision
- ✅ Real-time sprite management (add/remove)
- ✅ Performance with multiple active sprites

## Asset Creation Guide
See the `.md` files in the `assets/src/` folder for detailed specifications of what each PNG source file should contain. These files describe the expected dimensions, color usage, and visual content for each sprite.

The `.md` files in the `assets/` folder describe the final engine format specifications.

## Usage
This test app validates that the sprite system can handle various sprite sizes, manage multiple sprites simultaneously, and maintain proper depth sorting. It's essential for validating graphics performance and rendering accuracy.
