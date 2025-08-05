// engine/curated_api.h
#pragma once
#include "../../system/esp32_common.h"
#include "api_limits.h"
#include "../graphics/engine.h"
#include "../core/resource_manager.h"
#include "../database/save_system.h"
#include <string>

// Forward declarations
namespace WispEngine { class Engine; }
class AudioEngine;
class InputController;

// Curated API that apps are allowed to use
// This is the ONLY interface apps should access - no direct engine access

// Safe entity handle - apps can't access entities directly
typedef uint16_t EntityHandle;
#define INVALID_ENTITY 0xFFFF

// Safe resource handle - prevents direct memory access
typedef uint16_t ResourceHandle;
#define INVALID_RESOURCE 0xFFFF

// Safe timer handle
typedef uint16_t TimerHandle;
#define INVALID_TIMER 0xFFFF

// Input state structure (read-only for apps)
struct WispInputState {
    bool left, right, up, down;
    bool buttonA, buttonB, buttonC;
    bool select, start;
    int16_t analogX, analogY;  // -100 to +100 range
    
    // Touch input (if available)
    bool touched;
    int16_t touchX, touchY;
    
    WispInputState() : left(false), right(false), up(false), down(false),
                       buttonA(false), buttonB(false), buttonC(false),
                       select(false), start(false), analogX(0), analogY(0),
                       touched(false), touchX(0), touchY(0) {}
};

// Vector2 for positions and sizes
struct WispVec2 {
    float x, y;
    WispVec2() : x(0), y(0) {}
    WispVec2(float _x, float _y) : x(_x), y(_y) {}
};

// Color structure
struct WispColor {
    uint8_t r, g, b, a;
    WispColor() : r(255), g(255), b(255), a(255) {}
    WispColor(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a = 255) : r(_r), g(_g), b(_b), a(_a) {}
    
    // Convert to RGB565
    uint16_t toRGB565() const {
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }
};

// Audio playback parameters
struct WispAudioParams {
    float volume;     // 0.0 to 1.0
    float pitch;      // 0.5 to 2.0 (default 1.0)
    bool loop;        // Should audio loop
    uint8_t priority; // 0=highest, 255=lowest
    
    WispAudioParams() : volume(1.0f), pitch(1.0f), loop(false), priority(128) {}
};

// Animation parameters
struct WispAnimParams {
    uint8_t startFrame;
    uint8_t endFrame;
    uint16_t frameTime;  // milliseconds per frame
    bool loop;
    bool pingpong;       // reverse direction at end
    
    WispAnimParams() : startFrame(0), endFrame(0), frameTime(100), loop(true), pingpong(false) {}
};

// Collision detection result
struct WispCollision {
    bool hit;
    EntityHandle entity;
    WispVec2 point;
    WispVec2 normal;
    
    WispCollision() : hit(false), entity(INVALID_ENTITY) {}
};

// Particle emission parameters
struct WispParticleParams {
    WispVec2 position;
    WispVec2 velocity;
    WispVec2 acceleration;
    WispColor startColor;
    WispColor endColor;
    float startSize;
    float endSize;
    uint16_t lifetime;  // milliseconds
    
    WispParticleParams() : startSize(1.0f), endSize(0.0f), lifetime(1000) {}
};

// The curated API class - this is what apps get access to
class WispCuratedAPI {
private:
    WispEngine::Engine* engine;
    WispResourceQuota quota;
    
    // Performance monitoring
    uint32_t frameStartTime;
    uint32_t updateStartTime;
    uint32_t renderStartTime;
    
    // Error tracking
    uint16_t errorsThisSecond;
    uint32_t lastErrorReset;
    
    // Safety flags
    bool emergencyMode;
    bool quotaViolated;
    
    // App timing and lifecycle
    uint32_t startTime;
    uint32_t deltaTime;
    
    // App permissions structure
    struct {
        bool canLaunchApps;      // Can launch other apps
        bool canAccessNetwork;   // Can use WiFi/Bluetooth
        bool canAccessStorage;   // Can access SD card directly
        bool canModifySystem;    // Can change system settings
    } appPermissions;
    
public:
    WispCuratedAPI(WispEngine::Engine* eng);
    
    // === CORE LIFECYCLE ===
    // Called by engine - apps should not call these
    bool beginFrame();
    void endFrame();
    void beginUpdate();
    void endUpdate();
    void beginRender();
    void endRender();
    
    // === INPUT API ===
    const WispInputState& getInput() const;
    bool isKeyPressed(uint8_t key);
    bool isKeyJustPressed(uint8_t key);
    bool isKeyJustReleased(uint8_t key);
    
    // === GRAPHICS API ===
    // Resource management
    ResourceHandle loadSprite(const std::string& filePath);
    void unloadSprite(ResourceHandle handle);
    bool isSpriteLoaded(ResourceHandle handle);
    
