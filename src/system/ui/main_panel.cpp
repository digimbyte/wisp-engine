#include "main_panel.h"
#include "esp_timer.h"
#include <algorithm>

MainPanel::MainPanel() 
    : MenuPanel("Main Menu"), globalScriptActive(false), globalScriptInitialized(false),
      currentState(SystemState::BOOTING), frameCount(0), lastFPSUpdate(0), currentFPS(0),
      uiElementsRegistered(false) {
    
    ESP_LOGI(MAIN_PANEL_TAG, "MainPanel created with panel ID %d", getPanelId());
    setSystemState(SystemState::BOOTING, "System initializing...");
}

MainPanel::~MainPanel() {
    ESP_LOGI(MAIN_PANEL_TAG, "MainPanel destructor called");
    cleanup();
}

bool MainPanel::init() {
    ESP_LOGI(MAIN_PANEL_TAG, "Initializing MainPanel");
    
    if (!MenuPanel::init()) {
        ESP_LOGE(MAIN_PANEL_TAG, "Failed to initialize base MenuPanel");
        return false;
    }
    
    // Register UI elements as named entities
    registerUIElements();
    
    // Set initial system state
    setSystemState(SystemState::MENU_ACTIVE, "Main menu ready");
    
    ESP_LOGI(MAIN_PANEL_TAG, "MainPanel initialization complete");
    return true;
}

void MainPanel::cleanup() {
    ESP_LOGI(MAIN_PANEL_TAG, "Cleaning up MainPanel");
    
    // Shutdown global script
    shutdownGlobalScript();
    
    // Unregister UI elements
    unregisterUIElements();
    
    // Call base cleanup
    MenuPanel::cleanup();
    
    ESP_LOGI(MAIN_PANEL_TAG, "MainPanel cleanup complete");
}

bool MainPanel::initializeGlobalScript(const std::string& scriptName) {
    if (!scriptAuthority) {
        ESP_LOGW(MAIN_PANEL_TAG, "Cannot initialize global script: no script authority");
        return false;
    }
    
    // Shutdown existing global script if any
    if (globalScriptActive) {
        shutdownGlobalScript();
    }
    
    ESP_LOGI(MAIN_PANEL_TAG, "Initializing global script: %s", scriptName.c_str());
    
    // Create global script with system permissions
    if (scriptAuthority->createGlobalScript(scriptName, ScriptInstanceAuthority::PermissionLevel::SYSTEM)) {
        globalScriptName = scriptName;
        globalScriptActive = true;
        globalScriptInitialized = false;
        
        ESP_LOGI(MAIN_PANEL_TAG, "Global script '%s' created successfully", scriptName.c_str());
        
        // Call script initialization
        if (initializeGlobalScriptInternal()) {
            globalScriptInitialized = true;
            ESP_LOGI(MAIN_PANEL_TAG, "Global script '%s' initialized successfully", scriptName.c_str());
            return true;
        } else {
            ESP_LOGW(MAIN_PANEL_TAG, "Global script '%s' failed initialization", scriptName.c_str());
            // Keep script active but mark as not initialized
            return true;
        }
    }
    
    ESP_LOGE(MAIN_PANEL_TAG, "Failed to create global script: %s", scriptName.c_str());
    return false;
}

void MainPanel::shutdownGlobalScript() {
    if (globalScriptActive && scriptAuthority) {
        ESP_LOGI(MAIN_PANEL_TAG, "Shutting down global script: %s", globalScriptName.c_str());
        
        scriptAuthority->destroyGlobalScript(globalScriptName);
        globalScriptName.clear();
        globalScriptActive = false;
        globalScriptInitialized = false;
        
        ESP_LOGI(MAIN_PANEL_TAG, "Global script shutdown complete");
    }
}

void MainPanel::updatePanel(const WispInputState& input) {
    // Update frame counter and FPS
    frameCount++;
    updateFPS();
    
    // Execute global script if active
    if (globalScriptActive && globalScriptInitialized) {
        executeGlobalScript();
    }
    
    // Handle main menu input
    handleMainMenuInput(input);
    
    // Update UI elements based on system state
    updateUIElements();
}

