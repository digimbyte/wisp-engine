// scene_system.h
// WISP Engine Scene Management System
// Provides GBA/SNES-style scene architecture with layouts, panels, backgrounds, entities, and tiles
#pragma once

#include "engine_common.h"
#include "../graphics/sprite_system.h"
#include "../entities/system.h"
#include "../audio/audio_engine.h"

namespace WispEngine {

// Forward declarations
struct SceneLayout;
struct ScenePanel;
struct SceneBackground;
struct SceneTile;
class SceneManager;

// === CORE SCENE TYPES ===

// Panel focus modes - how the camera follows entities
enum class PanelFocusMode : uint8_t {
    FIXED,          // Panel never moves - static camera
    FOLLOW_SMOOTH,  // Smooth camera following with easing
    FOLLOW_SNAP,    // Instant camera following - snaps to entity
    FOLLOW_BOUNDED, // Follow within defined boundaries
    MANUAL          // Manually controlled camera position
};

// Background rendering modes
enum class BackgroundMode : uint8_t {
    STATIC,         // Fixed background image
    PARALLAX_H,     // Horizontal parallax scrolling
    PARALLAX_V,     // Vertical parallax scrolling  
    PARALLAX_BOTH,  // Both horizontal and vertical parallax
    ANIMATED,       // Animated background sequence
    TILED           // Repeating tiled background
};

// Entity behavior types within scenes
enum class EntityBehavior : uint8_t {
    STATIC,         // Never moves, purely visual
    KINEMATIC,      // Moves but not affected by physics
    DYNAMIC,        // Full physics simulation
    SCRIPTED,       // Movement controlled by logic scripts
    PLAYER,         // Player-controlled entity
    AI_DRIVEN       // AI/pattern-driven movement
};

// Tile interaction types
enum class TileType : uint8_t {
    BACKGROUND,     // Pure visual, no collision
    WALL,           // Solid collision boundary
    FLOOR,          // Walkable surface with collision
    PLATFORM,       // One-way collision (jump-through)
    TRIGGER,        // Invisible interaction zone
    ANIMATED        // Animated tile sequence
};

// === SCENE BACKGROUND ===

struct SceneBackground {
    uint16_t spriteId;           // Background sprite/image
    BackgroundMode mode;         // How background behaves
    float scrollSpeedX;          // Parallax scroll speed X (0.0 = static, 1.0 = normal)
    float scrollSpeedY;          // Parallax scroll speed Y
    int16_t offsetX, offsetY;    // Current background offset
    uint8_t animationFrames;     // Number of frames for animated backgrounds
    uint8_t currentFrame;        // Current animation frame
    uint16_t frameDelayMs;       // Delay between animation frames
    uint32_t lastFrameTime;      // Last frame change time
    bool visible;                // Background visibility
    uint8_t opacity;             // Background opacity (0-255)
    
    SceneBackground() : spriteId(0), mode(BackgroundMode::STATIC), scrollSpeedX(0.0f), 
                       scrollSpeedY(0.0f), offsetX(0), offsetY(0), animationFrames(1), 
                       currentFrame(0), frameDelayMs(0), lastFrameTime(0), visible(true), 
                       opacity(255) {}
};

// === SCENE TILE ===

struct SceneTile {
    uint16_t spriteId;           // Tile sprite
    int16_t worldX, worldY;      // World position
    uint8_t width, height;       // Tile dimensions in pixels
    TileType type;               // Tile behavior type
    uint8_t collisionMask;       // What entities can collide
    bool croppingEnabled;        // Whether tile respects panel cropping
    uint8_t layer;               // Rendering layer (0-7, 0 = back, 7 = front)
    
    // Animation support
    uint8_t animationFrames;     // Number of frames (1 = static)
    uint8_t currentFrame;        // Current animation frame
    uint16_t frameDelayMs;       // Animation timing
    uint32_t lastFrameTime;      // Last frame update time
    
    // Trigger data (for TRIGGER type tiles)
    uint16_t triggerId;          // Unique trigger identifier
    uint8_t triggerMask;         // What entities trigger this tile
    
