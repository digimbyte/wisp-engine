// app_loader.h
// app_loader.h - ESP32-C6/S3 App Loader using ESP-IDF
// Application loading system with memory management for ESP32
#pragma once
#include "../../system/esp32_common.h"  // Pure ESP-IDF native headers
// Note: SD.h replaced with ESP-IDF native SPIFFS/SD card support
#include "cJSON.h"  // ESP-IDF native JSON parser
#include "../audio/engine.h"
#include "../graphics/engine.h"  // For complete GraphicsEngine definition
#include "../system/wisp_asset_types.h"
#include "../namespaces.h"  // For WispEngine::Core::Debug
#include <dirent.h>
#include <stdio.h>

// Note: Debug macros are already defined by the core debug system
// No need to redefine them here

// Forward declarations for WISP assets
namespace WispAssets {
    constexpr uint32_t MAGIC_WISP = 0x50534957;  // "WISP" in little endian
}

// Use GraphicsEngine directly from namespace (complete definition in ../graphics/engine.h)
using GraphicsEngine = WispEngine::Graphics::GraphicsEngine;

// ESP-IDF File wrapper for Arduino compatibility
class File {
public:
    FILE* _file;
    bool _isDirectory;
    String _name;
    
    File() : _file(nullptr), _isDirectory(false) {}
    File(FILE* f, const char* name) : _file(f), _isDirectory(false), _name(name) {}
    File(const char* dirName) : _file(nullptr), _isDirectory(true), _name(dirName) {}
    
    operator bool() const { return _file != nullptr || _isDirectory; }
    void close() { if (_file) { fclose(_file); _file = nullptr; } }
    String name() const { return _name; }
    bool isDirectory() const { return _isDirectory; }
    size_t size() { 
        if (!_file) return 0;
        long pos = ftell(_file);
        fseek(_file, 0, SEEK_END);
        size_t sz = ftell(_file);
        fseek(_file, pos, SEEK_SET);
        return sz;
    }
};

// Forward declarations
struct AppConfig;

// App format types
enum AppFormat {
    APP_FORMAT_WISP_BUNDLE,  // WISP bundled application (recommended)
    WISP_FORMAT_CARTRIDGE = APP_FORMAT_WISP_BUNDLE,  // Alias for compatibility
    APP_FORMAT_UNKNOWN
};

// App database entry for fast lookup
struct AppDatabaseEntry {
    char name[64];
    char path[256];
    AppFormat format;
    uint32_t configOffset;   // Offset to config in WISP bundle
    uint32_t configSize;     // Size of config data
    uint32_t memoryRequirement; // Memory requirement in bytes
    bool validated;          // Has been validated for requirements
    AppConfig* cachedConfig;  // Cached configuration data pointer
};

// App configuration structure
struct AppConfig {
    char name[64];
    char version[16];
    char author[64];
    char description[256];
    
    // Audio requirements
    uint8_t requiredAudioOutputs = AUDIO_PIEZO;  // Bitmask
    uint32_t preferredSampleRate = 22050;
    uint8_t requiredChannels = 4;
    bool needsStreamingAudio = false;
    bool needsAudioEffects = false;
    bool needsAudioRecording = false;
    
    // Performance requirements
    uint8_t targetFPS = 16;
    uint32_t requiredRAM = 32768;  // bytes
    uint32_t requiredStorage = 0;  // bytes (0 = streaming from SD)
    
    // System requirements
    bool needsWiFi = false;
    bool needsBluetooth = false;
    bool needsEEPROM = false;
    
    // Entry points
    char mainBinary[64] = "main.wash";     // Compiled C++ binary
    char configData[64] = "config.yaml";  // Configuration data (YAML format)
};

// App loading results
enum AppLoadResult {
    APP_LOAD_SUCCESS,
    APP_LOAD_FILE_NOT_FOUND,
    APP_LOAD_INVALID_CONFIG,
    APP_LOAD_INSUFFICIENT_MEMORY,
    APP_LOAD_MISSING_REQUIREMENTS,
    APP_LOAD_AUDIO_INIT_FAILED
};

namespace WispEngine {
    namespace App {
        class Loader {
        public:
            static const int MAX_APPS = 32;
            AppDatabaseEntry appDatabase[MAX_APPS];
            int appCount;
            int numAppEntries;  // Track number of entries
            AppConfig currentAppConfig;
            char currentAppPath[256];
            AppFormat currentAppFormat;
            bool appLoaded = false;
            
