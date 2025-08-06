# Example Applications - Feature Test Suite

This directory contains focused test applications for validating individual Wisp Engine subsystems. Each example is organized in its own subfolder with dedicated assets and documentation.

## Directory Structure

```
examples/
├── sprite_test/           # Sprite System Testing
│   ├── sprite_test_app.cpp
│   ├── assets/
│   │   ├── test_16x16.spr.md
│   │   ├── test_32x32.spr.md
│   │   ├── test_64x64.spr.md
│   │   └── test_animated.spr.md
│   └── README.md
├── depth_test/            # Depth Buffer & Z-Ordering
│   ├── depth_test_app.cpp
│   ├── assets/
│   │   ├── depth_layer_bg.spr.md
│   │   ├── depth_layer_mid.spr.md
│   │   └── depth_layer_fg.spr.md
│   └── README.md
├── animation_test/        # Animation System Testing
│   ├── animation_test_app.cpp
│   ├── assets/
│   │   ├── anim_frames.spr.md
│   │   ├── anim_movement.spr.md
│   │   └── anim_scale.spr.md
│   └── README.md
├── lut_test/              # LUT & Palette Testing
│   ├── lut_test_app.cpp
│   ├── assets/
│   │   ├── test_palette.wlut.md
│   │   ├── sepia_lut.wlut.md
│   │   └── night_vision_lut.wlut.md
│   └── README.md
├── audio_test/            # Audio System Testing
│   ├── audio_test_app.cpp
│   ├── assets/
│   │   ├── test_bgm_calm.wbgm.md
│   │   ├── test_sfx_beep.wsfx.md
│   │   └── test_cry_pikachu.wcry.md
│   └── README.md
├── network_test/          # Network & WiFi Testing
│   ├── network_test_app.cpp
│   └── README.md
├── database_test/         # Database CRUD Testing
│   ├── database_test_app.cpp
│   └── README.md
└── save_test/             # Save/Load System Testing
    ├── save_test_app.cpp
    └── README.md
```

## Test Applications

### Core Graphics Tests
- **sprite_test** - Sprite loading, rendering, movement, depth layering
- **depth_test** - Z-ordering, depth buffer functionality
- **animation_test** - Frame animation, interpolation, easing functions
- **lut_test** - Color palettes, LUT effects, palette swapping

### Audio & Multimedia Tests  
- **audio_test** - BGM, SFX, cry synthesis, multi-channel mixing

### System & I/O Tests
- **network_test** - WiFi connectivity, HTTP API methods (GET/POST/PATCH)
- **database_test** - CRUD operations, field validation, batch processing
- **save_test** - Save/load operations, file management, data persistence

## Asset Creation Workflow

The Wisp Engine uses custom formats for optimized performance. Assets must be converted from standard formats:

### Source → Engine Format Conversion
- **PNG → SPR**: Sprite images with indexed color palettes
- **WAV → WBGM/WSFX/WCRY**: Audio files optimized for different uses
- **PNG → WLUT**: Color palette files for LUT operations

### Directory Structure
Each test application has this asset structure:
```
test_app/
├── assets/
│   ├── src/                    # Source files (PNG, WAV)
│   │   ├── sprite.png.md      # Documentation for source assets
│   │   └── sprite.png         # Actual source file (you create)
│   ├── sprite.spr             # Converted engine format
│   └── sprite.spr.md          # Documentation for engine format
```

### Build Process
1. **Create Source Assets**: Place PNG/WAV files in `assets/src/` folders
2. **Run Asset Builder**: Use the build script to convert assets
3. **Engine Loads**: Applications load the converted `.spr/.wbgm/.wlut` files

### Asset Builder Commands
```powershell
# Build assets for specific test
.\build_assets.ps1 -ExampleDir sprite_test

# Build all example assets
.\build_assets.ps1 -All

# Force rebuild (ignore timestamps)
.\build_assets.ps1 -All -Force

# Verbose output for debugging
.\build_assets.ps1 -All -Verbose
```

## Running Tests

Each test application follows the WispAppBase pattern and can be built/run independently. The applications use relative paths to load assets from their local `assets/` folders.

## Development Workflow

1. **Choose Test Area** - Select the engine feature you want to test
2. **Review Documentation** - Read the README and asset specifications
3. **Create Assets** - Build the required assets based on the `.md` guides
4. **Run Test** - Compile and execute the test application
5. **Validate Results** - Use the interactive controls to verify functionality

This organized structure makes it easy to focus on specific engine features, manage assets systematically, and maintain clear documentation for each test scenario.