    SceneTile() : spriteId(0), worldX(0), worldY(0), width(16), height(16), 
                 type(TileType::BACKGROUND), collisionMask(0), croppingEnabled(true), 
                 layer(4), animationFrames(1), currentFrame(0), frameDelayMs(0), 
                 lastFrameTime(0), triggerId(0), triggerMask(0) {}
};

// === SCENE ENTITY ===

struct SceneEntity {
    uint16_t entityId;           // Unique entity identifier  
    uint16_t spriteId;           // Entity sprite
    int16_t worldX, worldY;      // World position
    int16_t velocityX, velocityY; // Current velocity
    uint8_t width, height;       // Entity dimensions
    EntityBehavior behavior;     // Entity behavior type
    uint8_t collisionMask;       // Collision detection mask
    bool croppingEnabled;        // Whether entity respects panel cropping
    uint8_t layer;               // Rendering layer (0-7, 0 = back, 7 = front)
    
    // Animation
    uint8_t currentFrame;        // Current sprite frame
    uint8_t animationFrames;     // Total animation frames
    uint16_t frameDelayMs;       // Animation timing
    uint32_t lastFrameTime;      // Last frame update
    bool animationLoop;          // Should animation loop
    
    // Scripting and AI
    void* scriptData;            // Pointer to script/AI data
    uint16_t scriptId;           // Script identifier for behavior
    
    // Audio
    uint16_t footstepSoundId;    // Sound for movement
    uint16_t collisionSoundId;   // Sound for collisions
    uint16_t crySoundId;         // Entity-specific sound/cry
    
    // Status
    bool active;                 // Entity is active and updating
    bool visible;                // Entity should be rendered
    uint8_t health;              // Entity health (if applicable)
    uint8_t flags;               // General-purpose flags
    
    SceneEntity() : entityId(0), spriteId(0), worldX(0), worldY(0), velocityX(0), 
                   velocityY(0), width(16), height(16), behavior(EntityBehavior::STATIC), 
                   collisionMask(0), croppingEnabled(true), layer(5), currentFrame(0), 
                   animationFrames(1), frameDelayMs(0), lastFrameTime(0), animationLoop(true), 
                   scriptData(nullptr), scriptId(0), footstepSoundId(0), collisionSoundId(0), 
                   crySoundId(0), active(true), visible(true), health(100), flags(0) {}
};

// === SCENE PANEL ===

struct ScenePanel {
    char name[32];               // Panel identifier
    
    // Panel boundaries and viewport
    int16_t worldX, worldY;      // Panel world position
    uint16_t worldWidth, worldHeight; // Panel world dimensions
    int16_t viewportX, viewportY; // Current viewport position within panel
    uint16_t viewportWidth, viewportHeight; // Viewport dimensions (screen size)
    
    // Cropping settings
    bool croppingEnabled;        // Global panel cropping (can be overridden per-sprite)
    int16_t cropLeft, cropTop;   // Cropping boundaries
    int16_t cropRight, cropBottom;
    
    // Camera focus system
    PanelFocusMode focusMode;    // How camera behaves
    uint16_t focusEntityId;      // Entity to focus on (0 = no focus)
    float focusSpeed;            // Speed of camera movement (for smooth follow)
    int16_t focusBoundLeft, focusBoundTop;     // Camera boundaries
    int16_t focusBoundRight, focusBoundBottom;
    int16_t focusOffsetX, focusOffsetY; // Offset from focused entity
    
    // Panel contents
    SceneBackground background;  // Panel background
    
    static const uint8_t MAX_ENTITIES = 64;  // Max entities per panel
    static const uint8_t MAX_TILES = 128;    // Max tiles per panel
    
    SceneEntity entities[MAX_ENTITIES];
    uint8_t entityCount;
    
    SceneTile tiles[MAX_TILES];
    uint8_t tileCount;
    
    // Panel state
    bool active;                 // Panel is actively updating
    bool visible;                // Panel should be rendered
    uint8_t opacity;             // Panel opacity (0-255)
    