void MainPanel::render() {
    if (!active) return;
    
    // Render system status
    renderSystemStatus();
    
    // Render main menu options
    renderMainMenu();
    
    // Render debug info
    renderDebugInfo();
}

void MainPanel::setSystemState(SystemState state, const std::string& message) {
    SystemState oldState = currentState;
    currentState = state;
    statusMessage = message;
    
    if (oldState != state) {
        logStateChange(oldState, state);
        
        // Dispatch state change to global script
        if (globalScriptActive && globalScriptInitialized && scriptAuthority) {
            // TODO: Add custom event dispatch for state changes
            // scriptAuthority->dispatchCustomEvent("onSystemStateChange", 0, 0);
        }
    }
}

void MainPanel::updateFPS() {
    uint32_t currentTime = getCurrentTimeMs();
    
    if (currentTime - lastFPSUpdate >= 1000) { // Update FPS every second
        currentFPS = frameCount;
        frameCount = 0;
        lastFPSUpdate = currentTime;
    }
}

void MainPanel::onActivate() {
    ESP_LOGI(MAIN_PANEL_TAG, "MainPanel activated");
    
    // Dispatch activation event to global script
    if (globalScriptActive && globalScriptInitialized && scriptAuthority) {
        // TODO: Dispatch onActivate event to global script
    }
}

void MainPanel::onDeactivate() {
    ESP_LOGI(MAIN_PANEL_TAG, "MainPanel deactivated");
    
    // Dispatch deactivation event to global script
    if (globalScriptActive && globalScriptInitialized && scriptAuthority) {
        // TODO: Dispatch onDeactivate event to global script
    }
}

void MainPanel::onScriptAttached(const std::string& scriptName) {
    ESP_LOGI(MAIN_PANEL_TAG, "Script attached: %s", scriptName.c_str());
}

void MainPanel::onScriptDetached(const std::string& scriptName) {
    ESP_LOGI(MAIN_PANEL_TAG, "Script detached: %s", scriptName.c_str());
}

void MainPanel::onScriptError(const std::string& error) {
    ESP_LOGE(MAIN_PANEL_TAG, "Global script error: %s", error.c_str());
    
    // Set error state
    setSystemState(SystemState::ERROR_STATE, "Script error: " + error);
    
    // Call base implementation
    MenuPanel::onScriptError(error);
}

void MainPanel::registerUIElements() {
    if (!namedRegistry || uiElementsRegistered) {
        return;
    }
    
    ESP_LOGI(MAIN_PANEL_TAG, "Registering UI elements as named entities");
    
    // Register main menu UI elements
    // Note: In a real implementation, these would be actual UI entity UUIDs
    // For now, we'll use placeholder UUIDs that would be created by the UI system
    
    uint32_t menuTitleUUID = 2001; // Would be assigned by UI system
    uint32_t statusDisplayUUID = 2002;
    uint32_t debugPanelUUID = 2003;
    uint32_t fpsDisplayUUID = 2004;
    
    namedRegistry->registerEntity(menuTitleUUID, "menu_title", "ui_text", getPanelId());
    namedRegistry->registerEntity(statusDisplayUUID, "status_display", "ui_text", getPanelId());
    namedRegistry->registerEntity(debugPanelUUID, "debug_panel", "ui_panel", getPanelId());
    namedRegistry->registerEntity(fpsDisplayUUID, "fps_display", "ui_text", getPanelId());
    
    // Add UI elements to groups
    namedRegistry->addToGroup("menu_title", "main_menu_ui", getPanelId());
    namedRegistry->addToGroup("status_display", "main_menu_ui", getPanelId());
    namedRegistry->addToGroup("debug_panel", "debug_ui", getPanelId());
    namedRegistry->addToGroup("fps_display", "debug_ui", getPanelId());
    
    // Add tags
    namedRegistry->addTag("debug_panel", "toggleable", getPanelId());
    namedRegistry->addTag("fps_display", "performance", getPanelId());
    
    // Initially hide debug UI
    namedRegistry->hideEntity("debug_panel", getPanelId());
    
    uiElementsRegistered = true;
    ESP_LOGI(MAIN_PANEL_TAG, "UI elements registered successfully");
}

