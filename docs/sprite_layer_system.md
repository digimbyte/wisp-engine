# Wisp Engine Sprite Layer System

## Overview

The Wisp Engine uses an 8-layer sprite rendering system that provides depth control, animations, tiling, and UI support. Each layer has specific purposes and behaviors designed to recreate Game Boy Advance-style graphics with modern enhancements.

## Layer Structure

### Layer 0: Gradients
- **Purpose**: Simple background gradients and fills
- **Type**: `SPRITE_TYPE_GRADIENT`
- **Features**: Basic color gradients, solid colors
- **Use Cases**: Sky gradients, simple backgrounds, color washes

### Layer 1: Backgrounds
- **Purpose**: Static background sprites with tiling support
- **Type**: `SPRITE_TYPE_BACKGROUND`
- **Features**: Infinite tiling, parallax scrolling, mirroring modes
- **Use Cases**: Repeating textures, scrolling backgrounds, environment tiles

### Layers 2-6: Game Sprites
- **Layer 2**: Back game objects (distant objects, back decorations)
- **Layer 3**: Mid game objects (main gameplay elements, player)
- **Layer 4**: Front game objects (foreground elements, enemies)
- **Layer 5**: Top game objects (projectiles, items)
- **Layer 6**: Effects (particles, explosions, animations)
- **Type**: `SPRITE_TYPE_STANDARD`
- **Features**: Full animation support, depth masking, transforms
- **Use Cases**: Characters, items, effects, gameplay elements

### Layer 7: UI Elements
- **Purpose**: User interface sprites
- **Type**: `SPRITE_TYPE_UI`
- **Features**: 9-patch slicing, fixed screen positioning
- **Use Cases**: Buttons, panels, health bars, menus

### Layer 8: Text and Overlays
- **Purpose**: Text rendering and top-level overlays
- **Type**: `SPRITE_TYPE_TEXT`
- **Features**: Text rendering, highest priority
- **Use Cases**: Score displays, dialogue, debug text

## Key Features

### Depth Masking
Sprites can appear on multiple layers with different opacity/depth values:

```cpp
// Create a tree that appears on multiple layers
WispLayeredSprite* tree = layerSystem->createGameSprite(treeSprite, LAYER_3_GAME_MID);

// Make it appear on background, mid, and front layers
layerSystem->setMultiLayer(tree, {LAYER_2_GAME_BACK, LAYER_3_GAME_MID, LAYER_4_GAME_FRONT});

// Set different depths for each layer
layerSystem->setLayerDepth(tree, LAYER_2_GAME_BACK, 8);   // Faded on back
layerSystem->setLayerDepth(tree, LAYER_3_GAME_MID, 10);   // Full opacity on mid
layerSystem->setLayerDepth(tree, LAYER_4_GAME_FRONT, 6);  // More faded on front
```

### Tiling System (Layer 1)
Background sprites support multiple tiling modes:

```cpp
WispLayeredSprite* bg = layerSystem->createBackgroundSprite(bgSprite, TILE_REPEAT);
bg->parallaxX = 0.5f; // Slower horizontal parallax
bg->parallaxY = 0.8f; // Slower vertical parallax
bg->scrollX = 10.0f;  // Manual scroll offset
bg->scrollY = 5.0f;
```

**Tiling Modes:**
- `TILE_NONE`: No tiling (stretch or clip)
- `TILE_REPEAT`: Repeat infinitely
- `TILE_REPEAT_X`: Repeat horizontally only
- `TILE_REPEAT_Y`: Repeat vertically only
- `TILE_MIRROR`: Mirror at edges
- `TILE_MIRROR_X`: Mirror horizontally
- `TILE_MIRROR_Y`: Mirror vertically

### Animation System
Supports frame-based animations with timing control:

```cpp
// Create animation frames
std::vector<WispAnimationFrame> walkFrames = {
    WispAnimationFrame(0, 150), // Frame 0 for 150ms
    WispAnimationFrame(1, 150), // Frame 1 for 150ms
    WispAnimationFrame(2, 150), // Frame 2 for 150ms
    WispAnimationFrame(3, 150)  // Frame 3 for 150ms
};

// Apply to sprite
layerSystem->setAnimation(player, walkFrames);
layerSystem->playAnimation(player, true); // Loop animation

// Animation with frame offsets
WispAnimationFrame frameWithOffset(1, 200, 2, -1); // Frame 1, 200ms, +2x, -1y offset
```

**Animation Features:**
- Frame timing control
- Per-frame position offsets
- Per-frame alpha values
- Loop and ping-pong modes
- Pause/resume support

### 9-Patch UI Slicing (Layer 7)
UI sprites support 9-patch slicing for scalable interface elements:

```cpp
WispLayeredSprite* panel = layerSystem->createUISprite(panelSprite, 50, 100);
panel->slice = WispSpriteSlice(8, 56, 8, 24); // left, right, top, bottom borders
panel->targetWidth = 200;  // Scale to 200 pixels wide
panel->targetHeight = 80;  // Scale to 80 pixels tall
```

### Camera System
Camera controls affect layers 1-6 (not UI/text layers):

```cpp
// Set camera position (instant)
layerSystem->setCamera(playerX - 160, playerY - 120);

// Smooth camera following
layerSystem->setCameraSmooth(targetX, targetY, 0.1f); // 10% smoothing

// Different layers can have different parallax speeds
backgroundSprite->parallaxX = 0.5f; // Moves at half camera speed
foregroundSprite->parallaxX = 1.2f; // Moves faster than camera
```

## Usage Examples

### Basic Sprite Creation

```cpp
// Layer 0: Gradient background
WispLayeredSprite* gradient = layerSystem->createGradientSprite(0, 0, 320, 240, 
                                                               0x001F, 0x7C00); // Blue to red

// Layer 1: Tiled background
WispLayeredSprite* tiles = layerSystem->createBackgroundSprite(tileSprite, TILE_REPEAT);

// Layer 3: Player character
WispLayeredSprite* player = layerSystem->createGameSprite(playerSprite, LAYER_3_GAME_MID);
player->x = 160;
player->y = 120;

// Layer 7: Health bar UI
WispLayeredSprite* healthBar = layerSystem->createUISprite(healthBarSprite, 10, 10);

// Layer 8: Score text
WispLayeredSprite* score = layerSystem->createTextSprite("SCORE: 0", 10, 220);
```

### Layer Management

```cpp
// Hide/show layers
layerSystem->setLayerEnabled(LAYER_6_EFFECTS, false); // Hide effects
layerSystem->setLayerEnabled(LAYER_7_UI, true);       // Show UI

// Adjust layer transparency
layerSystem->setLayerAlpha(LAYER_6_EFFECTS, 128); // 50% transparency

// Clear specific layer
layerSystem->clearLayer(LAYER_6_EFFECTS);

// Clear all sprites
layerSystem->clearAllSprites();
```

### Rendering Loop

```cpp
void gameLoop() {
    uint32_t deltaTime = getDeltaTime();
    
    // Update animations
    layerSystem->updateAnimations(deltaTime);
    
    // Update game logic
    updateGame();
    
    // Clear buffers
    graphics->clearBuffers(0x0000);
    
    // Render all layers in order
    layerSystem->renderAllLayers();
    
    // Present to screen
    graphics->present();
}
```

### Performance Considerations

**Rendering Order:**
1. Layers render bottom-to-top (0→8)
2. Within each layer, sprites render by `renderPriority` (0=back, 255=front)
3. Viewport culling automatically skips off-screen sprites
4. Layer-specific optimizations (e.g., tiling calculations only for layer 1)

**Memory Usage:**
- Each sprite: ~100-200 bytes
- Animation data: ~16 bytes per frame
- Layer sorting: Minimal overhead with small sprite counts
- Depth masking: Additional memory only for multi-layer sprites

**Optimization Tips:**
- Use appropriate layers (don't put static backgrounds on game layers)
- Set `renderPriority` to minimize sorting
- Disable unused layers with `setLayerEnabled()`
- Use viewport culling for large worlds
- Limit animation frame counts for memory efficiency

## Integration with Graphics Engine

The layer system integrates with the existing graphics engine:

```cpp
// Initialize in bootloader
layerSystem = new WispSpriteLayerSystem(graphics);
layerSystem->setViewport(SCREEN_WIDTH, SCREEN_HEIGHT);
g_LayerSystem = layerSystem; // Set global instance

// Layer system uses these graphics functions:
// - drawSprite() for standard rendering
// - drawSpriteRegion() for 9-patch slicing
// - drawGradient() for layer 0
// - setGlobalAlpha() for layer transparency
// - setAlpha() for individual sprite alpha
```

## Best Practices

1. **Layer Assignment**: Use layers according to their intended purpose
2. **Render Priority**: Set meaningful priorities within layers (0-255)
3. **Animation Timing**: Use consistent frame timing for smooth animations
4. **Parallax**: Use subtle parallax values for depth (0.5-1.5 range)
5. **UI Positioning**: Keep UI sprites at fixed screen positions
6. **Depth Masking**: Use sparingly for special effects (trees, fog, etc.)
7. **Tiling**: Use power-of-2 tile sizes for optimal performance
8. **Alpha**: Minimize alpha blending for performance

This sprite layer system provides the foundation for GBA-style games while adding modern features like depth masking and advanced tiling modes.
