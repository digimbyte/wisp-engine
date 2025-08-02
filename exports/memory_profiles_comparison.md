# ESP32-C6 Memory Profiles for Game Development

Based on the ESP32-C6's **512KB HP SRAM** total capacity, here are three optimized configurations for different types of games:

## Hardware Reality Check

**ESP32-C6 Memory Architecture:**
- **HP SRAM**: 512KB - Main working memory (ALL game code, graphics, sprites live here)
- **LP SRAM**: 16KB - Low-power sleep memory (NOT usable for normal game operations)
- **Flash**: 4MB - Asset storage (sprites, sounds, levels can be loaded from here)
- **ROM**: 320KB - System firmware (not available to games)

**The 512KB HP SRAM must contain:**
- Arduino framework (~60KB)
- WiFi stack (~40KB if enabled)  
- Bluetooth stack (~20KB if enabled)
- Your game's code and variables
- Graphics buffers
- Sprite management
- Audio buffers
- Stack space and heap

## Memory Profile Comparison

### PROFILE_MINIMAL - "Game Boy Style" (384KB available for games)
**Philosophy**: Maximum memory for game logic, minimal graphics overhead

```cpp
#define WISP_MEMORY_PROFILE PROFILE_MINIMAL
```

**Memory Allocation:**
- System overhead: 92KB (Arduino + buffers, no WiFi/BT)
- Graphics engine: 8KB (tile-based rendering, 32x32 LUT)
- Sprite system: 4KB (32 sprites, 4 layers)
- Audio system: 4KB (basic audio)
- **Game logic space: 404KB** ⭐

**Features:**
- ✅ Tile-based rendering (no framebuffer)
- ✅ 32 sprites maximum
- ✅ 4 layers (background, game, UI, text)
- ✅ Simple 8-frame animations
- ❌ No WiFi/Bluetooth (saves 60KB)
- ❌ No full-screen effects
- ❌ No depth buffer

**Best for**: RPGs, puzzle games, turn-based games, retro arcade games

---

### PROFILE_BALANCED - "Game Boy Advance Style" (326KB available for games)
**Philosophy**: Good balance of visual features and game logic memory

```cpp
#define WISP_MEMORY_PROFILE PROFILE_BALANCED  
```

**Memory Allocation:**
- System overhead: 132KB (Arduino + WiFi stack, no Bluetooth)
- Graphics engine: 30KB (32-line strip buffers, 64x64 LUT)
- Sprite system: 12KB (64 sprites, 6 layers)
- Audio system: 8KB (multi-channel audio)
- **Game logic space: 330KB** ⭐

**Features:**
- ✅ Strip-based rendering (32-line strips, smooth scrolling)
- ✅ 64 sprites maximum
- ✅ 6 layers (background, game BG/main/FG, particles, UI)
- ✅ WiFi enabled (multiplayer, cloud saves)
- ✅ Better audio (4 channels)
- ❌ No full framebuffer
- ❌ No depth buffer

**Best for**: Action games, platformers, shoot-em-ups, multiplayer games

---

### PROFILE_FULL - "Modern Indie Game Style" (164KB available for games)
**Philosophy**: Maximum visual features, use Flash storage for large assets

```cpp
#define WISP_MEMORY_PROFILE PROFILE_FULL
```

**Memory Allocation:**
- System overhead: 152KB (Arduino + WiFi + Bluetooth stacks)
- Graphics engine: 194KB (full double framebuffer + depth buffer + 128x128 LUT)
- Sprite system: 24KB (128 sprites, 8 layers with effects)
- Audio system: 16KB (6-channel audio with effects)
- **Game logic space: 126KB** ⭐

**Features:**
- ✅ Full RGB565 framebuffer (172×320)
- ✅ Double buffering (smooth 60fps)
- ✅ Depth buffer (sprite layering effects)
- ✅ 128 sprites maximum
- ✅ 8 layers with advanced effects
- ✅ WiFi + Bluetooth (full connectivity)
- ✅ High-quality audio
- ⚠️ Less memory for game logic (use Flash for assets)

**Best for**: Visual showcases, particle effects, smooth animations, demos

## Development Strategy

### Asset Management
- **Minimal/Balanced**: Store core assets in HP SRAM, load extras from Flash as needed
- **Full**: Store only active assets in HP SRAM, stream everything else from 4MB Flash

### Performance Targets
- **60 FPS**: Possible with all profiles using different rendering strategies
- **30 FPS**: Comfortable for all profiles with complex game logic
- **Variable FPS**: Use frame pacing for consistent feel

### Profile Selection Guide

**Choose MINIMAL if:**
- Complex game logic (AI, physics, large worlds)
- Turn-based or slower-paced games
- Retro aesthetic is desired
- Offline-only games

**Choose BALANCED if:**
- Real-time action games
- Need WiFi multiplayer
- Want smooth scrolling
- Modern pixel art style

**Choose FULL if:**
- Visual effects are priority
- Need maximum sprite count
- Smooth 60fps is required
- Using Flash storage for assets

## Memory Usage Examples

### MINIMAL Profile Game
```cpp
// 404KB available for game logic
uint8_t gameWorld[200 * 1024];      // 200KB world data
uint8_t gameEntities[100 * 1024];   // 100KB entities/AI
uint8_t gameAudio[50 * 1024];       // 50KB audio samples
uint8_t gameScripts[54 * 1024];     // 54KB game logic
// Total: 404KB - perfect fit!
```

### BALANCED Profile Game  
```cpp
// 330KB available for game logic
uint8_t activeLevels[150 * 1024];   // 150KB current level
uint8_t gameState[80 * 1024];       // 80KB game state
uint8_t audioSamples[100 * 1024];   // 100KB audio
// Total: 330KB - good balance
```

### FULL Profile Game
```cpp
// 126KB available for game logic - use Flash for assets!
uint8_t currentScene[64 * 1024];    // 64KB active scene
uint8_t gameLogic[32 * 1024];       // 32KB core logic  
uint8_t audioStreams[30 * 1024];    // 30KB audio buffers
// Total: 126KB - stream assets from Flash
```

## Conclusion

The ESP32-C6 with 512KB HP SRAM is surprisingly capable! The key insight is that different games need different memory allocation strategies. The configuration system lets developers choose the right balance for their specific game type, from memory-hungry retro RPGs to visually stunning modern indies.

**Recommendation**: Start with `PROFILE_BALANCED` for most games, then switch to `PROFILE_MINIMAL` if you need more logic space, or `PROFILE_FULL` if you want maximum visual impact.