void MainPanel::unregisterUIElements() {
    if (!namedRegistry || !uiElementsRegistered) {
        return;
    }
    
    ESP_LOGI(MAIN_PANEL_TAG, "Unregistering UI elements");
    
    // Clear panel entities
    namedRegistry->clearPanel(getPanelId());
    
    uiElementsRegistered = false;
    ESP_LOGI(MAIN_PANEL_TAG, "UI elements unregistered");
}

void MainPanel::updateUIElements() {
    if (!namedRegistry || !uiElementsRegistered) {
        return;
    }
    
    // Update status display based on system state
    std::string stateMetadata = "{\"state\": \"" + std::string(stateToString(currentState)) + 
                               "\", \"message\": \"" + statusMessage + "\"}";
    namedRegistry->setMetadata("status_display", stateMetadata, getPanelId());
    
    // Update FPS display
    std::string fpsMetadata = "{\"fps\": " + std::to_string(currentFPS) + 
                             ", \"uptime\": " + std::to_string(getUptimeMs()) + "}";
    namedRegistry->setMetadata("fps_display", fpsMetadata, getPanelId());
}

void MainPanel::handleMainMenuInput(const WispInputState& input) {
    // Dispatch input events to global script
    for (int i = 0; i < WISP_INPUT_COUNT; i++) {
        WispInputSemantic semantic = static_cast<WispInputSemantic>(i);
        bool currentState = input.buttons[i];
        static bool previousState[WISP_INPUT_COUNT] = {false};
        
        if (currentState && !previousState[i]) {
            // Button pressed
            dispatchInputToScript(semantic, true);
        } else if (!currentState && previousState[i]) {
            // Button released
            dispatchInputToScript(semantic, false);
        }
        
        previousState[i] = currentState;
    }
    
    // Handle specific main menu actions
    if (input.buttons[WISP_INPUT_A]) {
        // A button pressed - could load ROM, start app, etc.
        ESP_LOGI(MAIN_PANEL_TAG, "A button pressed in main menu");
    }
    
    if (input.buttons[WISP_INPUT_MENU]) {
        // Menu button pressed - toggle debug info
        if (namedRegistry && uiElementsRegistered) {
            auto state = namedRegistry->getState("debug_panel", getPanelId());
            if (state == NamedEntityRegistry::EntityState::ACTIVE) {
                namedRegistry->hideEntity("debug_panel", getPanelId());
            } else {
                namedRegistry->showEntity("debug_panel", getPanelId());
            }
        }
    }
}

void MainPanel::dispatchInputToScript(WispInputSemantic input, bool pressed) {
    if (globalScriptActive && globalScriptInitialized && scriptAuthority) {
        scriptAuthority->dispatchInputEvent(input, pressed);
    }
}

MainPanel::MainPanelStats MainPanel::getStats() const {
    auto scriptStats = getScriptStats();
    
    return {
        currentState,
        globalScriptActive,
        frameCount,
        currentFPS,
        scriptStats.executionCount,
        scriptStats.errorCount,
        getUptimeMs()
    };
}

