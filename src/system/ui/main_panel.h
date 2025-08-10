#pragma once

#include "panels/menu.h"
#include "../../engine/security/script_instance_authority.h"
#include "../../engine/security/named_entity_registry.h"
#include "esp_log.h"

static const char* MAIN_PANEL_TAG = "MainPanel";

/**
 * @brief Main Bootloader Panel with Global Script Support
 * 
 * This is the primary panel used by the bootloader for fallback menu functionality.
 * It supports global scripts that can control system-wide behavior and UI elements.
 * 
 * Key Features:
 * - Global script execution with system-level permissions
 * - Named entity control for UI elements
 * - Input event dispatching to global scripts
 * - System state management through scripts
 * - Graceful fallback when no ROM is loaded
 */
class MainPanel : public MenuPanel {
private:
    // Global script management
    std::string globalScriptName;
    bool globalScriptActive;
    bool globalScriptInitialized;
    
    // System state
    enum class SystemState : uint8_t {
        BOOTING = 0,
        MENU_ACTIVE = 1,
        APP_LOADING = 2,
        APP_RUNNING = 3,
        ERROR_STATE = 4
    };
    SystemState currentState;
    std::string statusMessage;
    
    // Performance tracking
    uint32_t frameCount;
    uint32_t lastFPSUpdate;
    uint16_t currentFPS;
    
    // UI elements (managed by named entities)
    bool uiElementsRegistered;

public:
    MainPanel();
    virtual ~MainPanel();
    
    // === GLOBAL SCRIPT MANAGEMENT ===
    
    /**
     * @brief Initialize global script with system permissions
     * @param scriptName Name of the global script to load
     * @return True if global script was initialized successfully
     */
    bool initializeGlobalScript(const std::string& scriptName);
    
    /**
     * @brief Shutdown global script
     */
    void shutdownGlobalScript();
    
    /**
     * @brief Check if global script is active
     * @return True if global script is running
     */
    bool isGlobalScriptActive() const { return globalScriptActive; }
    
    /**
     * @brief Get name of active global script
     * @return Global script name or empty string if none
     */
    const std::string& getGlobalScriptName() const { return globalScriptName; }
    
    // === PANEL IMPLEMENTATION ===
    
    /**
     * @brief Panel-specific update implementation
     */
    void updatePanel(const WispInputState& input) override;
    
    /**
     * @brief Render the main panel
     */
    void render() override;
    
    /**
     * @brief Initialize main panel
     */
    bool init() override;
    
    /**
     * @brief Cleanup main panel
     */
    void cleanup() override;
    
    // === SYSTEM STATE MANAGEMENT ===
    
    /**
     * @brief Set system state (accessible to global scripts)
     * @param state New system state
     * @param message Optional status message
     */
    void setSystemState(SystemState state, const std::string& message = "");
    
    /**
     * @brief Get current system state
     * @return Current system state
     */
    SystemState getSystemState() const { return currentState; }
    
    /**
     * @brief Get current status message
     * @return Status message string
     */
    const std::string& getStatusMessage() const { return statusMessage; }
    
    /**
     * @brief Update FPS counter
     */
    void updateFPS();
    
    /**
     * @brief Get current FPS
     * @return FPS value
     */
    uint16_t getFPS() const { return currentFPS; }
    
    // === EVENT CALLBACKS ===
    
    void onActivate() override;
    void onDeactivate() override;
    void onScriptAttached(const std::string& scriptName) override;
    void onScriptDetached(const std::string& scriptName) override;
    void onScriptError(const std::string& error) override;
    
    // === UI ELEMENT MANAGEMENT ===
    
    /**
     * @brief Register UI elements as named entities
     * This allows global scripts to control main panel UI elements
     */
    void registerUIElements();
    
    /**
     * @brief Unregister UI elements from named entity registry
     */
    void unregisterUIElements();
    
    /**
     * @brief Update UI element states based on system state
     */
    void updateUIElements();
    
    // === INPUT HANDLING ===
    
    /**
     * @brief Handle main menu input
     * @param input Input state
     */
    void handleMainMenuInput(const WispInputState& input);
    
    /**
     * @brief Dispatch input events to global script
     * @param input Input semantic
     * @param pressed True if pressed, false if released
     */
    void dispatchInputToScript(WispInputSemantic input, bool pressed);
    
    // === DEBUGGING AND STATS ===
    
    /**
     * @brief Get main panel statistics
     */
    struct MainPanelStats {
        SystemState state;
        bool globalScriptActive;
        uint32_t frameCount;
        uint16_t fps;
        uint32_t scriptExecutionCount;
        uint8_t scriptErrorCount;
        uint32_t totalUptime;
    };
    
    MainPanelStats getStats() const;
    
    /**
     * @brief Dump main panel state for debugging
     */
    void dumpState() const;

protected:
    /**
     * @brief Execute global script with system context
     */
    void executeGlobalScript();
    
    /**
     * @brief Handle global script initialization
     */
    bool initializeGlobalScriptInternal();
    
    /**
     * @brief Render system status information
     */
    void renderSystemStatus();
    
    /**
     * @brief Render main menu options
     */
    void renderMainMenu();
    
    /**
     * @brief Render FPS and debug info
     */
    void renderDebugInfo();
    
private:
    // Internal helpers
    const char* stateToString(SystemState state) const;
    void logStateChange(SystemState oldState, SystemState newState);
    uint32_t getUptimeMs() const;
};

/**
 * @brief Example Global Script Functions
 * 
 * Global scripts attached to MainPanel can implement these functions:
 * 
 * function onSystemBoot() {
 *     // Called when system finishes booting
 *     show("boot_complete_message");
 *     enableGroup("main_menu_options");
 * }
 * 
 * function onUpdate() {
 *     // Called every frame
 *     var uptime = getSystemUptime();
 *     setMetadata("uptime_display", "{\"uptime\": " + uptime + "}");
 * }
 * 
 * function onInputPressed(input) {
 *     if (input == INPUT_MENU) {
 *         // Toggle debug info
 *         if (exists("debug_panel")) {
 *             if (getState("debug_panel") == STATE_ACTIVE) {
 *                 hide("debug_panel");
 *             } else {
 *                 show("debug_panel");
 *             }
 *         }
 *     }
 * }
 * 
 * function onAppLoad() {
 *     // Called when ROM is being loaded
 *     hide("main_menu_options");
 *     show("loading_spinner");
 *     setAnimation("loading_spinner", "spin");
 * }
 * 
 * function onAppUnload() {
 *     // Called when returning to main menu
 *     show("main_menu_options");
 *     hide("loading_spinner");
 * }
 */
