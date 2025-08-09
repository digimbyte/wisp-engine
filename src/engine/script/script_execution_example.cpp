// Script Execution Example
// Shows how ASH scripts are securely processed in the main loop with UUID tracking

#include "secure_wash_vm.h"

namespace WispEngine {
namespace Script {

// === EXAMPLE: MAIN LOOP SCRIPT PROCESSING ===

void WASHRuntime::updateAllScripts() {
    uint32_t frameStartTime = esp_log_timestamp();
    scriptsExecutedThisFrame = 0;
    totalExecutionTime = 0;
    
    // Update scripts in priority order: Global -> Panel -> Entity
    updateGlobalScripts();
    updatePanelScripts();
    updateEntityScripts();
    
    // Performance monitoring
    uint32_t frameEndTime = esp_log_timestamp();
    totalExecutionTime = frameEndTime - frameStartTime;
    
    // Safety check: if scripts take too long, reduce their execution frequency
    if (totalExecutionTime > 10) {  // 10ms budget per frame
        ESP_LOGW("WASH", "Scripts exceeded time budget: %d ms", totalExecutionTime);
        // Could implement adaptive throttling here
    }
}

void WASHRuntime::updateEntityScripts() {
    for (uint16_t i = 0; i < scriptCount; i++) {
        WASHScriptInstance& script = scripts[i];
        
        // Skip inactive entity scripts
        if (!script.active || script.paused || script.scriptType != "entity") {
            continue;
        }
        
        // Check if entity still exists
        if (!uuidTracker.isValid(script.contextUUID)) {
            ESP_LOGD("WASH", "Entity %d no longer exists, destroying script", script.contextUUID);
            script.active = false;
            continue;
        }
        
        // Execute the script's onUpdate() function through the secure VM
        uint32_t executionStart = esp_log_timestamp();
        
        bool success = vm.executeScript(script.bytecode, "onUpdate", 
                                       script.contextUUID, 0);
        
        uint32_t executionTime = esp_log_timestamp() - executionStart;
        
        // Update performance metrics
        script.totalExecutionTime += executionTime;
        script.executionCount++;
        script.lastUpdateTime = executionStart;
        
        if (!success) {
            script.errorCount++;
            ESP_LOGW("WASH", "Script %s failed: %s", 
                    script.scriptName.c_str(), vm.getError().c_str());
            
            // Disable scripts with too many errors
            if (script.errorCount > 5) {
                ESP_LOGW("WASH", "Disabling script %s due to excessive errors", 
                        script.scriptName.c_str());
                script.active = false;
            }
        }
        
        scriptsExecutedThisFrame++;
    }
}

// === EXAMPLE: SECURE BYTECODE EXECUTION ===

bool WASHVirtualMachine::executeScript(WASHBytecode* bytecode, const String& functionName,
                                      uint32_t entityUUID, uint16_t panelId) {
    // Security setup
    currentBytecode = bytecode;
    contextUUID = entityUUID;
    contextPanelId = panelId;
    
    // Find function entry point
    uint16_t functionOffset = 0;
    bool foundFunction = false;
    for (uint8_t i = 0; i < bytecode->functionCount; i++) {
        if (bytecode->functionNames[i] == functionName) {
            functionOffset = bytecode->functionOffsets[i];
            foundFunction = true;
            break;
        }
    }
    
    if (!foundFunction) {
        setError("Function not found: " + functionName);
        return false;
    }
    
    // Initialize secure execution context
    reset();
    codeStart = bytecode->code;
    codeEnd = bytecode->code + bytecode->codeSize;
    ip = codeStart + functionOffset;
    
    // Set security limits
    executionStartTime = esp_log_timestamp();
    maxExecutionTime = bytecode->maxExecutionTime;
    maxInstructions = 1000;  // Prevent DoS attacks
    instructionCount = 0;
    
    // Execute bytecode securely
    return runBytecode();
}

bool WASHVirtualMachine::runBytecode() {
    while (isRunning()) {
        // Security checks on every instruction
        if (!boundsCheck(ip) || !timeoutCheck() || !instructionLimitCheck()) {
            return false;
        }
        
        // Fetch and execute instruction
        WASHOpCode opcode = static_cast<WASHOpCode>(*ip++);
        instructionCount++;
        
        if (!executeInstruction(opcode)) {
            return false;
        }
    }
    
    return !error;
}

// === EXAMPLE: CURATED API BRIDGE ===

bool WASHVirtualMachine::executeInstruction(WASHOpCode opcode) {
    switch (opcode) {
        // Only safe operations allowed
        case OP_PUSH_INT: {
            if (!stackCheck(0)) return false;  // Check stack space
            uint32_t value = *reinterpret_cast<const uint32_t*>(ip);
            ip += 4;
            return push(WASHValue(static_cast<int32_t>(value)));
        }
        
        case OP_ADD: {
            if (!stackCheck(2)) return false;  // Need 2 values on stack
            WASHValue b = pop();
            WASHValue a = pop();
            
            // Safe arithmetic - check for overflow
            if (a.type == WASHValue::INT_VAL && b.type == WASHValue::INT_VAL) {
                int64_t result = static_cast<int64_t>(a.intValue) + static_cast<int64_t>(b.intValue);
                if (result > INT32_MAX || result < INT32_MIN) {
                    setError("Integer overflow");
                    return false;
                }
                return push(WASHValue(static_cast<int32_t>(result)));
            } else if (a.type == WASHValue::FLOAT_VAL || b.type == WASHValue::FLOAT_VAL) {
                float result = toFloat(a) + toFloat(b);
                return push(WASHValue(result));
            }
            setError("Invalid types for addition");
            return false;
        }
        
        // CURATED API CALLS - THE ONLY WAY TO AFFECT THE SYSTEM
        case OP_API_MOVE_ENTITY:
            return apiMoveEntity();
            
        case OP_API_SET_POSITION:
            return apiSetPosition();
            
        case OP_API_GET_POSITION:
            return apiGetPosition();
            
        case OP_API_FIND_ENTITIES_BY_TYPE:
            return apiFindEntitiesByType();
            
        case OP_HALT:
            halted = true;
            return true;
            
        default:
            setError("Unknown opcode: " + String(static_cast<int>(opcode)));
            return false;
    }
}

// === EXAMPLE: SECURE API IMPLEMENTATIONS ===

bool WASHVirtualMachine::apiMoveEntity() {
    // moveEntity(uuid, dx, dy) - moves entity by delta
    if (!stackCheck(3)) return false;
    
    WASHValue dy = pop();
    WASHValue dx = pop();
    WASHValue uuidVal = pop();
    
    // Validate parameters
    if (uuidVal.type != WASHValue::UUID_VAL) {
        setError("moveEntity: invalid UUID");
        return false;
    }
    
    uint32_t uuid = uuidVal.uuidValue;
    float deltaX = toFloat(dx);
    float deltaY = toFloat(dy);
    
    // Security: Limit movement speed to prevent exploits
    const float MAX_MOVEMENT = 50.0f;  // pixels per frame
    deltaX = std::clamp(deltaX, -MAX_MOVEMENT, MAX_MOVEMENT);
    deltaY = std::clamp(deltaY, -MAX_MOVEMENT, MAX_MOVEMENT);
    
    // Validate UUID exists
    if (!uuidTracker->isValid(uuid)) {
        setError("moveEntity: invalid entity UUID");
        return false;
    }
    
    // Call through curated API - THE ONLY WAY TO AFFECT ENTITIES
    WispVec2 currentPos = curatedAPI->getEntityPosition(uuid);
    WispVec2 newPos(currentPos.x + deltaX, currentPos.y + deltaY);
    
    // Security: Bounds checking to prevent entities going off-screen
    newPos.x = std::clamp(newPos.x, -100.0f, 340.0f);  // Allow some off-screen
    newPos.y = std::clamp(newPos.y, -100.0f, 260.0f);
    
    bool success = curatedAPI->setEntityPosition(uuid, newPos);
    return push(WASHValue(success));
}

bool WASHVirtualMachine::apiGetPosition() {
    // getPosition(uuid) -> vec2
    if (!stackCheck(1)) return false;
    
    WASHValue uuidVal = pop();
    if (uuidVal.type != WASHValue::UUID_VAL) {
        setError("getPosition: invalid UUID");
        return false;
    }
    
    uint32_t uuid = uuidVal.uuidValue;
    if (!uuidTracker->isValid(uuid)) {
        setError("getPosition: invalid entity UUID");
        return false;
    }
    
    // Get position through curated API
    WispVec2 pos = curatedAPI->getEntityPosition(uuid);
    
    // Return as vec2 value
    WASHValue result;
    result.type = WASHValue::VEC2_VAL;
    result.vec2Value.x = pos.x;
    result.vec2Value.y = pos.y;
    
    return push(result);
}

bool WASHVirtualMachine::apiFindEntitiesByType() {
    // findEntitiesByType(type, panelId) -> array of UUIDs
    if (!stackCheck(2)) return false;
    
    WASHValue panelIdVal = pop();
    WASHValue typeVal = pop();
    
    if (typeVal.type != WASHValue::STRING_VAL) {
        setError("findEntitiesByType: type must be string");
        return false;
    }
    
    String entityType = typeVal.stringValue;
    uint16_t panelId = static_cast<uint16_t>(toInteger(panelIdVal));
    
    // Security: Validate panel access
    if (contextPanelId != 0 && panelId != contextPanelId) {
        // Panel scripts can only access their own panel
        // Entity scripts can access any panel
        if (contextUUID != 0) {  // This is an entity script
            // Allow entity scripts to search any panel
        } else {
            setError("findEntitiesByType: access denied to panel");
            return false;
        }
    }
    
    // Find entities through UUID tracker
    std::vector<uint32_t> foundUUIDs = uuidTracker->findEntitiesByType(entityType, panelId);
    
    // Return as array (for simplicity, just return the first UUID or null)
    if (foundUUIDs.size() > 0) {
        return push(WASHValue(foundUUIDs[0]));
    } else {
        return push(WASHValue());  // null
    }
}

// === EXAMPLE: COLLISION EVENT DISPATCH ===

void WASHRuntime::dispatchCollisionEvent(uint32_t entityA, uint32_t entityB) {
    // Find entity script for entityA
    for (uint16_t i = 0; i < scriptCount; i++) {
        WASHScriptInstance& script = scripts[i];
        
        if (script.active && script.scriptType == "entity" && 
            script.contextUUID == entityA) {
            
            // Set up collision event parameters
            // In a full implementation, we'd push entityB UUID onto the stack
            // and call the script's onCollision(otherUUID) function
            
            bool success = vm.executeScript(script.bytecode, "onCollision", 
                                           entityA, 0);
            
            if (!success) {
                ESP_LOGW("WASH", "Collision script failed for entity %d: %s", 
                        entityA, vm.getError().c_str());
            }
            
            break;  // Found the script
        }
    }
}

// === EXAMPLE: ASH → WASH BYTECODE COMPILATION ===

/*
Unity C# ASH Compiler Example:

ASH Input:
```ash
entity_script "goblin_ai" {
    var health = 100;
    var target = null;
    
    function onUpdate() {
        var players = findEntitiesByType("player", currentPanel);
        if (players != null) {
            target = players;
            var pos = getPosition(self);
            var targetPos = getPosition(target);
            moveEntity(self, targetPos.x - pos.x, targetPos.y - pos.y);
        }
    }
}
```

Generated WASH Bytecode:
```
Header: "WASH", version 1.0, entity script, "goblin_ai"
Functions: ["onUpdate" at offset 0]
Constants: [100, "player", "currentPanel"]

Code:
// onUpdate() function:
0000: PUSH_STRING 1        // "player" 
0003: PUSH_STRING 2        // "currentPanel"
0006: API_FIND_ENTITIES_BY_TYPE
0007: DUP
0008: PUSH_NULL
0009: NE                   // players != null
0010: JUMP_IF_FALSE 0030   // if false, jump to end
0013: STORE_LOCAL 1        // target = players
0016: PUSH_UUID_SELF       // self
0017: API_GET_POSITION     // getPosition(self)
0018: STORE_LOCAL 2        // pos = result
0021: LOAD_LOCAL 1         // target
0023: API_GET_POSITION     // getPosition(target)
0024: LOAD_LOCAL 2         // pos
0026: SUB                  // targetPos - pos (dx, dy)
0027: PUSH_UUID_SELF       // self
0028: API_MOVE_ENTITY      // moveEntity(self, dx, dy)
0030: HALT
```

This bytecode is completely sandboxed and can only call curated API functions!
*/

} // namespace Script
} // namespace WispEngine
