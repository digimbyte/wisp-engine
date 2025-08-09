# Wisp Engine App Logic and Script System Design

## Overview

The Wisp Engine provides fundamental building blocks for diverse applications - from RPGs and platformers to memo apps and Pokémon-style games. The system focuses on essential components: sprite manipulation, basic physics, timers, data management, and user interaction.

## Core Philosophy: Essential Building Blocks

- **ROM**: The `.wisp` file containing all app assets, configuration, and logic
- **App**: The running instance loaded from a ROM, executing on the engine
- **Logic**: The executable code within an app (compiled or scripted)
- **Components**: Reusable systems (sprites, physics, timers, data)

## Fundamental Systems

### 1. Sprite System
**Core Features:**
- Position, movement, and animation
- Collision detection and response
- Layer-based rendering
- Frame-based animation sequences
- Flipping and basic transformations

### 2. Physics System (Simplified)
**Essential Components:**
- Velocity and acceleration
- Basic collision shapes (rectangle, circle)
- Collision resolution (bounce, slide, stop)
- Gravity and friction
- Trigger zones

### 3. Timer and Event System
**Time-Based Logic:**
- Countdown timers
- Repeating intervals
- Delayed actions
- State machines
- Event scheduling

### 4. Data Management
**Storage and State:**
- Key-value data storage
- Structured data tables
- Save/load functionality
- Translation support
- Settings persistence

## App Logic Types

### 1. Native C++ Apps (Recommended)
**Format:** `.wash` (Wisp ASsembly Hybrid) - Pre-compiled native ESP32 code
- Direct access to all engine systems
- Maximum performance and control
- Full access to curated API
- Best for complex apps (RPGs, platformers)

### 2. Interpreted Scripts (Simple Apps)
**Format:** `.ash` (App Script Hybrid) - Lightweight scripting
- Easy to write and modify
- Good for simple apps (memo apps, utilities)
- Timer-based behaviors
- Data manipulation focus

### 3. Hybrid Apps (Best of Both)
- Native performance-critical code
- Scripts for content and behavior
- Perfect for RPGs with scripted events
- Pokémon-style games with data-driven content

## ROM Structure for App Logic

```yaml
# WISP ROM Configuration
name: "My Game"
version: "1.0"
author: "Developer"

# App logic entry points
logic:
  main: "main.wash"           # Primary app logic
  scripts: ["ui.ash", "ai.ash"]  # Optional script components
  
# Scene and data assets
scenes:
  - layout: "level1.layout"
    panels: ["main_panel.panel", "ui_panel.panel"]
  
assets:
  sprites: ["player.art", "enemies.art"]
  audio: ["music.sfx", "effects.sfx"]
  palettes: ["main.wlut"]
```

## App Lifecycle with Logic Loading

### Phase 1: ROM Loading
```cpp
// Load ROM file structure
WispSegmentedLoader loader;
loader.openROM("game.wisp");
loader.loadROMStructure();

// Parse app configuration
String appName = loader.getAppName();
String mainLogic = loader.getConfigValue("logic.main");
```

### Phase 2: Logic Initialization
```cpp
// Load main app logic (.wash binary)
const uint8_t* mainBinary = loader.getMainBinary();
WispAppBase* app = loadNativeApp(mainBinary);

// Initialize app with curated API
WispAppInitData initData;
initData.api = &curatedAPI;
initData.sceneManager = &sceneManager;
initData.saveSystem = &saveSystem;
app->init(initData);
```

### Phase 3: Scene Loading
```cpp
// Load and initialize scenes from ROM
for (const auto& sceneName : loader.getSceneNames()) {
    const uint8_t* sceneData = loader.getLayoutData(sceneName);
    sceneManager.loadScene(sceneName, sceneData);
}
```

### Phase 4: Runtime Execution
```cpp
// App logic runs with access to:
// - Curated API for engine features
// - Scene system for game world
// - Asset streaming from ROM
// - Save system for persistence

while (running) {
    app->update();    // App logic update
    app->render();    // App rendering
}
```

## Script System Integration

### Entity Scripts
```cpp
// Entities can reference script logic
struct SceneEntity {
    uint16_t scriptId;        // Script identifier
    void* scriptData;         // Script instance data
    
    // Script execution hooks
    virtual void onUpdate() { 
        if (scriptId) executeEntityScript(scriptId, this); 
    }
    virtual void onCollision(uint16_t otherId) {
        if (scriptId) executeCollisionScript(scriptId, this, otherId);
    }
};
```

### Panel Scripts
```cpp
// Panels can have associated logic
struct ScenePanel {
    uint16_t updateScriptId;  // Script for panel updates
    uint16_t inputScriptId;   // Script for input handling
    
    void updatePanel() {
        if (updateScriptId) executePanelScript(updateScriptId, this);
    }
};
```

### Translation and Localization
```cpp
// Translation system integrated with scene data
class TranslationSystem {
    // Load translation data from ROM
    bool loadTranslations(const String& language);
    
    // Scene text replacement
    String translateText(const String& key);
    void updateSceneText(ScenePanel* panel, const String& language);
};
```

## Animation and Sprite Management