            // Graphics and timing for LUT animations
            GraphicsEngine* graphics = nullptr;
            uint32_t lastFrameTick = 0;
    
            // Build app database by scanning SD card and reading headers
            void buildAppDatabase() {
                appCount = 0;  // Reset count instead of clear()
                numAppEntries = 0;
        
                // Skip SD card initialization for now - use SPIFFS instead
                WispEngine::Core::Debug::info("LOADER", "Building app database from SPIFFS");
        
        
        // PERFORMANCE OPTIMIZATION: Only scan WISP bundles
        // Single-file format for optimal performance:
        // - Single file operation per app
        // - Efficient binary format
        // - Fast directory traversals
        // - Optimized asset access
        
        // Scan for WISP bundle apps (optimized single-file format)
        scanWispApps();
        
        WispEngine::Core::Debug::info("LOADER", "App database built - WISP apps found");
    }
    
    // Get list of app names for menu display
    void getAppNames(char appNames[][64], uint8_t* numNames, uint8_t maxNames) {
        *numNames = 0;
        for (uint8_t i = 0; i < numAppEntries && *numNames < maxNames; i++) {
            strncpy(appNames[*numNames], appDatabase[i].name, 63);
            appNames[*numNames][63] = '\0';
            (*numNames)++;
        }
    }
    
    // Load app by name from database
    AppLoadResult loadApp(const String& appName) {
        // Find app in database
        AppDatabaseEntry* entry = nullptr;
        for (int i = 0; i < appCount; i++) {
            if (strcmp(appDatabase[i].name, appName.c_str()) == 0) {
                entry = &appDatabase[i];
                break;
            }
        }
        
        if (!entry) {
            return APP_LOAD_FILE_NOT_FOUND;
        }
        
        // Load configuration based on format
        AppLoadResult result = loadAppConfigFromEntry(*entry);
        if (result != APP_LOAD_SUCCESS) return result;
        
        result = validateRequirements();
        if (result != APP_LOAD_SUCCESS) return result;
        
        result = configureAudioForApp();
        if (result != APP_LOAD_SUCCESS) return result;
        
        return executeApp(*entry);
    }
    
    void scanWispApps() {
        // Skip SD card for now - use placeholder data
        WispEngine::Core::Debug::info("LOADER", "Scanning for WISP apps");
        
        // For now, add a dummy app entry to test the system
        if (appCount < MAX_APPS) {
            strcpy(appDatabase[appCount].name, "test_app");
            strcpy(appDatabase[appCount].path, "/spiffs/test_app.wisp");
            appDatabase[appCount].format = WISP_FORMAT_CARTRIDGE;
            appDatabase[appCount].memoryRequirement = 32768;
            appCount++;
            numAppEntries++;
        }
    }
    
    bool loadWispHeader(const String& filePath, AppDatabaseEntry& entry) {
        // Simplified implementation for now - just return basic info
        strncpy(entry.name, "test_app", sizeof(entry.name) - 1);
        entry.name[sizeof(entry.name) - 1] = '\0';
        strncpy(entry.path, filePath.c_str(), sizeof(entry.path) - 1);
        entry.path[sizeof(entry.path) - 1] = '\0';
        entry.format = WISP_FORMAT_CARTRIDGE;
        entry.memoryRequirement = 32768;
        entry.configOffset = 0;
        entry.configSize = 0;
        entry.validated = false;
        entry.cachedConfig = nullptr;
        return true;
    }
            
            // Try to load embedded config to get app name
    
    bool loadWispConfig(File& wispFile, AppDatabaseEntry& entry) {
        // Simplified implementation - just return success for now
        return true;
    }
    
    AppLoadResult loadAppConfigFromEntry(const AppDatabaseEntry& entry) {
        // Copy config if available
        if (entry.cachedConfig) {
            currentAppConfig = *entry.cachedConfig;
        } else {
            // Set default config
            strncpy(currentAppConfig.name, entry.name, sizeof(currentAppConfig.name) - 1);
            currentAppConfig.name[sizeof(currentAppConfig.name) - 1] = '\0';
        }
        
        // Copy path
        strncpy(currentAppPath, entry.path, sizeof(currentAppPath) - 1);
        currentAppPath[sizeof(currentAppPath) - 1] = '\0';
        
        currentAppFormat = entry.format;
        
        // If it's a WISP bundle, we need to load it for asset access
        if (entry.format == APP_FORMAT_WISP_BUNDLE) {
            return loadWispForExecution(entry.path);
        }
        
        return APP_LOAD_SUCCESS;
    }
    
