// app_loader.h
// app_loader.h - ESP32-C6/S3 App Loader using ESP-IDF
// Application loading system with memory management for ESP32
#pragma once
#include "../../system/esp32_common.h"  // Pure ESP-IDF native headers
#include <SD.h>
#include <ArduinoJson.h>
#include "audio_engine.h"
#include "../system/wisp_asset_types.h"

// App format types
enum AppFormat {
    APP_FORMAT_WISP_BUNDLE,  // WISP bundled application (recommended)
    APP_FORMAT_UNKNOWN
};

// App database entry for fast lookup
struct AppDatabaseEntry {
    String name;
    String path;
    AppFormat format;
    uint32_t configOffset;   // Offset to config in WISP bundle
    uint32_t configSize;     // Size of config data
    bool validated;          // Has been validated for requirements
    AppConfig cachedConfig;  // Cached configuration data
};

// App configuration structure
struct AppConfig {
    String name;
    String version;
    String author;
    String description;
    
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
    String mainBinary = "main.wash";     // Compiled C++ binary
    String configData = "config.yaml";  // Configuration data (YAML format)
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

class AppLoader {
public:
    std::vector<AppDatabaseEntry> appDatabase;
    AppConfig currentAppConfig;
    String currentAppPath;
    AppFormat currentAppFormat;
    bool appLoaded = false;
    
    // Build app database by scanning SD card and reading headers
    void buildAppDatabase() {
        appDatabase.clear();
        
        if (!SD.begin()) {
            Serial.println("SD card initialization failed");
            return;
        }
        
        // PERFORMANCE OPTIMIZATION: Only scan WISP bundles
        // Single-file format for optimal performance:
        // - Single file operation per app
        // - Efficient binary format
        // - Fast directory traversals
        // - Optimized asset access
        
        // Scan for WISP bundle apps (optimized single-file format)
        scanWispApps();
        
        Serial.print("App database built: ");
        Serial.print(appDatabase.size());
        Serial.println(" WISP apps found");
    }
    
    // Get list of app names for menu display
    std::vector<String> getAppNames() {
        std::vector<String> names;
        for (const auto& entry : appDatabase) {
            names.push_back(entry.name);
        }
        return names;
    }
    
