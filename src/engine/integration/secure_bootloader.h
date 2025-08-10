#pragma once

#include "../security/engine_uuid_authority.h"
#include "../security/script_instance_authority.h" 
#include "../security/secure_wash_api_bridge.h"
#include "../security/secure_rom_loader.h"
#include "../security/named_entity_registry.h"
#include "../events/scene_event_dispatcher.h"
#include "../scene/scene_system.h"
#include "../app/curated_api_extended.h"
#include "../../system/ui/main_panel.h"
#include "../../system/esp32_common.h"
#include "../../system/definitions.h"
#include <memory>
#include "esp_log.h"
#include "esp_timer.h"

static const char* SECURE_BOOTLOADER_TAG = "SecureBootloader";

/**
 * @brief Enhanced Bootloader with Complete Security Integration
 * 
 * Provides a comprehensive bootloader that integrates all security systems:
 * - UUID Authority for entity management
 * - Script Instance Authority for script lifecycle
 * - Secure API Bridge for script-engine communication
 * - Named Entity Registry for efficient entity operations
 * - Scene Event Dispatcher for coordinated event handling
 * - MainPanel with global script support
 * 
 * This bootloader maintains backward compatibility while adding robust
 * security and scripting capabilities to the Wisp Engine.
 */
class SecureBootloader {
public:
    /**
     * @brief Bootloader phases with enhanced security integration
     */
    enum class BootPhase : uint8_t {
        INIT = 0,
        SECURITY_SETUP = 1,
        SYSTEMS_INIT = 2,
        APP_LOADING = 3,
        APP_RUNNING = 4,
        MENU_FALLBACK = 5,
        SHUTDOWN = 6,
        ERROR_STATE = 7
    };

private:
    // === CORE AUTHORITY SYSTEMS ===
    std::unique_ptr<EngineUUIDAuthority> uuidAuthority;
    std::unique_ptr<SecureWASHAPIBridge> secureApiBridge;
    std::unique_ptr<ScriptInstanceAuthority> scriptAuthority;
    std::unique_ptr<SecureROMLoader> romLoader;  // Phase 5: Secure ROM Loading
    std::unique_ptr<NamedEntityRegistry> namedRegistry;
    std::unique_ptr<SceneEventDispatcher> eventDispatcher;
    
    // === ENGINE SYSTEMS ===
    std::unique_ptr<WispCuratedAPIExtended> curatedAPI;
    std::unique_ptr<SceneManager> sceneManager;
    std::unique_ptr<MainPanel> mainPanel;
    
    // === STATE MANAGEMENT ===
    BootPhase currentPhase;
    BootPhase previousPhase;
    uint32_t phaseStartTime;
    std::string statusMessage;
    bool systemsInitialized;
    bool securityEnabled;
    
    // === PERFORMANCE TRACKING ===
    uint32_t frameCount;
    uint32_t lastFPSUpdate;
    uint16_t currentFPS;
    uint32_t bootStartTime;
    uint32_t totalUptime;
    
    // === CONFIGURATION ===
    bool enableLegacyMode;          // Backward compatibility mode
    bool enableDebugLogging;        // Enhanced debugging
    std::string globalScriptName;   // Global script to load
    uint32_t maxFrameTimeMicros;    // Frame time budget

public:
    SecureBootloader();
    virtual ~SecureBootloader();
    
    // === INITIALIZATION ===
    
    /**
     * @brief Initialize the secure bootloader
     * @param enableLegacy True to enable legacy compatibility mode
     * @param globalScript Name of global script to load (optional)
     * @return True if initialization successful
     */
    bool initialize(bool enableLegacy = true, const std::string& globalScript = "");
    
    /**
     * @brief Shutdown the bootloader and cleanup all systems
     */
    void shutdown();
    
    // === MAIN LOOP ===
    
    /**
     * @brief Main bootloader update loop
     * Call this once per frame to update all systems
     */
    void update();
    
    /**
     * @brief Render all systems
     * Call this once per frame after update()
     */
    void render();
    
    /**
     * @brief Process input events
     * @param inputState Current input state
     */
    void processInput(const WispInputState& inputState);
    
    // === PHASE MANAGEMENT ===
    
    /**
     * @brief Get current bootloader phase
     * @return Current phase
     */
    BootPhase getCurrentPhase() const { return currentPhase; }
    
    /**
     * @brief Get current status message
     * @return Status message string
     */
    const std::string& getStatusMessage() const { return statusMessage; }
    
    /**
     * @brief Check if systems are fully initialized
     * @return True if all systems ready
     */
    bool isInitialized() const { return systemsInitialized; }
    
    /**
     * @brief Check if security systems are enabled
     * @return True if security enabled
     */
    bool isSecurityEnabled() const { return securityEnabled; }
    
    // === SYSTEM ACCESS ===
    
    /**
     * @brief Get UUID authority system
     * @return Pointer to UUID authority (may be null)
     */
    EngineUUIDAuthority* getUUIDAuthority() const { return uuidAuthority.get(); }
    
    /**
     * @brief Get script authority system
     * @return Pointer to script authority (may be null)
     */
    ScriptInstanceAuthority* getScriptAuthority() const { return scriptAuthority.get(); }
    
    /**
     * @brief Get named entity registry
     * @return Pointer to named registry (may be null)
     */
    NamedEntityRegistry* getNamedRegistry() const { return namedRegistry.get(); }
    