    ScenePanel() : worldX(0), worldY(0), worldWidth(512), worldHeight(384), 
                  viewportX(0), viewportY(0), viewportWidth(240), viewportHeight(160),
                  croppingEnabled(true), cropLeft(0), cropTop(0), cropRight(240), 
                  cropBottom(160), focusMode(PanelFocusMode::FIXED), focusEntityId(0), 
                  focusSpeed(1.0f), focusBoundLeft(0), focusBoundTop(0), focusBoundRight(512), 
                  focusBoundBottom(384), focusOffsetX(0), focusOffsetY(0), entityCount(0), 
                  tileCount(0), active(true), visible(true), opacity(255) {
        memset(name, 0, sizeof(name));
        strcpy(name, "panel");
    }
};

// === SCENE LAYOUT ===

struct SceneLayout {
    char name[32];               // Layout identifier (level/scene name)
    
    static const uint8_t MAX_PANELS = 16;  // Max panels per layout
    ScenePanel panels[MAX_PANELS];
    uint8_t panelCount;
    uint8_t activePanelIndex;    // Currently active panel
    
    // Layout metadata
    char description[64];        // Layout description
    uint8_t difficulty;          // Difficulty level (if applicable)
    uint32_t layoutFlags;        // General-purpose flags
    
    // Layout audio
    uint16_t backgroundMusicId;  // Background music for this layout
    uint16_t ambientSoundId;     // Ambient sound effects
    float musicVolume;           // Music volume (0.0-1.0)
    float ambientVolume;         // Ambient volume (0.0-1.0)
    
    // Transition effects
    enum TransitionType : uint8_t {
        TRANSITION_NONE,         // Instant transition
        TRANSITION_FADE,         // Fade to black
        TRANSITION_SLIDE_LEFT,   // Slide left
        TRANSITION_SLIDE_RIGHT,  // Slide right
        TRANSITION_SLIDE_UP,     // Slide up
        TRANSITION_SLIDE_DOWN,   // Slide down
        TRANSITION_WIPE          // Wipe effect
    };
    TransitionType entryTransition;  // Transition when entering layout
    TransitionType exitTransition;   // Transition when leaving layout
    uint16_t transitionDurationMs;   // Transition duration
    
    SceneLayout() : panelCount(0), activePanelIndex(0), difficulty(1), layoutFlags(0), 
                   backgroundMusicId(0), ambientSoundId(0), musicVolume(0.8f), 
                   ambientVolume(0.3f), entryTransition(TRANSITION_NONE), 
                   exitTransition(TRANSITION_NONE), transitionDurationMs(500) {
        memset(name, 0, sizeof(name));
        memset(description, 0, sizeof(description));
        strcpy(name, "layout");
        strcpy(description, "Scene layout");
    }
};

// === SCENE MANAGER CLASS ===

class SceneManager {
private:
    static const uint8_t MAX_LAYOUTS = 8;
    SceneLayout layouts[MAX_LAYOUTS];
    uint8_t layoutCount;
    uint8_t currentLayoutIndex;
    
    // System references
    SpriteSystem* spriteSystem;
    EntitySystem* entitySystem; 
    AudioEngine* audioEngine;
    
    // Transition state
    bool inTransition;
    uint32_t transitionStartTime;
    SceneLayout::TransitionType currentTransition;
    uint16_t transitionDuration;
    
    // Performance tracking
    uint32_t lastUpdateTime;
    uint32_t frameCount;
    
public:
    SceneManager();
    ~SceneManager();
    
    // === INITIALIZATION ===
    bool initialize(SpriteSystem* sprites, EntitySystem* entities, AudioEngine* audio);
    void shutdown();
    
    // === LAYOUT MANAGEMENT ===
    uint8_t createLayout(const char* name, const char* description = nullptr);
    bool loadLayout(const char* name);
    bool loadLayoutFromAsset(const char* assetName);  // Load from WISP ROM
    bool setActiveLayout(uint8_t layoutIndex);
    SceneLayout* getCurrentLayout();
    SceneLayout* getLayout(uint8_t index);
    SceneLayout* findLayout(const char* name);
    
    // === PANEL MANAGEMENT ===
    uint8_t addPanel(uint8_t layoutIndex, const char* panelName);
    bool setActivePanel(uint8_t panelIndex);
    ScenePanel* getCurrentPanel();
    ScenePanel* getPanel(uint8_t layoutIndex, uint8_t panelIndex);
    ScenePanel* findPanel(uint8_t layoutIndex, const char* panelName);
    
    // === ENTITY MANAGEMENT ===
    uint16_t addEntity(uint8_t layoutIndex, uint8_t panelIndex, uint16_t spriteId, 
                      int16_t x, int16_t y, EntityBehavior behavior = EntityBehavior::STATIC);
    
