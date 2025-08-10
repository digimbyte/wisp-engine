// menu.h - Enhanced Menu Panel class for Wisp Engine with Script Integration
#ifndef MENU_PANEL_H
#define MENU_PANEL_H

#include "../../../engine/app/curated_api.h"
#include "../../../engine/security/script_instance_authority.h"
#include "../../../engine/security/named_entity_registry.h"
#include "../../esp32_common.h"
#include "../../system/definitions.h"
#include <memory>

/**
 * @brief Enhanced Base Menu Panel with Script Integration
 * 
 * Provides script attachment capabilities to UI panels for dynamic behavior.
 * Supports panel scripts that can control UI elements and respond to input.
 * 
 * Key Features:
 * - Script attachment/detachment
 * - Automatic script execution during update
 * - Named entity integration for UI element control
 * - Security-validated script operations
 * - Panel-scoped script context
 */
class MenuPanel {
protected:
    // Core panel properties
    WispCuratedAPI* api;
    bool active;
    std::string panelTitle;
    
    // Script integration
    std::string panelScript;                    // Associated script name
    uint16_t panelId;                           // Panel identifier for scripts
    ScriptInstanceAuthority* scriptAuthority;   // Script authority system
    NamedEntityRegistry* namedRegistry;         // Named entity registry
    bool scriptEnabled;                         // Enable/disable script execution
    
    // Script execution tracking
    uint32_t lastScriptExecution;              // Timestamp of last script run
    uint16_t scriptExecutionCount;              // Number of times script has run
    uint8_t scriptErrorCount;                   // Script error counter
    
    static uint16_t nextPanelId;                // Static panel ID counter
    
public:
    /**
     * @brief Constructor with API pointer
     */
    MenuPanel(WispCuratedAPI* apiPtr) 
        : api(apiPtr), active(false), panelId(++nextPanelId),
          scriptAuthority(nullptr), namedRegistry(nullptr), scriptEnabled(true),
          lastScriptExecution(0), scriptExecutionCount(0), scriptErrorCount(0) {}
    
    /**
     * @brief Constructor with title
     */
    MenuPanel(const std::string& title) 
        : api(nullptr), active(false), panelTitle(title), panelId(++nextPanelId),
          scriptAuthority(nullptr), namedRegistry(nullptr), scriptEnabled(true),
          lastScriptExecution(0), scriptExecutionCount(0), scriptErrorCount(0) {}
    
    virtual ~MenuPanel() {
        // Cleanup script when panel is destroyed
        if (hasScript() && scriptAuthority) {
            detachScript();
        }
    }
    
    // === CORE PANEL METHODS ===
    
    virtual void activate() { 
        active = true;
        onActivate();
    }
    
    virtual void deactivate() { 
        active = false;
        onDeactivate();
    }
    
    virtual bool isActive() const { return active; }
    
    /**
     * @brief Enhanced update with script execution
     */
    virtual void update(const WispInputState& input) {
        if (!active) return;
        
        // Execute panel script first (if attached and enabled)
        if (hasScript() && scriptEnabled && scriptAuthority) {
            executeScript();
        }
        
        // Then call panel-specific update
        updatePanel(input);
    }
    
    /**
     * @brief Panel-specific update (implemented by derived classes)
     */
    virtual void updatePanel(const WispInputState& input) = 0;
    
    virtual void render() = 0;
    
    // === SCRIPT INTEGRATION METHODS ===
    
    /**
     * @brief Attach script to this panel
     * @param scriptName Name of the script to attach
     * @param permissions Permission level for the script
     * @return True if script was attached successfully
     */
    virtual bool attachScript(const std::string& scriptName, 
                             ScriptInstanceAuthority::PermissionLevel permissions = 
                             ScriptInstanceAuthority::PermissionLevel::STANDARD) {
        if (!scriptAuthority) {
            ESP_LOGW("MenuPanel", "Cannot attach script '%s': no script authority", scriptName.c_str());
            return false;
        }
        
        // Detach existing script first
        if (hasScript()) {
            detachScript();
        }
        
        // Create new panel script
        if (scriptAuthority->createPanelScript(scriptName, panelId, permissions)) {
            panelScript = scriptName;
            scriptErrorCount = 0;
            ESP_LOGI("MenuPanel", "Attached script '%s' to panel '%s' (ID: %d)", 
                     scriptName.c_str(), panelTitle.c_str(), panelId);
            onScriptAttached(scriptName);
            return true;
        }
        
        ESP_LOGE("MenuPanel", "Failed to attach script '%s' to panel '%s'", 
                 scriptName.c_str(), panelTitle.c_str());
        return false;
    }
    