    AppLoadResult loadWispForExecution(const String& filePath) {
        // Load the WISP bundle structure for runtime access
        WispEngine::Core::Debug::info("LOADER", "Loading WISP bundle");
        
        // TODO: Implement full WISP loading for runtime asset access
        // This would parse the entire bundle and prepare assets for use
        
        return APP_LOAD_SUCCESS;
    }
    
    AppLoadResult executeApp(const AppDatabaseEntry& entry) {
        WispEngine::Core::Debug::info("LOADER", "Executing app");
        
        // PERFORMANCE OPTIMIZATION: Only support WISP format
        // Single file execution for optimal performance
        
        if (entry.format == APP_FORMAT_WISP_BUNDLE) {
            // Execute from WISP bundle
            WispEngine::Core::Debug::info("LOADER", "Executing from WISP bundle");
            
            // TODO: Find and execute main binary (.wash) from WISP bundle
            // This would:
            // 1. Locate main.wash in the bundle
            // 2. Load required assets (.wlut, .art, .sfx)
            // 3. Initialize C++ application
            // 4. Transfer control to main() function
            
            appLoaded = true;
            return APP_LOAD_SUCCESS;
            
        } else {
            // Unsupported format
            WispEngine::Core::Debug::error("LOADER", "Unsupported app format");
            return APP_LOAD_INVALID_CONFIG;
        }
    }
    
    // Simple YAML parser for basic key-value configuration
    String getYamlValue(const String& yamlData, const String& key) {
        String searchKey = key + ":";
        int keyIndex = yamlData.indexOf(searchKey);
        if (keyIndex == -1) return "";
        
        int valueStart = keyIndex + searchKey.length();
        // Skip whitespace and quotes
        while (valueStart < yamlData.length() && 
               (yamlData.charAt(valueStart) == ' ' || yamlData.charAt(valueStart) == '\t' || yamlData.charAt(valueStart) == '"')) {
            valueStart++;
        }
        
        int valueEnd = yamlData.indexOf('\n', valueStart);
        if (valueEnd == -1) valueEnd = yamlData.length();
        
        String value = yamlData.substring(valueStart, valueEnd);
        value.trim();
        // Remove trailing quotes and comments
        if (value.endsWith("\"")) value = value.substring(0, value.length() - 1);
        int commentIndex = value.indexOf('#');
        if (commentIndex != -1) value = value.substring(0, commentIndex);
        value.trim();
        
        return value;
    }
    
    // Get YAML nested value (e.g., "audio.sampleRate")
    String getNestedYamlValue(const String& yamlData, const String& section, const String& key) {
        String sectionHeader = section + ":";
        int sectionIndex = yamlData.indexOf(sectionHeader);
        if (sectionIndex == -1) return "";
        
        // Find the next section or end of file
        int nextSectionIndex = yamlData.length();
        int searchFrom = sectionIndex + sectionHeader.length();
        
        for (int i = searchFrom; i < yamlData.length(); i++) {
            if (yamlData.charAt(i) == '\n' && i + 1 < yamlData.length() && yamlData.charAt(i + 1) != ' ' && yamlData.charAt(i + 1) != '\t') {
                // Found a line that starts without indentation (new section)
                nextSectionIndex = i;
                break;
            }
        }
        
        String sectionData = yamlData.substring(sectionIndex, nextSectionIndex);
        return getYamlValue(sectionData, String("  ") + key);  // Account for indentation
    }
    
