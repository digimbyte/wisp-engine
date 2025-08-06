// app_header.h - ESP32-C6/S3 App Header System using ESP-IDF
// Application metadata and configuration system for ESP32 microcontroller
#pragma once
#include "esp32_common.h"  // Pure ESP-IDF native headers

// App frame rate options
enum AppFrameRate : uint8_t {
    FRAMERATE_8FPS = 8,
    FRAMERATE_10FPS = 10,
    FRAMERATE_12FPS = 12,
    FRAMERATE_15FPS = 15,
    FRAMERATE_20FPS = 20,
    FRAMERATE_24FPS = 24,
    FRAMERATE_30FPS = 30,
    FRAMERATE_60FPS = 60
};

// App performance profile
enum AppPerformanceProfile : uint8_t {
    PROFILE_MINIMAL = 0,     // 8-12 FPS, minimal features
    PROFILE_STANDARD = 1,    // 15-20 FPS, standard features  
    PROFILE_SMOOTH = 2,      // 24-30 FPS, enhanced features
    PROFILE_MAXIMUM = 3      // 60 FPS, all features enabled
};

// App resource requirements
struct AppResourceRequirements {
    uint32_t maxMemoryKB;        // Maximum memory usage in KB
    uint16_t maxEntities;        // Maximum game entities
    uint8_t maxRegions;          // Maximum physics regions
    uint8_t audioChannels;       // Required audio channels
    bool requiresDepthBuffer;    // Needs depth testing
    bool requiresLUT;           // Needs color LUT
};

// App header structure (stored in app metadata)
struct AppHeader {
    // App identification
    char name[32];              // App name
    char version[16];           // Version string
    char author[32];            // Author name
    
    // Performance requirements
    AppFrameRate targetFrameRate;
    AppFrameRate minimumFrameRate;
    AppPerformanceProfile performanceProfile;
    
    // Resource requirements
    AppResourceRequirements resources;
    
    // Engine requirements
    uint8_t requiredEngineVersion[3]; // Major, minor, patch
    uint32_t features;          // Feature flags (bitfield)
    
    // Runtime configuration
    bool allowFrameRateScaling; // Can engine reduce frame rate if needed
    bool allowFeatureDisabling; // Can engine disable features for performance
    uint8_t priority;           // App priority (0-255, higher = more important)
    
    // Validation
    uint32_t headerCRC;         // CRC32 of header for validation
};

// App feature flags
namespace AppFeatures {
    constexpr uint32_t GRAPHICS_2D          = 0x00000001;
    constexpr uint32_t GRAPHICS_SPRITES     = 0x00000002;
    constexpr uint32_t GRAPHICS_DEPTH       = 0x00000004;
    constexpr uint32_t GRAPHICS_PARTICLES   = 0x00000008;
    constexpr uint32_t AUDIO_BASIC          = 0x00000010;
    constexpr uint32_t AUDIO_MULTICHANNEL   = 0x00000020;
    constexpr uint32_t AUDIO_EFFECTS        = 0x00000040;
    constexpr uint32_t PHYSICS_BASIC        = 0x00000080;
    constexpr uint32_t PHYSICS_COLLISION    = 0x00000100;
    constexpr uint32_t PHYSICS_TRIGGERS     = 0x00000200;
    constexpr uint32_t INPUT_BUTTONS        = 0x00000400;
    constexpr uint32_t INPUT_ANALOG         = 0x00000800;
    constexpr uint32_t STORAGE_READ         = 0x00001000;
    constexpr uint32_t STORAGE_WRITE        = 0x00002000;
    constexpr uint32_t NETWORK_BASIC        = 0x00004000;
    constexpr uint32_t SCRIPTING_NATIVE     = 0x00008000;
}

// Helper functions for app header
class AppHeaderUtils {
public:
    // Create default app header
    static AppHeader createDefault(const char* appName) {
        AppHeader header = {};
        
        strncpy(header.name, appName, sizeof(header.name) - 1);
        strncpy(header.version, "1.0.0", sizeof(header.version) - 1);
        strncpy(header.author, "Unknown", sizeof(header.author) - 1);
        
        header.targetFrameRate = FRAMERATE_24FPS;
        header.minimumFrameRate = FRAMERATE_12FPS;
        header.performanceProfile = PROFILE_STANDARD;
        
        header.resources.maxMemoryKB = 64;
        header.resources.maxEntities = 64;
        header.resources.maxRegions = 16;
        header.resources.audioChannels = 4;
        header.resources.requiresDepthBuffer = true;
        header.resources.requiresLUT = true;
        
        header.requiredEngineVersion[0] = 1; // Major
        header.requiredEngineVersion[1] = 0; // Minor  
        header.requiredEngineVersion[2] = 0; // Patch
        
        header.features = AppFeatures::GRAPHICS_2D | 
                         AppFeatures::GRAPHICS_SPRITES |
                         AppFeatures::AUDIO_BASIC |
                         AppFeatures::PHYSICS_BASIC |
                         AppFeatures::INPUT_BUTTONS |
                         AppFeatures::SCRIPTING_NATIVE;
        
        header.allowFrameRateScaling = true;
        header.allowFeatureDisabling = false;
        header.priority = 128; // Normal priority
        
        header.headerCRC = calculateCRC(header);
        return header;
    }
    