void MainPanel::dumpState() const {
    ESP_LOGI(MAIN_PANEL_TAG, "=== MainPanel State Dump ===");
    ESP_LOGI(MAIN_PANEL_TAG, "Panel ID: %d", getPanelId());
    ESP_LOGI(MAIN_PANEL_TAG, "System State: %s", stateToString(currentState));
    ESP_LOGI(MAIN_PANEL_TAG, "Status Message: %s", statusMessage.c_str());
    ESP_LOGI(MAIN_PANEL_TAG, "Global Script: %s (%s)", 
             globalScriptName.c_str(), 
             globalScriptActive ? "active" : "inactive");
    ESP_LOGI(MAIN_PANEL_TAG, "Frame Count: %lu", frameCount);
    ESP_LOGI(MAIN_PANEL_TAG, "FPS: %d", currentFPS);
    ESP_LOGI(MAIN_PANEL_TAG, "Uptime: %lu ms", getUptimeMs());
    ESP_LOGI(MAIN_PANEL_TAG, "UI Elements Registered: %s", uiElementsRegistered ? "yes" : "no");
    
    auto scriptStats = getScriptStats();
    ESP_LOGI(MAIN_PANEL_TAG, "Script Executions: %lu", scriptStats.executionCount);
    ESP_LOGI(MAIN_PANEL_TAG, "Script Errors: %d", scriptStats.errorCount);
    ESP_LOGI(MAIN_PANEL_TAG, "==============================");
}

// Protected Methods

void MainPanel::executeGlobalScript() {
    if (!scriptAuthority) return;
    
    try {
        // Execute global scripts - this will call the script authority's executeGlobalScripts()
        // which will in turn execute our specific global script
        scriptAuthority->executeGlobalScripts();
    } catch (const std::exception& e) {
        onScriptError(e.what());
    }
}

bool MainPanel::initializeGlobalScriptInternal() {
    // TODO: Call initialization function in global script
    // This would dispatch an "onSystemBoot" or "onInitialize" event
    // to the global script
    
    ESP_LOGI(MAIN_PANEL_TAG, "Global script internal initialization");
    
    // For now, just return true
    // In a full implementation, this would:
    // 1. Call script's onSystemBoot() function
    // 2. Verify script responded correctly
    // 3. Set up any script-specific state
    
    return true;
}

void MainPanel::renderSystemStatus() {
    // TODO: Implement actual rendering
    // This would draw the system status text, current state, etc.
    // For now, just log occasionally
    
    static uint32_t lastStatusLog = 0;
    uint32_t currentTime = getCurrentTimeMs();
    
    if (currentTime - lastStatusLog > 5000) { // Log every 5 seconds
        ESP_LOGD(MAIN_PANEL_TAG, "System Status: %s - %s", 
                 stateToString(currentState), statusMessage.c_str());
        lastStatusLog = currentTime;
    }
}

void MainPanel::renderMainMenu() {
    // TODO: Implement actual menu rendering
    // This would draw menu options, buttons, etc.
    ESP_LOGV(MAIN_PANEL_TAG, "Rendering main menu");
}

void MainPanel::renderDebugInfo() {
    // TODO: Implement debug info rendering
    // This would show FPS, memory usage, script stats, etc.
    if (namedRegistry && uiElementsRegistered) {
        auto debugState = namedRegistry->getState("debug_panel", getPanelId());
        if (debugState == NamedEntityRegistry::EntityState::ACTIVE) {
            ESP_LOGV(MAIN_PANEL_TAG, "Rendering debug info - FPS: %d", currentFPS);
        }
    }
}

// Private Methods

const char* MainPanel::stateToString(SystemState state) const {
    switch (state) {
        case SystemState::BOOTING: return "BOOTING";
        case SystemState::MENU_ACTIVE: return "MENU_ACTIVE";
        case SystemState::APP_LOADING: return "APP_LOADING";
        case SystemState::APP_RUNNING: return "APP_RUNNING";
        case SystemState::ERROR_STATE: return "ERROR_STATE";
        default: return "UNKNOWN";
    }
}

void MainPanel::logStateChange(SystemState oldState, SystemState newState) {
    ESP_LOGI(MAIN_PANEL_TAG, "System state changed: %s -> %s (%s)", 
             stateToString(oldState), stateToString(newState), statusMessage.c_str());
}

uint32_t MainPanel::getUptimeMs() const {
    return esp_timer_get_time() / 1000;
}