    /**
     * @brief Detach script from this panel
     */
    virtual void detachScript() {
        if (hasScript() && scriptAuthority) {
            std::string oldScript = panelScript;
            scriptAuthority->destroyPanelScript(panelId);
            panelScript.clear();
            ESP_LOGI("MenuPanel", "Detached script '%s' from panel '%s'", 
                     oldScript.c_str(), panelTitle.c_str());
            onScriptDetached(oldScript);
        }
    }
    
    /**
     * @brief Check if panel has an attached script
     */
    virtual bool hasScript() const {
        return !panelScript.empty();
    }
    
    /**
     * @brief Get name of attached script
     */
    virtual const std::string& getScriptName() const {
        return panelScript;
    }
    
    /**
     * @brief Enable or disable script execution
     */
    virtual void setScriptEnabled(bool enabled) {
        scriptEnabled = enabled;
        ESP_LOGD("MenuPanel", "Script execution %s for panel '%s'", 
                 enabled ? "enabled" : "disabled", panelTitle.c_str());
    }
    
    /**
     * @brief Check if script execution is enabled
     */
    virtual bool isScriptEnabled() const {
        return scriptEnabled;
    }
    
    // === SYSTEM INTEGRATION ===
    
    /**
     * @brief Set script authority reference
     */
    virtual void setScriptAuthority(ScriptInstanceAuthority* authority) {
        scriptAuthority = authority;
    }
    
    /**
     * @brief Set named entity registry reference
     */
    virtual void setNamedEntityRegistry(NamedEntityRegistry* registry) {
        namedRegistry = registry;
    }
    
    /**
     * @brief Get panel ID for script context
     */
    virtual uint16_t getPanelId() const {
        return panelId;
    }
    
    // === LIFECYCLE CALLBACKS ===
    
    virtual void cleanup() {
        detachScript();
    }
    
    virtual bool init() { 
        return true;
    }
    
    // === EVENT CALLBACKS (Override in derived classes) ===
    
    /**
     * @brief Called when panel is activated
     */
    virtual void onActivate() {}
    
    /**
     * @brief Called when panel is deactivated
     */
    virtual void onDeactivate() {}
    
    /**
     * @brief Called when script is successfully attached
     */
    virtual void onScriptAttached(const std::string& scriptName) {}
    
    /**
     * @brief Called when script is detached
     */
    virtual void onScriptDetached(const std::string& scriptName) {}
    
    /**
     * @brief Called when script encounters an error
     */
    virtual void onScriptError(const std::string& error) {
        scriptErrorCount++;
        ESP_LOGW("MenuPanel", "Script error in panel '%s': %s (count: %d)", 
                 panelTitle.c_str(), error.c_str(), scriptErrorCount);
        
        // Auto-disable script after too many errors
        if (scriptErrorCount >= 5) {
            ESP_LOGE("MenuPanel", "Too many script errors, disabling script for panel '%s'", 
                     panelTitle.c_str());
            setScriptEnabled(false);
        }
    }
    
    // === UTILITY METHODS ===
    
    virtual const char* getTitle() const { 
        return panelTitle.c_str();
    }
    
    virtual void setAPI(WispCuratedAPI* apiPtr) { 
        api = apiPtr;
    }
    
    /**
     * @brief Get script execution statistics
     */
    struct ScriptStats {
        uint32_t executionCount;
        uint32_t lastExecutionTime;
        uint8_t errorCount;
        bool enabled;
    };
    
    virtual ScriptStats getScriptStats() const {
        return {
            scriptExecutionCount,
            lastScriptExecution,
            scriptErrorCount,
            scriptEnabled
        };
    }

protected:
    /**
     * @brief Execute the attached panel script
     */
    virtual void executeScript() {
        if (!scriptAuthority || !hasScript()) return;
        
        try {
            // Execute panel script update function
            scriptAuthority->executePanelScripts();
            
            // Update execution tracking
            scriptExecutionCount++;
            lastScriptExecution = getCurrentTimeMs();
            
        } catch (const std::exception& e) {
            onScriptError(e.what());
        }
    }
    
    /**
     * @brief Get current time in milliseconds
     */
    virtual uint32_t getCurrentTimeMs() {
        return esp_timer_get_time() / 1000;
    }
};

// Static member initialization
uint16_t MenuPanel::nextPanelId = 0;

#endif // MENU_PANEL_H
