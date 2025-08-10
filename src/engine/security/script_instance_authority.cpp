#include "script_instance_authority.h"
#include "uuid_authority.h"
#include "esp_timer.h"
#include <algorithm>

// Static instance counter for performance tracking
static uint32_t s_totalScriptExecutions = 0;
static uint32_t s_totalSecurityViolations = 0;

bool ScriptInstanceAuthority::initialize(SecureWASHAPIBridge* bridge, EngineUUIDAuthority* authority) {
    if (!bridge || !authority) {
        ESP_LOGE(SCRIPT_AUTH_TAG, "Cannot initialize: null bridge or authority");
        return false;
    }
    
    apiBridge = bridge;
    uuidAuthority = authority;
    
    // Initialize VM with secure bridge
    if (!vm.initialize()) {
        ESP_LOGE(SCRIPT_AUTH_TAG, "Failed to initialize WASH VM");
        return false;
    }
    
    // Reserve capacity for expected script load
    activeScripts.reserve(MAX_ACTIVE_SCRIPTS);
    entityScriptMap.reserve(32);
    panelScriptMap.reserve(8);
    globalScriptMap.reserve(4);
    
    // Initialize performance tracking
    resetFrameCounters();
    
    ESP_LOGI(SCRIPT_AUTH_TAG, "Script Instance Authority initialized successfully");
    ESP_LOGI(SCRIPT_AUTH_TAG, "Max scripts: %d, Max instructions/frame: %d, Max execution time: %d μs", 
             MAX_ACTIVE_SCRIPTS, MAX_INSTRUCTIONS_PER_FRAME, MAX_EXECUTION_TIME_MICROS);
    
    return true;
}

void ScriptInstanceAuthority::shutdown() {
    ESP_LOGI(SCRIPT_AUTH_TAG, "Shutting down Script Instance Authority");
    ESP_LOGI(SCRIPT_AUTH_TAG, "Final stats - Total executions: %lu, Security violations: %lu", 
             s_totalScriptExecutions, s_totalSecurityViolations);
    
    // Clean up all active scripts
    for (auto& script : activeScripts) {
        if (script.bytecode) {
            // Don't delete bytecode - it's owned by ROM loader
            script.bytecode = nullptr;
        }
    }
    
    activeScripts.clear();
    entityScriptMap.clear();
    panelScriptMap.clear();
    globalScriptMap.clear();
    
    vm.shutdown();
}

bool ScriptInstanceAuthority::createEntityScript(const std::string& scriptName, uint32_t entityUUID, 
                                                PermissionLevel permissions) {
    if (activeScripts.size() >= MAX_ACTIVE_SCRIPTS) {
        ESP_LOGW(SCRIPT_AUTH_TAG, "Cannot create entity script '%s': max scripts reached", scriptName.c_str());
        return false;
    }
    
    // Validate entity exists in UUID authority
    if (!uuidAuthority->validateUUID(entityUUID, "script_attach")) {
        ESP_LOGW(SCRIPT_AUTH_TAG, "Cannot create entity script '%s': invalid UUID %lu", 
                 scriptName.c_str(), entityUUID);
        return false;
    }
    
    // Check if entity already has a script
    if (entityScriptMap.find(entityUUID) != entityScriptMap.end()) {
        ESP_LOGW(SCRIPT_AUTH_TAG, "Entity %lu already has a script attached", entityUUID);
        return false;
    }
    
    // Load bytecode
    WASHBytecode* bytecode = nullptr;
    if (!loadScriptBytecode(scriptName, &bytecode)) {
        ESP_LOGE(SCRIPT_AUTH_TAG, "Failed to load bytecode for entity script '%s'", scriptName.c_str());
        return false;
    }
    
    // Create script instance
    ScriptInstance script;
    script.scriptName = scriptName;
    script.scriptType = ScriptType::ENTITY;
    script.contextUUID = entityUUID;
    script.contextPanelId = 0;
    script.bytecode = bytecode;
    script.permissions = permissions;
    script.allowedOperations = getPermittedOperations(permissions, ScriptType::ENTITY);
    script.active = true;
    script.paused = false;
    
    size_t scriptIndex = activeScripts.size();
    activeScripts.push_back(script);
    entityScriptMap[entityUUID] = scriptIndex;
    
    ESP_LOGI(SCRIPT_AUTH_TAG, "Created entity script '%s' for UUID %lu with %s permissions", 
             scriptName.c_str(), entityUUID, 
             permissions == PermissionLevel::RESTRICTED ? "RESTRICTED" :
             permissions == PermissionLevel::STANDARD ? "STANDARD" :
             permissions == PermissionLevel::ELEVATED ? "ELEVATED" : "SYSTEM");
    
    return true;
}