    /**
     * @brief Get event dispatcher
     * @return Pointer to event dispatcher (may be null)
     */
    SceneEventDispatcher* getEventDispatcher() const { return eventDispatcher.get(); }
    
    /**
     * @brief Get scene manager
     * @return Pointer to scene manager (may be null)
     */
    SceneManager* getSceneManager() const { return sceneManager.get(); }
    
    /**
     * @brief Get main panel
     * @return Pointer to main panel (may be null)
     */
    MainPanel* getMainPanel() const { return mainPanel.get(); }
    
    /**
     * @brief Get ROM loader system (Phase 5)
     * @return Pointer to ROM loader (may be null)
     */
    SecureROMLoader* getROMLoader() const { return romLoader.get(); }
    
    // === APP MANAGEMENT ===
    
    /**
     * @brief Load and start an application ROM
     * @param romPath Path to the ROM file
     * @return True if app loaded successfully
     */
    bool loadApp(const std::string& romPath);
    
    /**
     * @brief Unload current application and return to menu
     */
    void unloadApp();
    
    /**
     * @brief Check if an app is currently running
     * @return True if app is running
     */
    bool isAppRunning() const;
    
    // === CONFIGURATION ===
    
    /**
     * @brief Enable or disable legacy compatibility mode
     * @param enabled True to enable legacy mode
     */
    void setLegacyMode(bool enabled);
    
    /**
     * @brief Enable or disable debug logging
     * @param enabled True to enable debug logging
     */
    void setDebugLogging(bool enabled);
    
    /**
     * @brief Set frame time budget in microseconds
     * @param micros Maximum frame time (default: 16666 μs = 60 FPS)
     */
    void setFrameTimeBudget(uint32_t micros);
    
    // === DEBUGGING AND STATISTICS ===
    
    /**
     * @brief Get comprehensive system statistics
     */
    struct SystemStats {
        // Boot info
        BootPhase currentPhase;
        uint32_t totalUptime;
        uint32_t bootTime;
        
        // Performance
        uint16_t fps;
        uint32_t frameCount;
        uint32_t lastFrameTime;
        
        // Security systems
        uint32_t totalUUIDs;
        uint16_t activeScripts;
        uint16_t quarantinedScripts;
        uint32_t totalEvents;
        uint32_t droppedEvents;
        
        // Memory usage (if available)
        uint32_t freeHeapSize;
        uint32_t minFreeHeapSize;
    };
    
    SystemStats getSystemStats() const;
    
    /**
     * @brief Dump complete system state for debugging
     */
    void dumpSystemState() const;
    
    /**
     * @brief Get current FPS
     * @return FPS value
     */
    uint16_t getFPS() const { return currentFPS; }
    
    /**
     * @brief Get total uptime in milliseconds
     * @return Uptime in milliseconds
     */
    uint32_t getUptime() const;

private:
    // === INITIALIZATION PHASES ===
    
    /**
     * @brief Phase 0: Basic initialization
     * @return True if successful
     */
    bool initializeBasicSystems();
    
    /**
     * @brief Phase 1: Security system setup
     * @return True if successful
     */
    bool initializeSecuritySystems();
    
    /**
     * @brief Phase 2: Engine systems initialization
     * @return True if successful
     */
    bool initializeEngineSystems();
    
    /**
     * @brief Phase 3: UI and script integration
     * @return True if successful
     */
    bool initializeUIAndScripts();
    
    // === UPDATE PHASES ===
    
    /**
     * @brief Update during app running phase
     */
    void updateAppRunning();
    
    /**
     * @brief Update during menu fallback phase
     */
    void updateMenuFallback();
    
    /**
     * @brief Update during app loading phase
     */
    void updateAppLoading();
    
    /**
     * @brief Update during error state
     */
    void updateErrorState();
    
    // === PHASE MANAGEMENT ===
    
    /**
     * @brief Change to new bootloader phase
     * @param newPhase Phase to change to
     * @param message Status message for the phase
     */
    void changePhase(BootPhase newPhase, const std::string& message = "");
    
    /**
     * @brief Get phase name as string
     * @param phase Phase to get name for
     * @return Phase name
     */
    const char* getPhaseString(BootPhase phase) const;
    
    // === ERROR HANDLING ===
    
    /**
     * @brief Handle system error and enter error state
     * @param error Error message
     */
    void handleSystemError(const std::string& error);
    
    /**
     * @brief Handle script error
     * @param scriptName Script that encountered error
     * @param error Error message
     */
    void handleScriptError(const std::string& scriptName, const std::string& error);
    
    /**
     * @brief Handle security violation
     * @param violationType Type of violation
     * @param details Violation details
     */
    void handleSecurityViolation(const std::string& violationType, const std::string& details);
    
    // === PERFORMANCE MONITORING ===
    
    /**
     * @brief Update FPS counter
     */
    void updateFPS();
    
    /**
     * @brief Check frame time budget and warn if exceeded
     * @param frameStartTime Start time of current frame
     */
    void checkFrameTimeBudget(uint32_t frameStartTime);
    
    /**
     * @brief Get current time in milliseconds
     * @return Current time
     */
    uint32_t getCurrentTimeMs() const;
    
    /**
     * @brief Get current time in microseconds
     * @return Current time
     */
    uint32_t getCurrentTimeMicros() const;
};
