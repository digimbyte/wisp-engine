#pragma once

#include "engine_uuid_authority.h"
#include "script_instance_authority.h"
#include "secure_wash_api_bridge.h"
#include "../app/wisp_segmented_loader.h"
#include "../app/wisp_runtime_loader.h"
#include "engine/scene/scene_system.h"
#include "system/ui/curated_api.h"
#include "system/debug_logger.h"
#include <vector>
#include <string>
#include <unordered_map>

/**
 * @brief Secure ROM Loader - Phase 5 of Secure Script-Entity Integration
 * 
 * Integrates with existing WispSegmentedLoader to provide secure validation
 * of ROM content while respecting ESP32-C6 memory constraints (~200KB).
 * 
 * Architecture:
 * - Uses WispSegmentedLoader for efficient asset streaming
 * - Validates scripts/entities on-demand during panel loading
 * - Respects existing category-based loading strategies
 * - Only loads and validates assets as panels are accessed
 * - Never loads entire ROM into memory at once
 * 
 * Key Security Features:
 * - Panel-scoped security validation (only validate what's loaded)
 * - WASH bytecode validation during script loading
 * - Entity intent validation during panel/layout loading
 * - Resource limit enforcement per panel
 * - Malicious content detection in loaded segments
 */
class SecureROMLoader {
public:
    /**
     * @brief Enhanced script definition with security validation
     */
    struct SecureScriptDef {
        String scriptName;           // Script identifier
        String scriptType;           // "entity", "panel", "global"
        String entityType;           // For entity scripts (optional)
        uint8_t permissionLevel;     // 0=RESTRICTED, 1=STANDARD, 2=ELEVATED, 3=SYSTEM
        size_t bytecodeSize;         // Bytecode size in bytes
        uint32_t bytecodeChecksum;   // Integrity checksum
        bool validated;              // Security validation passed
        String securityNotes;        // Security validation notes
    };

    /**
     * @brief Entity creation intent (ROM cannot specify UUIDs)
     */
    struct EntityIntent {
        String entityType;           // Type identifier
        float x, y;                  // Requested position
        String scriptName;           // Associated script (optional)
        uint16_t panelId;            // Target panel
        uint8_t behavior;            // EntityBehavior enum value
        String metadata;             // Additional entity data
    };

    /**
     * @brief Enhanced app metadata with security validation
     */
    struct SecureAppInfo {
        // Basic app information
        std::string name, version, author, description;
        std::string iconPath, splashPath, executablePath;
        bool autoStart;
        uint16_t screenWidth, screenHeight;
        
        // Security and resource limits
        uint32_t maxEntities;        // Maximum entities ROM can create
        uint32_t maxScripts;         // Maximum concurrent scripts
        uint8_t maxPermissionLevel;  // Highest permission level allowed
        uint32_t memoryLimitKB;      // Memory usage limit
        
        // Script definitions with security validation
        std::vector<SecureScriptDef> scripts;
        
        // Initial entity intents (validated by engine)
        std::vector<EntityIntent> initialEntities;
        
        // ROM integrity
        uint32_t romChecksum;        // Overall ROM checksum
        uint32_t securityVersion;    // Security validation version
        bool validated;              // Security validation passed
    };

    /**
     * @brief ROM loading statistics and security information
     */
    struct LoadingStats {
        uint32_t totalScriptsLoaded;
        uint32_t scriptsValidated;
        uint32_t scriptsRejected;
        uint32_t entitiesCreated;
        uint32_t securityViolations;
        uint32_t loadTimeMs;
        String lastError;
        bool loadSuccessful;
    };

private:
    // Authority system references
    EngineUUIDAuthority* uuidAuthority;
    ScriptInstanceAuthority* scriptAuthority;
    SecureWASHAPIBridge* apiBridge;
    SceneManager* sceneManager;
    WispCuratedAPIExtended* curatedAPI;
    
    // Segmented loader integration (THIS IS THE KEY CHANGE)
    WispEngine::App::WispSegmentedLoader* segmentedLoader;
    