bool ScriptInstanceAuthority::createPanelScript(const std::string& scriptName, uint16_t panelId,
                                               PermissionLevel permissions) {
    if (activeScripts.size() >= MAX_ACTIVE_SCRIPTS) {
        ESP_LOGW(SCRIPT_AUTH_TAG, "Cannot create panel script '%s': max scripts reached", scriptName.c_str());
        return false;
    }
    
    // Check if panel already has a script
    if (panelScriptMap.find(panelId) != panelScriptMap.end()) {
        ESP_LOGW(SCRIPT_AUTH_TAG, "Panel %d already has a script attached", panelId);
        return false;
    }
    
    // Load bytecode
    WASHBytecode* bytecode = nullptr;
    if (!loadScriptBytecode(scriptName, &bytecode)) {
        ESP_LOGE(SCRIPT_AUTH_TAG, "Failed to load bytecode for panel script '%s'", scriptName.c_str());
        return false;
    }
    
    // Create script instance
    ScriptInstance script;
    script.scriptName = scriptName;
    script.scriptType = ScriptType::PANEL;
    script.contextUUID = 0;
    script.contextPanelId = panelId;
    script.bytecode = bytecode;
    script.permissions = permissions;
    script.allowedOperations = getPermittedOperations(permissions, ScriptType::PANEL);
    script.active = true;
    script.paused = false;
    
    size_t scriptIndex = activeScripts.size();
    activeScripts.push_back(script);
    panelScriptMap[panelId] = scriptIndex;
    
    ESP_LOGI(SCRIPT_AUTH_TAG, "Created panel script '%s' for panel %d with %s permissions", 
             scriptName.c_str(), panelId,
             permissions == PermissionLevel::RESTRICTED ? "RESTRICTED" :
             permissions == PermissionLevel::STANDARD ? "STANDARD" :
             permissions == PermissionLevel::ELEVATED ? "ELEVATED" : "SYSTEM");
    
    return true;
}

bool ScriptInstanceAuthority::createGlobalScript(const std::string& scriptName,
                                                PermissionLevel permissions) {
    if (activeScripts.size() >= MAX_ACTIVE_SCRIPTS) {
        ESP_LOGW(SCRIPT_AUTH_TAG, "Cannot create global script '%s': max scripts reached", scriptName.c_str());
        return false;
    }
    
    // Check if global script already exists
    if (globalScriptMap.find(scriptName) != globalScriptMap.end()) {
        ESP_LOGW(SCRIPT_AUTH_TAG, "Global script '%s' already exists", scriptName.c_str());
        return false;
    }
    
    // Load bytecode
    WASHBytecode* bytecode = nullptr;
    if (!loadScriptBytecode(scriptName, &bytecode)) {
        ESP_LOGE(SCRIPT_AUTH_TAG, "Failed to load bytecode for global script '%s'", scriptName.c_str());
        return false;
    }
    
    // Global scripts should have SYSTEM permissions by default
    if (permissions < PermissionLevel::SYSTEM) {
        ESP_LOGW(SCRIPT_AUTH_TAG, "Elevating global script '%s' to SYSTEM permissions", scriptName.c_str());
        permissions = PermissionLevel::SYSTEM;
    }
    
    // Create script instance
    ScriptInstance script;
    script.scriptName = scriptName;
    script.scriptType = ScriptType::GLOBAL;
    script.contextUUID = 0;
    script.contextPanelId = 0;
    script.bytecode = bytecode;
    script.permissions = permissions;
    script.allowedOperations = getPermittedOperations(permissions, ScriptType::GLOBAL);
    script.active = true;
    script.paused = false;
    
    size_t scriptIndex = activeScripts.size();
    activeScripts.push_back(script);
    globalScriptMap[scriptName] = scriptIndex;
    
    ESP_LOGI(SCRIPT_AUTH_TAG, "Created global script '%s' with SYSTEM permissions", scriptName.c_str());
    
    return true;
}