    bool parseYamlConfig(const String& yamlData, AppConfig& config) {
        // Extract app metadata
        String nameStr = getYamlValue(yamlData, "name");
        strncpy(config.name, nameStr.isEmpty() ? "Unknown App" : nameStr.c_str(), sizeof(config.name) - 1);
        config.name[sizeof(config.name) - 1] = '\0';
        
        String versionStr = getYamlValue(yamlData, "version");
        strncpy(config.version, versionStr.isEmpty() ? "1.0.0" : versionStr.c_str(), sizeof(config.version) - 1);
        config.version[sizeof(config.version) - 1] = '\0';
        
        String authorStr = getYamlValue(yamlData, "author");
        strncpy(config.author, authorStr.isEmpty() ? "Unknown" : authorStr.c_str(), sizeof(config.author) - 1);
        config.author[sizeof(config.author) - 1] = '\0';
        
        String descStr = getYamlValue(yamlData, "description");
        strncpy(config.description, descStr.c_str(), sizeof(config.description) - 1);
        config.description[sizeof(config.description) - 1] = '\0';
        
        // Extract audio requirements with hardware validation
        String sampleRateStr = getNestedYamlValue(yamlData, "audio", "sampleRate");
        uint32_t sampleRate = sampleRateStr.isEmpty() ? 22050 : sampleRateStr.toInt();
        // Validate sample rate against hardware capabilities
        if (sampleRate != 8000 && sampleRate != 11025 && sampleRate != 16000 && 
            sampleRate != 22050 && sampleRate != 44100) {
            sampleRate = 22050; // Default to safe value
        }
        config.preferredSampleRate = sampleRate;
        
        String channelsStr = getNestedYamlValue(yamlData, "audio", "channels");
        uint8_t channels = channelsStr.isEmpty() ? 4 : channelsStr.toInt();
        // Validate audio channels (1-16 based on audio_engine.h)
        if (channels < 1) channels = 1;
        else if (channels > 16) channels = 16;
        config.requiredChannels = channels;
        
        // Audio outputs - simplified for now, default to piezo
        config.requiredAudioOutputs = AUDIO_PIEZO; // Always available
        
        // Extract performance requirements with ESP32-C6 validation
        String fpsStr = getNestedYamlValue(yamlData, "performance", "fps");
        uint8_t fps = fpsStr.isEmpty() ? 16 : fpsStr.toInt();
        // Validate app FPS against allowed rates (8, 10, 12, 14, 16) - system runs at 24 FPS
        if (fps > 16) fps = 16;
        else if (fps > 14) fps = 16;  // Round up to 16
        else if (fps > 12) fps = 14;  // Round up to 14
        else if (fps > 10) fps = 12;  // Round up to 12
        else if (fps > 8) fps = 10;   // Round up to 10
        else fps = 8;                 // Minimum app rate
        config.targetFPS = fps;
        
        String ramStr = getNestedYamlValue(yamlData, "performance", "ram");
        uint32_t ram = ramStr.isEmpty() ? 131072 : ramStr.toInt(); // Default 128KB
        // Validate RAM against ESP32-C6 available memory (32KB-384KB for apps)
        if (ram < 32768) ram = 32768;
        else if (ram > 393216) ram = 393216;
        config.requiredRAM = ram;
        
        String storageStr = getNestedYamlValue(yamlData, "performance", "storage");
        config.requiredStorage = storageStr.isEmpty() ? 0 : storageStr.toInt();
        // Validate storage against flash capacity (0-4MB)
        if (config.requiredStorage > 4194304) config.requiredStorage = 4194304;
        
        // Extract system requirements with hardware validation
        String wifiStr = getNestedYamlValue(yamlData, "system", "wifi");
        config.needsWiFi = (wifiStr == String("true"));
        
        String bluetoothStr = getNestedYamlValue(yamlData, "system", "bluetooth");
        config.needsBluetooth = (bluetoothStr == String("true"));
        
        String eepromStr = getNestedYamlValue(yamlData, "system", "eeprom");
        config.needsEEPROM = (eepromStr == String("true"));
        
        // Extract entry points
        String mainBinaryStr = getNestedYamlValue(yamlData, "entry", "main");
        strncpy(config.mainBinary, mainBinaryStr.isEmpty() ? "main.wash" : mainBinaryStr.c_str(), sizeof(config.mainBinary) - 1);
        config.mainBinary[sizeof(config.mainBinary) - 1] = '\0';
        
        String configDataStr = getNestedYamlValue(yamlData, "entry", "config");
        strncpy(config.configData, configDataStr.isEmpty() ? "config.yaml" : configDataStr.c_str(), sizeof(config.configData) - 1);
        config.configData[sizeof(config.configData) - 1] = '\0';
        
        return true;
    }
    