    // Adaptive memory constraints (evaluated at runtime)
    static const uint32_t MIN_SCRIPT_SIZE_KB = 1;        // Minimum script size (1KB)
    static const uint32_t MAX_SCRIPT_SIZE_KB = 128;      // Maximum script size before truncation
    static const uint32_t MIN_PANEL_MEMORY_KB = 8;       // Minimum viable panel memory
    static const uint32_t FALLBACK_MEMORY_KB = 32;       // Reserved for fallback systems
    static const uint32_t SECURITY_VERSION = 1;          // Current security version
    
    // Dynamic limits (calculated per device/load)
    struct DynamicLimits {
        uint32_t availableMemoryKB;      // Current available memory
        uint32_t maxPanelMemoryKB;       // Max memory per panel (calculated)
        uint32_t maxScriptsPerPanel;     // Max scripts per panel (calculated)
        uint32_t maxEntitiesPerPanel;    // Max entities per panel (calculated)
        uint32_t maxAssetCacheKB;        // Max asset cache size (calculated)
        bool useAssetFallbacks;          // Use low-res fallbacks for graphics
        bool truncateScripts;            // Allow script truncation if needed
        bool streamAudio;                // Force audio streaming vs caching
    };
    
    // Loading state
    LoadingStats currentStats;
    SecureAppInfo currentApp;
    bool loadingInProgress;
    String currentROMPath;
    DynamicLimits currentLimits;    // Current memory limits for this session
    
    // Panel-scoped validation cache (memory efficient)
    struct PanelValidationCache {
        uint16_t panelId;
        std::unordered_map<String, bool> validatedScripts;
        uint32_t memoryUsageKB;
        uint32_t lastAccessed;
    };
    
    static const uint8_t MAX_PANEL_CACHE = 4;  // Only cache 4 panels max
    PanelValidationCache panelCache[MAX_PANEL_CACHE];
    uint8_t activePanelCacheCount;

public:
    /**
     * @brief Constructor - Initialize with authority systems
     */
    SecureROMLoader(EngineUUIDAuthority* uuidAuth,
                   ScriptInstanceAuthority* scriptAuth,
                   SecureWASHAPIBridge* bridge,
                   SceneManager* sceneMgr,
                   WispCuratedAPIExtended* api);

    /**
     * @brief Destructor - Cleanup resources
     */
    ~SecureROMLoader();

    // =========================
    // ROM Loading Interface (Segmented)
    // =========================

    /**
     * @brief Initialize ROM with WispSegmentedLoader (lightweight)
     * Only loads config and asset table, no actual content
     * 
     * @param romPath Path to ROM file
     * @return true if ROM structure loaded and validated
     */
    bool initializeROM(const String& romPath);

    /**
     * @brief Load and validate a specific panel's content
     * Only loads scripts/entities for the requested panel
     * 
     * @param panelName Panel to load (e.g. "main_menu", "level_1")
     * @param layoutIndex Layout containing the panel
     * @param panelIndex Index within the layout
     * @return true if panel loaded and validated successfully
     */
    bool loadPanel(const String& panelName, uint8_t layoutIndex, uint8_t panelIndex);

    /**
     * @brief Unload a specific panel and its resources
     * 
     * @param panelName Panel to unload
     * @return true if unloaded successfully
     */
    bool unloadPanel(const String& panelName);

    /**
     * @brief Unload current ROM and cleanup all associated resources
     * 
     * @return true if unloaded successfully
     */
    bool unloadCurrentROM();

    /**
     * @brief Check if a panel is currently loaded and validated
     * 
     * @param panelName Panel to check
     * @return true if panel is loaded and secure
     */
    bool isPanelLoaded(const String& panelName) const;

    /**
     * @brief Get current app information
     */
    const SecureAppInfo& getCurrentApp() const { return currentApp; }

    /**
     * @brief Get loading statistics
     */
    const LoadingStats& getLoadingStats() const { return currentStats; }

    /**
     * @brief Check if ROM is currently loaded
     */
    bool isROMLoaded() const { return currentApp.validated; }

    // =========================
    // Security Validation
    // =========================

    /**
     * @brief Validate ROM file integrity and structure
     * 
     * @param romData Raw ROM data
     * @param romSize Size of ROM data in bytes
     * @return true if ROM structure is valid and secure
     */
    bool validateROMIntegrity(const uint8_t* romData, size_t romSize);