void ScriptInstanceAuthority::destroyEntityScript(uint32_t entityUUID) {
    auto it = entityScriptMap.find(entityUUID);
    if (it == entityScriptMap.end()) {
        return; // No script attached to this entity
    }
    
    size_t scriptIndex = it->second;
    if (scriptIndex >= activeScripts.size()) {
        ESP_LOGE(SCRIPT_AUTH_TAG, "Invalid script index %zu for entity %lu", scriptIndex, entityUUID);
        entityScriptMap.erase(it);
        return;
    }
    
    ScriptInstance& script = activeScripts[scriptIndex];
    ESP_LOGI(SCRIPT_AUTH_TAG, "Destroying entity script '%s' for UUID %lu", 
             script.scriptName.c_str(), entityUUID);
    
    // Mark script as inactive
    script.active = false;
    script.bytecode = nullptr;
    
    // Remove from entity map
    entityScriptMap.erase(it);
    
    // Update other maps if this script was swapped from end
    // (This is a simplified approach - in production we'd use a more efficient removal strategy)
}

void ScriptInstanceAuthority::destroyPanelScript(uint16_t panelId) {
    auto it = panelScriptMap.find(panelId);
    if (it == panelScriptMap.end()) {
        return; // No script attached to this panel
    }
    
    size_t scriptIndex = it->second;
    if (scriptIndex >= activeScripts.size()) {
        ESP_LOGE(SCRIPT_AUTH_TAG, "Invalid script index %zu for panel %d", scriptIndex, panelId);
        panelScriptMap.erase(it);
        return;
    }
    
    ScriptInstance& script = activeScripts[scriptIndex];
    ESP_LOGI(SCRIPT_AUTH_TAG, "Destroying panel script '%s' for panel %d", 
             script.scriptName.c_str(), panelId);
    
    // Mark script as inactive
    script.active = false;
    script.bytecode = nullptr;
    
    // Remove from panel map
    panelScriptMap.erase(it);
}

void ScriptInstanceAuthority::destroyGlobalScript(const std::string& scriptName) {
    auto it = globalScriptMap.find(scriptName);
    if (it == globalScriptMap.end()) {
        ESP_LOGW(SCRIPT_AUTH_TAG, "Global script '%s' not found for destruction", scriptName.c_str());
        return;
    }
    
    size_t scriptIndex = it->second;
    if (scriptIndex >= activeScripts.size()) {
        ESP_LOGE(SCRIPT_AUTH_TAG, "Invalid script index %zu for global script '%s'", 
                 scriptIndex, scriptName.c_str());
        globalScriptMap.erase(it);
        return;
    }
    
    ScriptInstance& script = activeScripts[scriptIndex];
    ESP_LOGI(SCRIPT_AUTH_TAG, "Destroying global script '%s'", scriptName.c_str());
    
    // Mark script as inactive
    script.active = false;
    script.bytecode = nullptr;
    
    // Remove from global map
    globalScriptMap.erase(it);
}