### Sprite Animation (No Rotation Support)
```cpp
// Sprite animation through frame sequences
struct SpriteAnimation {
    uint16_t spriteBaseId;    // Base sprite ID
    uint8_t frameCount;       // Number of animation frames
    uint16_t frameDelayMs;    // Delay between frames
    bool looping;             // Should animation loop
    
    // Animation states from sprite definitions
    enum AnimState {
        IDLE, WALK, RUN, JUMP, ATTACK, HURT, DEATH
    };
    
    void playAnimation(AnimState state);
    void updateAnimation(uint32_t currentTime);
};
```

### Position and Movement Data
```cpp
// Scene entities handle position and movement
struct SceneEntity {
    // World coordinates (fixed-point for precision)
    int32_t worldX, worldY;   // 16.16 fixed point
    int32_t velocityX, velocityY;
    
    // Movement patterns (instead of rotation)
    enum MovementType {
        STATIC, LINEAR, CIRCULAR, SINE_WAVE, BEZIER, SCRIPTED
    };
    
    MovementType movementType;
    void* movementData;       // Type-specific movement parameters
    
    // Sprite flipping for direction indication
    bool flipX, flipY;        // Horizontal/vertical flipping
    uint8_t facingDirection;  // 4 or 8-direction facing
};
```

## Asset Streaming and Caching

### Smart Asset Management
```cpp
class SceneAssetManager {
public:
    // Preload critical assets for current scene
    void preloadSceneAssets(const String& sceneName);
    
    // Stream assets as needed for entities
    uint16_t getSpriteId(const String& spriteName);
    uint16_t getAudioId(const String& audioName);
    
    // Manage memory budget per scene
    void setSceneMemoryBudget(uint32_t bytes);
    void evictUnusedAssets();
    
private:
    WispSegmentedLoader* romLoader;
    LRUCache<String, uint16_t> spriteCache;
    LRUCache<String, uint16_t> audioCache;
};
```

### Session State Management
```cpp
class SessionManager {
public:
    // Handle scene transitions
    void transitionToScene(const String& sceneName);
    void pushSceneState();  // For temporary overlays
    void popSceneState();   // Return from overlay
    
    // Session data persistence
    void saveSessionData();
    void loadSessionData();
    
    // App state management
    struct SessionData {
        String currentScene;
        uint32_t playTime;
        HashMap<String, Variant> gameVariables;
        Vector<String> unlockedAchievements;
    };
    
private:
    SessionData session;
    SaveSystem* saveSystem;
};
```

## Security and Sandboxing

### App API Boundaries
```cpp
// Apps can ONLY access engine features through curated API
class WispCuratedAPI {
    // Scene manipulation
    bool loadScene(const String& name);
    bool transitionToScene(const String& name);
    
    // Entity management
    EntityHandle createEntity(uint16_t spriteId);
    bool moveEntity(EntityHandle handle, int32_t x, int32_t y);
    bool playEntityAnimation(EntityHandle handle, uint8_t animId);
    
    // Asset access
    SpriteHandle getSprite(const String& name);
    AudioHandle getAudio(const String& name);
    
    // NO direct memory access
    // NO file system access
    // NO network access (unless explicitly granted)
};
```

### Memory Safety
```cpp
// All app data contained within allocated boundaries
class AppSandbox {
    uint8_t* appMemory;      // Isolated app memory region
    uint32_t memorySize;     // Memory boundary
    uint32_t memoryUsed;     // Current usage
    
    // Memory allocation within sandbox
    void* appMalloc(size_t size);
    void appFree(void* ptr);
    
    // Prevent buffer overruns
    bool validateMemoryAccess(void* ptr, size_t size);
};
```

## Implementation Priority

### Phase 1 (Current): Native C++ Apps
1. ✅ WISP ROM loading and structure parsing
2. ✅ Asset streaming with segmented loader  
3. ✅ Scene system with panels and entities
4. 🔄 Native app (.wash) loading and execution
5. 🔄 Curated API integration with scene system

### Phase 2 (Near Future): Enhanced Scene System
1. Entity script hooks for behavior
2. Animation system with frame-based sprites
3. Translation/localization system
4. Session state management
5. Advanced asset caching strategies

### Phase 3 (Future): Script Interpreter
1. .ash script format and bytecode
2. Script interpreter with sandboxing
3. Hot-reloadable script components
4. Visual scripting tools for game development

## Example App Structure

```
my_game.wisp
├── config.yaml           # App metadata and configuration
├── main.wash             # Compiled main app logic
├── scripts/
│   ├── ui.ash           # UI interaction scripts
│   └── ai.ash           # Entity AI scripts
├── scenes/
│   ├── level1.layout    # Scene layout definition
│   ├── main_panel.panel # Main game panel
│   └── ui_panel.panel   # UI overlay panel
├── assets/
│   ├── sprites/
│   │   ├── player.art   # Player sprite frames
│   │   └── enemies.art  # Enemy sprite collection
│   ├── audio/
│   │   ├── music.sfx    # Background music
│   │   └── effects.sfx  # Sound effects
│   └── palettes/
│       └── main.wlut    # Main color palette
└── data/
    ├── translations.json # Localization data
    └── save_schema.json  # Save data structure
```

This design provides a robust foundation for app logic while maintaining the security, performance, and architectural goals of the Wisp Engine. The system is extensible and can grow from simple compiled apps to sophisticated scripted experiences.