    /**
     * @brief Validate WASH bytecode for security compliance
     * 
     * @param bytecode Bytecode to validate
     * @param size Size of bytecode in bytes
     * @param permissionLevel Required permission level for bytecode
     * @return true if bytecode is safe and compliant
     */
    bool validateWASHBytecode(const uint8_t* bytecode, size_t size, uint8_t permissionLevel);

    /**
     * @brief Validate script definitions for security compliance
     * 
     * @param scripts Script definitions to validate
     * @return true if all script definitions are secure
     */
    bool validateScriptDefinitions(std::vector<SecureScriptDef>& scripts);

    /**
     * @brief Validate entity intents (ROM cannot specify UUIDs directly)
     * 
     * @param entities Entity intents to validate
     * @param maxEntities Maximum allowed entities
     * @return true if entity intents are valid and secure
     */
    bool validateEntityIntents(const std::vector<EntityIntent>& entities, uint32_t maxEntities);

    // =========================
    // Resource Management
    // =========================

    /**
     * @brief Get memory usage of currently loaded ROM
     */
    uint32_t getCurrentMemoryUsageKB() const;

    /**
     * @brief Check if ROM exceeds resource limits
     */
    bool checkResourceLimits(const SecureAppInfo& appInfo) const;

    /**
     * @brief Cleanup expired bytecode validation cache
     */
    void cleanupValidationCache();

    // =========================
    // Debug and Statistics
    // =========================

    /**
     * @brief Dump current ROM loading state for debugging
     */
    void dumpLoadingState() const;

    /**
     * @brief Get detailed security validation report
     */
    String getSecurityValidationReport() const;

    /**
     * @brief Reset loading statistics
     */
    void resetStats();

    // =========================
    // Adaptive Memory Management
    // =========================

    /**
     * @brief Evaluate available memory and calculate dynamic limits
     * Queries heap, resource quotas, and existing allocations
     * 
     * @param limits Output structure for calculated limits
     * @return true if evaluation successful and memory is adequate
     */
    bool evaluateMemoryLimits(DynamicLimits* limits);

    /**
     * @brief Check if loading a panel would exceed memory constraints
     * 
     * @param panelName Panel to evaluate
     * @param estimatedMemoryKB Estimated memory usage
     * @return true if panel can be loaded safely
     */
    bool canLoadPanel(const String& panelName, uint32_t estimatedMemoryKB);

    /**
     * @brief Trigger asset fallback system for memory constrained loading
     * Reduces image resolution, animation frames, audio quality, etc.
     * 
     * @param panelName Panel requiring fallback assets
     * @param severity Fallback severity (0=minimal, 3=aggressive)
     * @return true if fallback assets configured
     */
    bool configureAssetFallbacks(const String& panelName, uint8_t severity);

    /**
     * @brief Truncate or compress script bytecode if memory is constrained
     * Removes debug info, compacts constants, etc.
     * 
     * @param scriptName Script to optimize
     * @param targetSizeKB Target size after optimization
     * @return true if optimization successful
     */
    bool optimizeScriptForMemory(const String& scriptName, uint32_t targetSizeKB);

    /**
     * @brief Free memory by unloading least critical panels/assets
     * 
     * @param requiredMemoryKB Memory needed to free
     * @return actual memory freed in KB
     */
    uint32_t freeMemoryForLoading(uint32_t requiredMemoryKB);

    /**
     * @brief Get current heap memory status from system
     * 
     * @param totalHeap Output: total heap size in KB
     * @param freeHeap Output: free heap size in KB
     * @param largestBlock Output: largest contiguous block in KB
     * @return true if memory status retrieved successfully
     */
    bool getHeapMemoryStatus(uint32_t* totalHeap, uint32_t* freeHeap, uint32_t* largestBlock);

    // =========================
    // Asset-Specific Validation
    // =========================

    /**
     * @brief Validate entity asset assignment based on script complexity
     * Ensures scripted entities use npc.spr, simple entities use item.spr
     * 
     * @param intent Entity creation intent to validate
     * @return true if asset assignment is appropriate
     */
    bool validateEntityAssetAssignment(const EntityIntent& intent);