void ScriptInstanceAuthority::executeEntityScripts() {
    frameStartTime = esp_timer_get_time();
    totalScriptsExecuted = 0;
    totalExecutionTimeMicros = 0;
    
    for (auto& [entityUUID, scriptIndex] : entityScriptMap) {
        if (scriptIndex >= activeScripts.size()) continue;
        
        ScriptInstance& script = activeScripts[scriptIndex];
        if (!script.active || script.paused || script.quarantined) continue;
        
        // Set execution context for this entity script
        apiBridge->setExecutionContext(script.scriptName, entityUUID, 0);
        
        // Execute onUpdate function
        if (executeScriptWithContext(scriptIndex, "onUpdate")) {
            totalScriptsExecuted++;
        }
        
        // Check if we're exceeding frame budget
        uint32_t currentTime = esp_timer_get_time();
        if (currentTime - frameStartTime > MAX_EXECUTION_TIME_MICROS) {
            ESP_LOGW(SCRIPT_AUTH_TAG, "Entity script execution exceeded frame budget, stopping early");
            break;
        }
    }
    
    ESP_LOGD(SCRIPT_AUTH_TAG, "Executed %d entity scripts in %lu μs", 
             totalScriptsExecuted, totalExecutionTimeMicros);
}

void ScriptInstanceAuthority::executePanelScripts() {
    for (auto& [panelId, scriptIndex] : panelScriptMap) {
        if (scriptIndex >= activeScripts.size()) continue;
        
        ScriptInstance& script = activeScripts[scriptIndex];
        if (!script.active || script.paused || script.quarantined) continue;
        
        // Set execution context for this panel script
        apiBridge->setExecutionContext(script.scriptName, 0, panelId);
        
        // Execute onUpdate function
        if (executeScriptWithContext(scriptIndex, "onUpdate")) {
            totalScriptsExecuted++;
        }
    }
}

void ScriptInstanceAuthority::executeGlobalScripts() {
    for (auto& [scriptName, scriptIndex] : globalScriptMap) {
        if (scriptIndex >= activeScripts.size()) continue;
        
        ScriptInstance& script = activeScripts[scriptIndex];
        if (!script.active || script.paused || script.quarantined) continue;
        
        // Set execution context for this global script
        apiBridge->setExecutionContext(script.scriptName, 0, 0);
        
        // Execute onUpdate function
        if (executeScriptWithContext(scriptIndex, "onUpdate")) {
            totalScriptsExecuted++;
        }
    }
}

void ScriptInstanceAuthority::dispatchCollisionEvent(uint32_t entityA, uint32_t entityB) {
    // Dispatch to entity A's script
    auto itA = entityScriptMap.find(entityA);
    if (itA != entityScriptMap.end()) {
        size_t scriptIndex = itA->second;
        if (scriptIndex < activeScripts.size()) {
            ScriptInstance& script = activeScripts[scriptIndex];
            if (script.active && !script.paused && !script.quarantined) {
                apiBridge->setExecutionContext(script.scriptName, entityA, 0);
                // TODO: Pass entityB as parameter to onCollision function
                executeScriptWithContext(scriptIndex, "onCollision");
            }
        }
    }
    
    // Dispatch to entity B's script (if different)
    if (entityA != entityB) {
        auto itB = entityScriptMap.find(entityB);
        if (itB != entityScriptMap.end()) {
            size_t scriptIndex = itB->second;
            if (scriptIndex < activeScripts.size()) {
                ScriptInstance& script = activeScripts[scriptIndex];
                if (script.active && !script.paused && !script.quarantined) {
                    apiBridge->setExecutionContext(script.scriptName, entityB, 0);
                    executeScriptWithContext(scriptIndex, "onCollision");
                }
            }
        }
    }
}

void ScriptInstanceAuthority::dispatchInputEvent(WispInputSemantic input, bool pressed) {
    // Dispatch to all panel scripts
    for (auto& [panelId, scriptIndex] : panelScriptMap) {
        if (scriptIndex >= activeScripts.size()) continue;
        
        ScriptInstance& script = activeScripts[scriptIndex];
        if (script.active && !script.paused && !script.quarantined) {
            apiBridge->setExecutionContext(script.scriptName, 0, panelId);
            // TODO: Pass input and pressed state as parameters
            executeScriptWithContext(scriptIndex, pressed ? "onInputPressed" : "onInputReleased");
        }
    }
    
    // Dispatch to global scripts
    for (auto& [scriptName, scriptIndex] : globalScriptMap) {
        if (scriptIndex >= activeScripts.size()) continue;
        
        ScriptInstance& script = activeScripts[scriptIndex];
        if (script.active && !script.paused && !script.quarantined) {
            apiBridge->setExecutionContext(script.scriptName, 0, 0);
            executeScriptWithContext(scriptIndex, pressed ? "onInputPressed" : "onInputReleased");
        }
    }
}