    // NEW: Secure entity creation with UUID authority integration
    uint32_t addEntitySecure(uint8_t layoutIndex, uint8_t panelIndex, 
                           const String& entityType, uint16_t spriteId, 
                           int16_t x, int16_t y, const String& scriptName = "",
                           EntityBehavior behavior = EntityBehavior::STATIC);
    
    bool removeEntity(uint16_t entityId);
    SceneEntity* findEntity(uint16_t entityId);
    bool setEntityPosition(uint16_t entityId, int16_t x, int16_t y);
    bool setEntityVelocity(uint16_t entityId, int16_t vx, int16_t vy);
    bool setEntityAnimation(uint16_t entityId, uint8_t frames, uint16_t delayMs, bool loop = true);
    
    // === TILE MANAGEMENT ===
    bool addTile(uint8_t layoutIndex, uint8_t panelIndex, uint16_t spriteId, 
                int16_t x, int16_t y, TileType type = TileType::BACKGROUND);
    bool removeTile(int16_t x, int16_t y);
    SceneTile* getTileAt(int16_t x, int16_t y);
    bool setTileAnimation(int16_t x, int16_t y, uint8_t frames, uint16_t delayMs);
    
    // === LAYER MANAGEMENT ===
    bool setEntityLayer(uint16_t entityId, uint8_t layer);
    bool setTileLayer(int16_t x, int16_t y, uint8_t layer);
    void sortPanelRenderOrder(uint8_t panelIndex);  // Sort all objects by layer
    uint8_t getEntityLayer(uint16_t entityId);
    uint8_t getTileLayer(int16_t x, int16_t y);
    
    // === CAMERA/FOCUS SYSTEM ===
    bool setPanelFocus(uint8_t panelIndex, uint16_t entityId, PanelFocusMode mode);
    bool setPanelFocusBounds(uint8_t panelIndex, int16_t left, int16_t top, int16_t right, int16_t bottom);
    bool setCameraPosition(int16_t x, int16_t y);  // For manual camera control
    void getCameraPosition(int16_t* x, int16_t* y);
    
    // Enhanced focus switching - supports null entity (0) to unlock focus
    bool switchPanelFocus(uint8_t panelIndex, uint16_t newEntityId, float moveSpeed = 2.0f);
    bool clearPanelFocus(uint8_t panelIndex);  // Convenience function to unlock focus
    bool setPanelFocusSpeed(uint8_t panelIndex, float pixelsPerSecond);  // Rate in pixels/second
    uint16_t getPanelFocusTarget(uint8_t panelIndex);  // Get current focus target (0 = no focus)
    
    // === BACKGROUND SYSTEM ===
    bool setPanelBackground(uint8_t panelIndex, uint16_t spriteId, BackgroundMode mode = BackgroundMode::STATIC);
    bool setBackgroundScrollSpeed(uint8_t panelIndex, float speedX, float speedY);
    bool setBackgroundAnimation(uint8_t panelIndex, uint8_t frames, uint16_t delayMs);
    
    // === AUDIO INTEGRATION ===
    bool setLayoutMusic(uint8_t layoutIndex, uint16_t musicId, float volume = 0.8f);
    bool setLayoutAmbient(uint8_t layoutIndex, uint16_t soundId, float volume = 0.3f);
    bool playEntitySound(uint16_t entityId, uint16_t soundId);  // Play entity cry/sound
    
    // === SCENE TRANSITIONS ===
    bool transitionToLayout(uint8_t layoutIndex, SceneLayout::TransitionType transition = SceneLayout::TRANSITION_FADE);
    bool isInTransition() const { return inTransition; }
    float getTransitionProgress() const;
    
    // === CORE UPDATE AND RENDERING ===
    void update(uint32_t deltaTimeMs);
    void render();
    