    // Validate app header
    static bool validate(const AppHeader& header) {
        // Check CRC
        AppHeader tempHeader = header;
        uint32_t storedCRC = tempHeader.headerCRC;
        tempHeader.headerCRC = 0;
        uint32_t calculatedCRC = calculateCRC(tempHeader);
        
        if (storedCRC != calculatedCRC) {
            ESP_LOGE("WISP", "App header CRC mismatch");
            return false;
        }
        
        // Validate frame rates
        if (header.targetFrameRate < header.minimumFrameRate) {
            ESP_LOGE("WISP", "Invalid frame rate configuration");
            return false;
        }
        
        // Validate memory requirements
        if (header.resources.maxMemoryKB > 512) { // 512KB max
            ESP_LOGE("APP_HEADER", "Memory requirements too high");
            return false;
        }
        
        return true;
    }
    
    // Get frame time in microseconds
    static uint32_t getFrameTimeUs(AppFrameRate frameRate) {
        return 1000000 / static_cast<uint32_t>(frameRate);
    }
    
    // Get frame time in milliseconds
    static uint32_t getFrameTimeMs(AppFrameRate frameRate) {
        return 1000 / static_cast<uint32_t>(frameRate);
    }
    
    // Check if app is compatible with engine
    static bool isCompatible(const AppHeader& header) {
        // Check engine version compatibility
        // For now, just check major version
        const uint8_t ENGINE_VERSION_MAJOR = 1;
        if (header.requiredEngineVersion[0] > ENGINE_VERSION_MAJOR) {
            return false;
        }
        
        // Check available features (simplified)
        uint32_t availableFeatures = AppFeatures::GRAPHICS_2D |
                                    AppFeatures::GRAPHICS_SPRITES |
                                    AppFeatures::GRAPHICS_DEPTH |
                                    AppFeatures::AUDIO_BASIC |
                                    AppFeatures::PHYSICS_BASIC |
                                    AppFeatures::PHYSICS_COLLISION |
                                    AppFeatures::PHYSICS_TRIGGERS |
                                    AppFeatures::INPUT_BUTTONS |
                                    AppFeatures::SCRIPTING_NATIVE;
        
        return (header.features & availableFeatures) == header.features;
    }
    
    // Print app header info
    static void printInfo(const AppHeader& header) {
        ESP_LOGI("APP_HEADER", "=== App Header ===");
        ESP_LOGI("APP_HEADER", "Name: %s", header.name);
        ESP_LOGI("APP_HEADER", "Version: %s", header.version);
        ESP_LOGI("APP_HEADER", "Author: %s", header.author);
        ESP_LOGI("APP_HEADER", "Target FPS: %d", header.targetFrameRate);
        ESP_LOGI("APP_HEADER", "Min FPS: %d", header.minimumFrameRate);
        ESP_LOGI("APP_HEADER", "Memory: %lu KB", (unsigned long)header.resources.maxMemoryKB);
        ESP_LOGI("APP_HEADER", "Entities: %d", header.resources.maxEntities);
        ESP_LOGI("APP_HEADER", "Features: 0x%lX", (unsigned long)header.features);
        ESP_LOGI("APP_HEADER", "Compatible: %s", isCompatible(header) ? "Yes" : "No");
    }
    
private:
    // Simple CRC32 calculation
    static uint32_t calculateCRC(const AppHeader& header) {
        const uint8_t* data = reinterpret_cast<const uint8_t*>(&header);
        size_t size = sizeof(AppHeader) - sizeof(uint32_t); // Exclude CRC field
        
        uint32_t crc = 0xFFFFFFFF;
        for (size_t i = 0; i < size; i++) {
            crc ^= data[i];
            for (int j = 0; j < 8; j++) {
                if (crc & 1) {
                    crc = (crc >> 1) ^ 0xEDB88320;
                } else {
                    crc >>= 1;
                }
            }
        }
        return crc ^ 0xFFFFFFFF;
    }
};