    // Load app by name from database
    AppLoadResult loadApp(const String& appName) {
        // Find app in database
        AppDatabaseEntry* entry = nullptr;
        for (auto& dbEntry : appDatabase) {
            if (dbEntry.name == appName) {
                entry = &dbEntry;
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
        File appsDir = SD.open("/apps");
        if (!appsDir || !appsDir.isDirectory()) {
            return;
        }
        
        File entry = appsDir.openNextFile();
        while (entry) {
            if (!entry.isDirectory()) {
                String fileName = entry.name();
                if (fileName.endsWith(".wisp")) {
                    String fullPath = "/apps/" + fileName;
                    
                    AppDatabaseEntry dbEntry;
                    if (loadWispHeader(fullPath, dbEntry)) {
                        appDatabase.push_back(dbEntry);
                    }
                }
            }
            entry = appsDir.openNextFile();
        }
        appsDir.close();
    }
    
    bool loadWispHeader(const String& filePath, AppDatabaseEntry& entry) {
        File wispFile = SD.open(filePath);
        if (!wispFile) return false;
        
        // Read WISP header (12 bytes: magic + version + entry_count + config_size)
        uint32_t magic;
        uint16_t version;
        uint16_t entryCount;
        uint32_t configSize;
        
        if (wispFile.read((uint8_t*)&magic, 4) != 4 || magic != WispAssets::MAGIC_WISP) {
            wispFile.close();
            return false;
        }
        
        if (wispFile.read((uint8_t*)&version, 2) != 2 ||
            wispFile.read((uint8_t*)&entryCount, 2) != 2 ||
            wispFile.read((uint8_t*)&configSize, 4) != 4) {
            wispFile.close();
            return false;
        }
        
        // If config is embedded, read it directly (modern ROM format)
        if (configSize > 0) {
            entry.path = filePath;
            entry.format = APP_FORMAT_WISP_BUNDLE;
            entry.configOffset = 12; // Right after header
            entry.configSize = configSize;
            entry.validated = false;
            
            // Try to load embedded config to get app name
            if (loadWispConfig(wispFile, entry)) {
                wispFile.close();
                return true;
            }
        }
        
        // Fallback: Legacy format - look for config in entry table
        for (uint16_t i = 0; i < entryCount; i++) {
            // Seek to entry table start (after header + embedded config)
            uint32_t entryTableStart = 12 + configSize;
            wispFile.seek(entryTableStart + (i * 48));
            
            // Read WISP entry (48 bytes total)
            char name[32];
            uint32_t offset;
            uint32_t size;
            uint8_t type;
            uint8_t flags;
            uint8_t entryReserved[6];
            
            if (wispFile.read((uint8_t*)name, 32) != 32 ||
                wispFile.read((uint8_t*)&offset, 4) != 4 ||
                wispFile.read((uint8_t*)&size, 4) != 4 ||
                wispFile.read((uint8_t*)&type, 1) != 1 ||
                wispFile.read((uint8_t*)&flags, 1) != 1 ||
                wispFile.read((uint8_t*)entryReserved, 6) != 6) {
                wispFile.close();
                return false;
            }
            
            String entryName = String(name);
            if (entryName == "config.yaml" || entryName == "config.yml") {
                // Found config entry (legacy format)
                entry.path = filePath;
                entry.format = APP_FORMAT_WISP_BUNDLE;
                entry.configOffset = entryTableStart + (entryCount * 48) + offset; // Header + config + entry table + data offset
                entry.configSize = size;
                entry.validated = false;
                
                // Try to load config to get app name
                if (loadWispConfig(wispFile, entry)) {
                    wispFile.close();
                    return true;
                }
                break;
            }
        }
        
        wispFile.close();
        return false;
    }
    
    bool loadWispConfig(File& wispFile, AppDatabaseEntry& entry) {
        // Seek to config data
        if (!wispFile.seek(entry.configOffset)) return false;
        
        // Read config data
        String configData;
        configData.reserve(entry.configSize);
        
        for (uint32_t i = 0; i < entry.configSize; i++) {
            int c = wispFile.read();
            if (c == -1) return false;
            configData += (char)c;
        }
        
        // Parse YAML configuration
        if (!parseYamlConfig(configData, entry.cachedConfig)) return false;
        
        entry.name = entry.cachedConfig.name;
        entry.validated = true;
        
        return true;
    }
    
    AppLoadResult loadAppConfigFromEntry(const AppDatabaseEntry& entry) {
        currentAppConfig = entry.cachedConfig;
        currentAppPath = entry.path;
        currentAppFormat = entry.format;
        
        // If it's a WISP bundle, we need to load it for asset access
        if (entry.format == APP_FORMAT_WISP_BUNDLE) {
            return loadWispForExecution(entry.path);
        }
        
        return APP_LOAD_SUCCESS;
    }
    
    AppLoadResult loadWispForExecution(const String& filePath) {
        // Load the WISP bundle structure for runtime access
        Serial.println("Loading WISP bundle: " + filePath);
        
        // TODO: Implement full WISP loading for runtime asset access
        // This would parse the entire bundle and prepare assets for use
        
        return APP_LOAD_SUCCESS;
    }
    
    AppLoadResult executeApp(const AppDatabaseEntry& entry) {
        Serial.println("Executing app: " + entry.name);
        
        // PERFORMANCE OPTIMIZATION: Only support WISP format
        // Single file execution for optimal performance
        
        if (entry.format == APP_FORMAT_WISP_BUNDLE) {
            // Execute from WISP bundle
            Serial.println("Executing from WISP bundle");
            
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
            Serial.println("ERROR: Unsupported app format");
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
        return getYamlValue(sectionData, "  " + key);  // Account for indentation
    }
    
    bool parseYamlConfig(const String& yamlData, AppConfig& config) {
        // Extract app metadata
        config.name = getYamlValue(yamlData, "name");
        if (config.name.isEmpty()) config.name = "Unknown App";
        
        config.version = getYamlValue(yamlData, "version");
        if (config.version.isEmpty()) config.version = "1.0.0";
        
        config.author = getYamlValue(yamlData, "author");
        if (config.author.isEmpty()) config.author = "Unknown";
        
        config.description = getYamlValue(yamlData, "description");
        
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
        config.requiredChannels = constrain(channels, 1, 16);
        
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
        config.requiredRAM = constrain(ram, 32768, 393216);
        
        String storageStr = getNestedYamlValue(yamlData, "performance", "storage");
        config.requiredStorage = storageStr.isEmpty() ? 0 : storageStr.toInt();
        // Validate storage against flash capacity (0-4MB)
        config.requiredStorage = constrain(config.requiredStorage, 0, 4194304);
        
        // Extract system requirements with hardware validation
        String wifiStr = getNestedYamlValue(yamlData, "system", "wifi");
        config.needsWiFi = (wifiStr == "true");
        
        String bluetoothStr = getNestedYamlValue(yamlData, "system", "bluetooth");
        config.needsBluetooth = (bluetoothStr == "true");
        
        String eepromStr = getNestedYamlValue(yamlData, "system", "eeprom");
        config.needsEEPROM = (eepromStr == "true");
        
        // Extract entry points
        config.mainBinary = getNestedYamlValue(yamlData, "entry", "main");
        if (config.mainBinary.isEmpty()) config.mainBinary = "main.wash";
        
        config.configData = getNestedYamlValue(yamlData, "entry", "config");
        if (config.configData.isEmpty()) config.configData = "config.yaml";
        
        return true;
    }
    
    // Check if current system can run the app
    AppLoadResult validateRequirements() {
        // Check memory requirements
        if (ESP.getFreeHeap() < currentAppConfig.requiredRAM) {
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
    uint8_t parseAudioOutputs(JsonVariant outputs) {
        uint8_t result = 0;
        
        if (outputs.is<JsonArray>()) {
            for (JsonVariant output : outputs.as<JsonArray>()) {
                String outputStr = output.as<String>();
                if (outputStr == "piezo") result |= AUDIO_PIEZO;
                else if (outputStr == "i2s") result |= AUDIO_I2S_DAC;
                else if (outputStr == "bluetooth") result |= AUDIO_BLUETOOTH;
                else if (outputStr == "pwm") result |= AUDIO_PWM;
                else if (outputStr == "dac") result |= AUDIO_INTERNAL_DAC;
            }
        } else if (outputs.is<String>()) {
            String outputStr = outputs.as<String>();
            if (outputStr == "all") result = AUDIO_ALL;
            else if (outputStr == "piezo") result = AUDIO_PIEZO;
            else if (outputStr == "i2s") result = AUDIO_I2S_DAC;
            // ... etc
        }
        
        return result == 0 ? AUDIO_PIEZO : result; // Default to piezo
    }
    
    // Load LUT/palette assets from bundle
    bool loadLUTAssets() {
        Serial.println("Loading LUT assets...");
        
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
            
            Serial.println("Enhanced LUT configured with default effects");
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
};
};