    // === DEBUGGING AND STATS ===
    void printSceneStats();
    void printLayoutInfo(uint8_t layoutIndex);
    uint32_t getTotalEntityCount() const;
    uint32_t getTotalTileCount() const;
    
private:
    // Internal update methods
    void updateLayout(SceneLayout* layout, uint32_t deltaTimeMs);
    void updatePanel(ScenePanel* panel, uint32_t deltaTimeMs);
    void updateEntity(SceneEntity* entity, uint32_t deltaTimeMs);
    void updateTile(SceneTile* tile, uint32_t deltaTimeMs);
    void updateBackground(SceneBackground* background, uint32_t deltaTimeMs);
    void updateCamera(ScenePanel* panel);
    void updateTransition(uint32_t deltaTimeMs);
    
    // Rendering methods
    void renderLayout(const SceneLayout* layout);
    void renderPanel(const ScenePanel* panel);
    void renderBackground(const SceneBackground* background, const ScenePanel* panel);
    void renderTiles(const ScenePanel* panel);
    void renderEntities(const ScenePanel* panel);
    void renderTransition();
    
    // Collision and physics
    void processCollisions(ScenePanel* panel);
    bool checkTileCollision(const SceneEntity* entity, const SceneTile* tile);
    void handleTriggerActivation(SceneEntity* entity, SceneTile* tile);
    
    // Utility methods
    bool isInViewport(int16_t x, int16_t y, uint16_t width, uint16_t height, const ScenePanel* panel);
    void worldToScreen(int16_t worldX, int16_t worldY, const ScenePanel* panel, int16_t* screenX, int16_t* screenY);
    void screenToWorld(int16_t screenX, int16_t screenY, const ScenePanel* panel, int16_t* worldX, int16_t* worldY);
    
    // Memory management
    void cleanupLayout(SceneLayout* layout);
    uint16_t generateEntityId();
};

// === GLOBAL SCENE MANAGER INSTANCE ===
extern SceneManager sceneManager;

// === LAYER CONSTANTS ===
constexpr uint8_t LAYER_FAR_BACKGROUND = 0;   // Furthest background layer
constexpr uint8_t LAYER_BACKGROUND = 1;        // Main background layer  
constexpr uint8_t LAYER_BACK_TILES = 2;        // Background tiles
constexpr uint8_t LAYER_BACK_ENTITIES = 3;     // Background entities
constexpr uint8_t LAYER_MAIN_TILES = 4;        // Main gameplay tiles (default)
constexpr uint8_t LAYER_MAIN_ENTITIES = 5;     // Main gameplay entities (default)
constexpr uint8_t LAYER_FRONT_TILES = 6;       // Foreground tiles
constexpr uint8_t LAYER_FRONT_ENTITIES = 7;    // Foreground entities/effects

// Depth constants within layers
constexpr uint8_t DEPTH_BACK = 0;              // Back of layer
constexpr uint8_t DEPTH_MID = 128;             // Middle of layer (default)
constexpr uint8_t DEPTH_FRONT = 255;           // Front of layer

// === CONVENIENCE MACROS ===
#define SCENE_CURRENT_LAYOUT() (sceneManager.getCurrentLayout())
#define SCENE_CURRENT_PANEL() (sceneManager.getCurrentPanel())

// Entity management macros
#define SCENE_ADD_ENTITY(layoutIdx, panelIdx, sprite, x, y, behavior) \
    (sceneManager.addEntity(layoutIdx, panelIdx, sprite, x, y, behavior))
#define SCENE_SET_ENTITY_LAYER(entityId, layer) \
    (sceneManager.setEntityLayer(entityId, layer))
#define SCENE_MOVE_ENTITY(entityId, x, y) \
    (sceneManager.setEntityPosition(entityId, x, y))

// Tile management macros
#define SCENE_ADD_TILE(layoutIdx, panelIdx, sprite, x, y, type) \
    (sceneManager.addTile(layoutIdx, panelIdx, sprite, x, y, type))
#define SCENE_SET_TILE_LAYER(x, y, layer) \
    (sceneManager.setTileLayer(x, y, layer))

// Camera/Focus macros
#define SCENE_FOCUS_ON_ENTITY(panelIdx, entityId, speed) \
    (sceneManager.switchPanelFocus(panelIdx, entityId, speed))
#define SCENE_UNLOCK_FOCUS(panelIdx) \
    (sceneManager.clearPanelFocus(panelIdx))
#define SCENE_SET_CAMERA_SPEED(panelIdx, speed) \
    (sceneManager.setPanelFocusSpeed(panelIdx, speed))

} // namespace WispEngine