    /**
     * @brief Validate UI asset usage for panel elements
     * Ensures UI elements only use light.png/dark.png assets
     * 
     * @param entityType Type of UI entity
     * @param assetPath Requested asset path
     * @return true if UI asset usage is valid
     */
    bool validateUIAssetUsage(const String& entityType, const String& assetPath);

    /**
     * @brief Configure asset fallbacks based on entity types
     * Applies appropriate compression/optimization per asset type
     * 
     * @param panelName Panel requiring asset optimization
     * @param availableMemoryKB Current available memory
     * @return true if asset configuration successful
     */
    bool configureEntityAssetFallbacks(const String& panelName, uint32_t availableMemoryKB);

private:
    // =========================
    // Segmented Loading Integration
    // =========================

    /**
     * @brief Initialize WispSegmentedLoader and validate ROM structure
     */
    bool initializeSegmentedLoader(const String& romPath);

    /**
     * @brief Load and validate panel-specific scripts
     * Only loads scripts associated with the given panel
     */
    bool loadPanelScripts(const String& panelName, uint8_t layoutIndex, uint8_t panelIndex);

    /**
     * @brief Load and validate panel-specific entities
     * Creates entities through UUID authority for the given panel
     */
    bool loadPanelEntities(const String& panelName, uint8_t layoutIndex, uint8_t panelIndex);

    /**
     * @brief Load and validate panel-specific tiles/background
     * Validates tile data and background assets for the panel
     */
    bool loadPanelAssets(const String& panelName, uint8_t layoutIndex, uint8_t panelIndex);

    /**
     * @brief Validate script bytecode on-demand from segmented loader
     */
    bool validateScriptBytecodeFromLoader(const String& scriptName);

    /**
     * @brief Get or allocate panel validation cache slot
     */
    PanelValidationCache* getPanelCache(const String& panelName);

    /**
     * @brief Evict least recently used panel from cache
     */
    void evictLRUPanel();

    // =========================
    // Internal ROM Processing (Updated for Segmented Loading)
    // =========================

    /**
     * @brief Parse ROM config data for app metadata (lightweight)
     */
    bool parseROMConfig(SecureAppInfo* appInfo);

    /**
     * @brief Create panel-specific entities through secure UUID authority
     */
    bool createPanelEntitiesSecure(const String& panelName, uint8_t layoutIndex, uint8_t panelIndex);

    // =========================
    // Security Validation Internals
    // =========================

    /**
     * @brief Calculate and verify ROM checksum
     */
    uint32_t calculateROMChecksum(const uint8_t* romData, size_t size);

    /**
     * @brief Scan bytecode for malicious patterns
     */
    bool scanBytecodeForMaliciousPatterns(const uint8_t* bytecode, size_t size);

    /**
     * @brief Validate bytecode instruction compliance
     */
    bool validateBytecodeInstructions(const uint8_t* bytecode, size_t size);

    /**
     * @brief Check script permission level validity
     */
    bool validateScriptPermissions(const SecureScriptDef& script);

    /**
     * @brief Validate entity type and parameters
     */
    bool validateEntityParameters(const EntityIntent& intent);

    // =========================
    // Error Handling
    // =========================

    /**
     * @brief Record security violation
     */
    void recordSecurityViolation(const String& violation, const String& details);

    /**
     * @brief Handle ROM loading error
     */
    void handleLoadingError(const String& error);

    /**
     * @brief Cleanup partial loading state on error
     */
    void cleanupPartialLoad();

    // =========================
    // Utility Functions
    // =========================

    /**
     * @brief Convert permission level to string for logging
     */
    String permissionLevelToString(uint8_t level) const;

    /**
     * @brief Get human-readable size string
     */
    String formatSizeString(uint32_t sizeBytes) const;

    /**
     * @brief Validate string for security (no control characters, reasonable length)
     */
    bool validateSecureString(const String& str, uint32_t maxLength) const;
};

/**
 * @brief Factory function for creating SecureROMLoader with proper dependencies
 */
SecureROMLoader* createSecureROMLoader(
    EngineUUIDAuthority* uuidAuth,
    ScriptInstanceAuthority* scriptAuth,
    SecureWASHAPIBridge* bridge,
    SceneManager* sceneMgr,
    WispCuratedAPIExtended* api
);