void ScriptInstanceAuthority::dispatchAnimationEvent(uint32_t entityUUID, uint8_t animationId) {
    auto it = entityScriptMap.find(entityUUID);
    if (it != entityScriptMap.end()) {
        size_t scriptIndex = it->second;
        if (scriptIndex < activeScripts.size()) {
            ScriptInstance& script = activeScripts[scriptIndex];
            if (script.active && !script.paused && !script.quarantined) {
                apiBridge->setExecutionContext(script.scriptName, entityUUID, 0);
                // TODO: Pass animationId as parameter
                executeScriptWithContext(scriptIndex, "onAnimationComplete");
            }
        }
    }
}

ScriptInstanceAuthority::SystemStats ScriptInstanceAuthority::getSystemStats() const {
    SystemStats stats = {};
    
    for (const auto& script : activeScripts) {
        if (!script.active) continue;
        
        if (script.quarantined) {
            stats.quarantinedScripts++;
            continue;
        }
        
        switch (script.scriptType) {
            case ScriptType::ENTITY:
                stats.activeEntityScripts++;
                break;
            case ScriptType::PANEL:
                stats.activePanelScripts++;
                break;
            case ScriptType::GLOBAL:
                stats.activeGlobalScripts++;
                break;
        }
        
        stats.totalExecutionTimeThisFrame += script.totalExecutionTime;
        stats.totalAPICallsThisFrame += script.apiCallCount;
    }
    
    return stats;
}

void ScriptInstanceAuthority::cleanupScripts() {
    // Remove inactive scripts (swap-remove for efficiency)
    for (size_t i = 0; i < activeScripts.size(); ) {
        if (!activeScripts[i].active) {
            // Swap with last element and pop
            if (i != activeScripts.size() - 1) {
                activeScripts[i] = activeScripts.back();
                // Update maps to point to new location
                updateMapsAfterSwap(i, activeScripts.size() - 1);
            }
            activeScripts.pop_back();
        } else {
            i++;
        }
    }
    
    ESP_LOGD(SCRIPT_AUTH_TAG, "Cleanup complete. Active scripts: %zu", activeScripts.size());
}

// Private Methods

bool ScriptInstanceAuthority::loadScriptBytecode(const std::string& scriptName, WASHBytecode** bytecode) {
    // TODO: Implement ROM script loading
    // For now, return nullptr to indicate failure
    ESP_LOGW(SCRIPT_AUTH_TAG, "Script bytecode loading not yet implemented for '%s'", scriptName.c_str());
    *bytecode = nullptr;
    return false;
}

bool ScriptInstanceAuthority::executeScriptWithContext(size_t scriptIndex, const std::string& functionName) {
    if (scriptIndex >= activeScripts.size()) return false;
    
    ScriptInstance& script = activeScripts[scriptIndex];
    if (!script.active || !script.bytecode) return false;
    
    uint32_t startTime = esp_timer_get_time();
    
    // Reset per-frame counters
    script.instructionCount = 0;
    script.apiCallCount = 0;
    
    // Execute the script function
    bool success = false;
    try {
        success = vm.executeFunction(script.bytecode, functionName.c_str(), 
                                   script.contextUUID, script.contextPanelId);
        s_totalScriptExecutions++;
    } catch (...) {
        ESP_LOGE(SCRIPT_AUTH_TAG, "Exception in script '%s' function '%s'", 
                 script.scriptName.c_str(), functionName.c_str());
        script.errorCount++;
        success = false;
    }
    
    uint32_t executionTime = esp_timer_get_time() - startTime;
    
    // Update statistics
    updateScriptStats(scriptIndex, executionTime, script.instructionCount, script.apiCallCount);
    
    // Check for quarantine conditions
    if (script.errorCount >= MAX_ERRORS_BEFORE_QUARANTINE) {
        quarantineScript(scriptIndex, "Too many execution errors");
        return false;
    }
    
    if (executionTime > MAX_EXECUTION_TIME_MICROS) {
        recordSecurityViolation(scriptIndex, "Execution time exceeded");
    }
    
    if (script.instructionCount > MAX_INSTRUCTIONS_PER_FRAME) {
        recordSecurityViolation(scriptIndex, "Instruction count exceeded");
    }
    
    if (script.apiCallCount > MAX_API_CALLS_PER_FRAME) {
        recordSecurityViolation(scriptIndex, "API call count exceeded");
    }
    
    return success;
}