    // Check if current system can run the app
    AppLoadResult validateRequirements() {
        // Check memory requirements
        if (esp_get_free_heap_size() < currentAppConfig.requiredRAM) {
            return APP_LOAD_INSUFFICIENT_MEMORY;
        }
        
        // Check audio capabilities
        extern AudioEngine audio;
        uint8_t availableOutputs = audio.getAvailableOutputs();
        if ((currentAppConfig.requiredAudioOutputs & availableOutputs) != currentAppConfig.requiredAudioOutputs) {
            return APP_LOAD_MISSING_REQUIREMENTS;
        }
        
        return APP_LOAD_SUCCESS;
    }
    
    // Configure audio system based on app requirements
    AppLoadResult configureAudioForApp() {
        extern AudioEngine audio;
        
        // Initialize audio with app's requirements
        audio.init(currentAppConfig.requiredAudioOutputs, currentAppConfig.preferredSampleRate);
        
        // Verify initialization succeeded
        if (!audio.enabled) {
            return APP_LOAD_AUDIO_INIT_FAILED;
        }
        
        return APP_LOAD_SUCCESS;
    }
    
    // Get error message for display
    String getErrorMessage(AppLoadResult result) {
        switch (result) {
            case APP_LOAD_SUCCESS: return "Success";
            case APP_LOAD_FILE_NOT_FOUND: return "App files not found";
            case APP_LOAD_INVALID_CONFIG: return "Invalid configuration";
            case APP_LOAD_INSUFFICIENT_MEMORY: return "Not enough memory";
            case APP_LOAD_MISSING_REQUIREMENTS: return "Missing hardware features";
            case APP_LOAD_AUDIO_INIT_FAILED: return "Audio initialization failed";
            default: return "Unknown error";
        }
    }

private:
    uint8_t parseAudioOutputs(cJSON* outputs) {
        uint8_t result = 0;
        
        if (cJSON_IsArray(outputs)) {
            cJSON* output = NULL;
            cJSON_ArrayForEach(output, outputs) {
                if (cJSON_IsString(output)) {
                    const char* outputStr = cJSON_GetStringValue(output);
                    if (strcmp(outputStr, "piezo") == 0) result |= AUDIO_PIEZO;
                    else if (strcmp(outputStr, "i2s") == 0) result |= AUDIO_I2S_DAC;
                    else if (strcmp(outputStr, "bluetooth") == 0) result |= AUDIO_BLUETOOTH;
                    else if (strcmp(outputStr, "pwm") == 0) result |= AUDIO_PWM;
                    else if (strcmp(outputStr, "dac") == 0) result |= AUDIO_INTERNAL_DAC;
                }
            }
        } else if (cJSON_IsString(outputs)) {
            const char* outputStr = cJSON_GetStringValue(outputs);
            if (strcmp(outputStr, "all") == 0) result = AUDIO_ALL;
            else if (strcmp(outputStr, "piezo") == 0) result = AUDIO_PIEZO;
            else if (strcmp(outputStr, "i2s") == 0) result = AUDIO_I2S_DAC;
            // ... etc
        }
        
        return result == 0 ? static_cast<uint8_t>(AUDIO_PIEZO) : result; // Default to piezo
    }
    
    // Load LUT/palette assets from bundle
    bool loadLUTAssets() {
        WispEngine::Core::Debug::info("LOADER", "Loading LUT assets...");
        
        // For now, use the existing LUT data
        // In a full implementation, this would:
        // 1. Search for .wlut files in the WISP bundle
        // 2. Load and parse the LUT data
        // 3. Configure enhanced LUT system with dynamic slots
        
        if (graphics && graphics->isUsingEnhancedLUT()) {
            // Set up some default dynamic effects for demo
            graphics->setupLUTPulseEffect(0, 0xF800, 8);     // Red pulse on slot 0
            graphics->setupLUTFlashEffect(1, 0x001F, 0x07FF, 2); // Blue flash on slot 1
            // Slots 2 and 3 remain transparent (disabled)
            
            WispEngine::Core::Debug::info("LOADER", "Enhanced LUT configured with default effects");
            return true;
        }
        
        return false;
    }
    
    // Update LUT animations based on app frame tick
    void updateLUTAnimations() {
        if (graphics && graphics->isUsingEnhancedLUT()) {
            graphics->updateLUTForFrame(lastFrameTick);
        }
    }
        };  // class Loader
    }  // namespace App
}  // namespace WispEngine