    // Drawing functions (all quota-limited)
    bool drawSprite(ResourceHandle sprite, float x, float y, uint8_t depth = 5);
    bool drawSpriteFrame(ResourceHandle sprite, float x, float y, uint8_t frame, uint8_t depth = 5);
    bool drawSpriteScaled(ResourceHandle sprite, float x, float y, float scaleX, float scaleY, uint8_t depth = 5);
    bool drawSpriteRotated(ResourceHandle sprite, float x, float y, float angle, uint8_t depth = 5);
    
    // Primitive drawing
    bool drawRect(float x, float y, float width, float height, WispColor color, uint8_t depth = 5);
    bool drawCircle(float x, float y, float radius, WispColor color, uint8_t depth = 5);
    bool drawLine(float x1, float y1, float x2, float y2, WispColor color, uint8_t depth = 5);
    bool drawText(const std::string& text, float x, float y, WispColor color, uint8_t depth = 5);
    
    // Screen/camera management
    void setCameraPosition(float x, float y);
    WispVec2 getCameraPosition() const;
    WispVec2 getScreenSize() const;
    WispVec2 worldToScreen(WispVec2 worldPos) const;
    WispVec2 screenToWorld(WispVec2 screenPos) const;
    
    // === AUDIO API ===
    ResourceHandle loadAudio(const std::string& filePath);
    void unloadAudio(ResourceHandle handle);
    bool playAudio(ResourceHandle audio, const WispAudioParams& params = WispAudioParams());
    void stopAudio(ResourceHandle audio);
    void setMasterVolume(float volume);
    
    // === ENTITY SYSTEM ===
    EntityHandle createEntity();
    void destroyEntity(EntityHandle entity);
    bool isEntityValid(EntityHandle entity);
    
    // Entity properties
    void setEntityPosition(EntityHandle entity, WispVec2 position);
    WispVec2 getEntityPosition(EntityHandle entity);
    void setEntityVelocity(EntityHandle entity, WispVec2 velocity);
    WispVec2 getEntityVelocity(EntityHandle entity);
    void setEntitySprite(EntityHandle entity, ResourceHandle sprite);
    void setEntityAnimation(EntityHandle entity, const WispAnimParams& anim);
    
    // === COLLISION SYSTEM ===
    WispCollision checkCollision(EntityHandle entity1, EntityHandle entity2);
    int getEntitiesInRadius(WispVec2 center, float radius, EntityHandle* entities, int maxEntities);
    int getEntitiesInRect(float x, float y, float width, float height, EntityHandle* entities, int maxEntities);
    bool isPointInEntity(WispVec2 point, EntityHandle entity);
    
    // === PARTICLE SYSTEM ===
    bool createParticle(const WispParticleParams& params);
    void createParticleBurst(WispVec2 center, uint16_t count, const WispParticleParams& template_params);
    void clearAllParticles();
    
    // === TIMER SYSTEM ===
    TimerHandle createTimer(uint32_t intervalMs, bool repeating = false);
    void destroyTimer(TimerHandle timer);
    bool isTimerFinished(TimerHandle timer);
    void resetTimer(TimerHandle timer);
    uint32_t getTimerRemaining(TimerHandle timer);
    
    // === APP MANAGEMENT SYSTEM ===
    // App discovery and management (for launcher/menu systems)
    int getAvailableApps(char appNames[][WISP_MAX_STRING_LENGTH], int maxApps); // Get list of available app names
    bool getAppDescription(const char* appName, char* description, int maxLen); // Get app description
    bool getAppAuthor(const char* appName, char* author, int maxLen);           // Get app author
    bool getAppVersion(const char* appName, char* version, int maxLen);         // Get app version
    bool isAppCompatible(const char* appName);                                 // Check if app is compatible
    
    // App launching (restricted - requires special permissions)
    bool requestAppLaunch(const char* appName);                // Request to launch an app
    bool canLaunchApps() const;                                 // Check if this app can launch others
    
    // Permission management (system use only)
    void setAppPermissions(bool canLaunch, bool canNetwork, bool canStorage, bool canSystem);
    
    // === UTILITY FUNCTIONS ===
    uint32_t getTime() const;           // Milliseconds since app start
    uint32_t getDeltaTime() const;      // Milliseconds since last frame
    float random(float min = 0.0f, float max = 1.0f);
    int randomInt(int min, int max);
    
    // Math utilities
    float lerp(float a, float b, float t);
    float distance(WispVec2 a, WispVec2 b);
    float angle(WispVec2 from, WispVec2 to);
    WispVec2 normalize(WispVec2 vec);
    
    // === SAVE SYSTEM API ===
    // App identity management
    bool setAppIdentity(const std::string& uuid, const std::string& version, uint32_t saveFormatVersion = 1);
    
    // Save field registration (must be done during app initialization)
    bool registerSaveField(const std::string& key, bool* value);
    bool registerSaveField(const std::string& key, int8_t* value);
    bool registerSaveField(const std::string& key, uint8_t* value);
    bool registerSaveField(const std::string& key, int16_t* value);
    bool registerSaveField(const std::string& key, uint16_t* value);
    bool registerSaveField(const std::string& key, int32_t* value);
    bool registerSaveField(const std::string& key, uint32_t* value);
    bool registerSaveField(const std::string& key, float* value);
    bool registerSaveField(const std::string& key, std::string* value, size_t maxLength = 256);
    bool registerSaveBlob(const std::string& key, void* data, size_t size);
    