void ScriptInstanceAuthority::updateScriptStats(size_t scriptIndex, uint32_t executionTime, 
                                               uint16_t instructions, uint16_t apiCalls) {
    if (scriptIndex >= activeScripts.size()) return;
    
    ScriptInstance& script = activeScripts[scriptIndex];
    script.lastExecutionTime = executionTime;
    script.totalExecutionTime += executionTime;
    script.instructionCount = instructions;
    script.apiCallCount = apiCalls;
    
    totalExecutionTimeMicros += executionTime;
}

void ScriptInstanceAuthority::recordSecurityViolation(size_t scriptIndex, const std::string& violation) {
    if (scriptIndex >= activeScripts.size()) return;
    
    ScriptInstance& script = activeScripts[scriptIndex];
    script.securityViolations++;
    s_totalSecurityViolations++;
    
    ESP_LOGW(SCRIPT_AUTH_TAG, "Security violation in script '%s': %s (count: %d)", 
             script.scriptName.c_str(), violation.c_str(), script.securityViolations);
    
    if (script.securityViolations >= MAX_SECURITY_VIOLATIONS) {
        quarantineScript(scriptIndex, "Too many security violations");
    }
}

void ScriptInstanceAuthority::quarantineScript(size_t scriptIndex, const std::string& reason) {
    if (scriptIndex >= activeScripts.size()) return;
    
    ScriptInstance& script = activeScripts[scriptIndex];
    script.quarantined = true;
    script.active = false;
    
    ESP_LOGE(SCRIPT_AUTH_TAG, "QUARANTINED script '%s': %s", 
             script.scriptName.c_str(), reason.c_str());
}

std::set<std::string> ScriptInstanceAuthority::getPermittedOperations(PermissionLevel level, ScriptType type) {
    std::set<std::string> operations;
    
    // Base operations for all scripts
    operations.insert("math_operations");
    operations.insert("get_position");
    
    if (level >= PermissionLevel::STANDARD) {
        operations.insert("set_position");
        operations.insert("move_entity");
        operations.insert("play_sound");
        operations.insert("set_animation");
    }
    
    if (level >= PermissionLevel::ELEVATED) {
        operations.insert("spawn_entity");
        operations.insert("destroy_entity");
        operations.insert("find_entities");
    }
    
    if (level >= PermissionLevel::SYSTEM && type == ScriptType::GLOBAL) {
        operations.insert("system_operations");
        operations.insert("debug_operations");
        operations.insert("resource_management");
    }
    
    return operations;
}

void ScriptInstanceAuthority::resetFrameCounters() {
    frameStartTime = esp_timer_get_time();
    totalScriptsExecuted = 0;
    totalExecutionTimeMicros = 0;
    
    // Reset per-script frame counters
    for (auto& script : activeScripts) {
        script.instructionCount = 0;
        script.apiCallCount = 0;
    }
}

void ScriptInstanceAuthority::updateMapsAfterSwap(size_t newIndex, size_t oldIndex) {
    // Update entity script map
    for (auto& [uuid, index] : entityScriptMap) {
        if (index == oldIndex) {
            index = newIndex;
            break;
        }
    }
    
    // Update panel script map
    for (auto& [panelId, index] : panelScriptMap) {
        if (index == oldIndex) {
            index = newIndex;
            break;
        }
    }
    
    // Update global script map
    for (auto& [scriptName, index] : globalScriptMap) {
        if (index == oldIndex) {
            index = newIndex;
            break;
        }
    }
}
