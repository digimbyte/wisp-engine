# Depth Test Application

## Purpose
Tests depth buffer functionality, Z-ordering, and sprite layer management to ensure proper rendering order.

## Files
- `depth_test_app.cpp` - Main test application
- `assets/` - Depth testing sprites and documentation

## Asset Requirements
The application expects the following sprite files in the `assets/` folder:
- `depth_layer_bg.spr` - Background layer sprite (128x128)
- `depth_layer_mid.spr` - Midground layer sprite (64x64)  
- `depth_layer_fg.spr` - Foreground layer sprite (48x48)

## Controls
- **Up/Down**: Navigate through depth layers
- **A Button**: Swap selected layer depth with adjacent layer
- **B Button**: Reset all layers to default depths
- **Left/Right**: Move selected layer horizontally

## Features Tested
- ✅ Depth layer sorting (Z-order)
- ✅ Interactive depth manipulation
- ✅ Visual depth indicators
- ✅ Multiple overlapping sprites
- ✅ Depth buffer accuracy
- ✅ Real-time depth swapping
- ✅ Layer visibility and ordering

## Test Scenarios
The app creates 10 colored depth layers that can be manipulated to test:
- Correct depth sorting algorithms
- Visual feedback for depth changes  
- Performance with multiple depth layers
- Edge cases in depth management

## Visual Indicators
- Each layer has a unique color for easy identification
- Depth numbers are displayed on each layer
- Selected layer is highlighted
- Moving layers demonstrate depth interactions

## Usage
This test validates that the graphics engine properly handles sprite depth sorting and can maintain correct rendering order even when depths are changed dynamically.