    // Save field access (null-safe)
    template<typename T> T* getSaveField(const std::string& key);
    std::string* getSaveString(const std::string& key);
    void* getSaveBlob(const std::string& key, size_t* outSize = nullptr);
    
    // Save field modification (marks dirty for auto-save)
    template<typename T> bool setSaveField(const std::string& key, const T& value);
    bool setSaveString(const std::string& key, const std::string& value);
    bool setSaveBlob(const std::string& key, const void* data, size_t size);
    
    // Save/load operations
    bool save();
    bool load();
    bool resetSaveData();
    
    // Save file management
    bool hasSaveFile() const;
    bool deleteSaveFile();
    void enableAutoSave(bool enabled, uint32_t intervalMs = 30000);
    
    // Save system status
    bool isSaveSystemReady() const;
    uint64_t getSaveTimestamp() const;
    size_t getSaveFileSize() const;
    
    // === DEBUG API ===
    void print(const std::string& message);
    void printWarning(const std::string& message);
    void printError(const std::string& message);
    
    // === QUOTA MONITORING ===
    const WispResourceQuota& getQuota() const { return quota; }
    bool isQuotaViolated() const { return quotaViolated; }
    bool isInEmergencyMode() const { return emergencyMode; }
    float getPerformanceRating() const; // 0.0 = terrible, 1.0 = perfect
    
    // === RESTRICTED FUNCTIONS ===
    // These functions require special permission or are limited
    bool requestNetworkAccess();       // Must be explicitly granted
    bool requestFileWrite();           // Must be explicitly granted
    void yield();                      // Cooperative multitasking
    
private:
    // Internal quota enforcement
    bool checkDrawQuota();
    bool checkMemoryQuota(uint32_t bytes);
    bool checkEntityQuota();
    bool checkAudioQuota();
    bool checkParticleQuota();
    
    // Error handling
    void recordError(const String& error);
    void checkEmergencyMode();
    
    // Performance monitoring
    void checkPerformanceLimits();
    void enforceFrameTimeLimit();
    
    // Safety checks
    bool validateEntityHandle(EntityHandle entity);
    bool validateResourceHandle(ResourceHandle resource);
    bool validateTimerHandle(TimerHandle timer);
    
    // Internal helpers
    void resetFrameCounters();
    void updateQuotaUsage();
};

// Implementation of key methods
inline bool WispCuratedAPI::drawSprite(ResourceHandle sprite, float x, float y, uint8_t depth) {
    if (!checkDrawQuota()) {
        recordError("Draw quota exceeded");
        return false;
    }
    
    if (!validateResourceHandle(sprite)) {
        recordError("Invalid sprite handle");
        return false;
    }
    
    quota.draw();
    
    // Forward to actual graphics engine through engine
    // TODO: Implement actual drawing
    return true;
}

inline EntityHandle WispCuratedAPI::createEntity() {
    if (!checkEntityQuota()) {
        recordError("Entity quota exceeded");
        return INVALID_ENTITY;
    }
    
    quota.allocateEntity();
    
    // TODO: Create actual entity through engine
    static uint16_t nextEntityId = 1;
    return nextEntityId++;
}

inline bool WispCuratedAPI::checkDrawQuota() {
    return quota.canDraw();
}

inline bool WispCuratedAPI::checkEntityQuota() {
    return quota.canAllocateEntity();
}

inline void WispCuratedAPI::recordError(const String& error) {
    uint32_t currentTime = get_millis();
    
    // Reset error counter every second
    if (currentTime - lastErrorReset > 1000) {
        errorsThisSecond = 0;
        lastErrorReset = currentTime;
    }
    
    errorsThisSecond++;
    
    Serial.print("WISP API ERROR: ");
    Serial.println(error.c_str());  // Use c_str() to convert to const char*
    
    // Check if too many errors
    if (errorsThisSecond > WISP_MAX_ERRORS_PER_SECOND) {
        emergencyMode = true;
        Serial.println("EMERGENCY MODE: Too many API errors");
    }
}

inline void WispCuratedAPI::resetFrameCounters() {
    quota.resetFrameCounters();
}

inline bool WispCuratedAPI::beginFrame() {
    frameStartTime = get_micros();
    resetFrameCounters();
    
    // Check if still in emergency mode
    if (emergencyMode) {
        return false; // Don't allow frame processing
    }
    
    return true;
}

inline void WispCuratedAPI::endFrame() {
    uint32_t frameTime = get_micros() - frameStartTime;
    
    if (frameTime > WISP_MAX_FRAME_TIME_US) {
        recordError("Frame time exceeded limit");
    }
    
    // Update quota usage for next frame
    updateQuotaUsage();
}

// Macro for easy API access in apps
#define WISP_API WispCuratedAPI::getInstance()
