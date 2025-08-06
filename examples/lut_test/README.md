# LUT Test Application

## Purpose
Tests Look-Up Table (LUT) and palette systems for color mapping, palette swapping, and color effects.

## Files
- `lut_test_app.cpp` - Main test application
- `assets/` - Color palette files and documentation

## Asset Requirements
The application expects the following LUT files in the `assets/` folder:
- `test_palette.wlut` - Standard game color palette (256 colors)
- `sepia_lut.wlut` - Sepia tone effect palette
- `night_vision_lut.wlut` - Green night vision effect palette

## Controls
- **Up/Down**: Switch LUT test modes
- **A Button**: Apply selected LUT/effect
- **B Button**: Reset to default palette
- **Left/Right**: Adjust effect parameters
- **Start**: Toggle real-time palette cycling

## LUT Test Modes

### 1. Standard Palette
- Tests basic color mapping
- Full 256-color palette usage
- Color accuracy validation

### 2. Sepia Effect
- Tests color transformation effects
- Warm brown tone mapping
- Vintage photography effect

### 3. Night Vision Effect
- Tests monochromatic effects
- Green-tinted color mapping
- High contrast enhancement

### 4. Custom Palette
- Tests real-time palette editing
- Dynamic color cycling
- User-controlled color effects

## Features Tested
- ✅ LUT file loading and parsing
- ✅ Real-time palette application
- ✅ Color mapping accuracy
- ✅ Palette swapping performance
- ✅ Color effect transformations
- ✅ Dynamic palette editing
- ✅ Color cycling animations
- ✅ Multiple LUT management

## Test Patterns
The application generates various test patterns:
- Color gradient bars
- Checkerboard patterns
- Rainbow cycles
- Grayscale ramps

These patterns help visualize how different LUTs affect color reproduction and validate the accuracy of color mapping operations.

## Usage
This test ensures the LUT system can handle various color palettes, apply effects correctly, and maintain performance during real-time palette operations.
