#include "wisp_runtime_loader.h"
#include "../namespaces.h"  // Include full namespace definitions

// Platform-specific includes and implementations
#if defined(ESP32) || defined(ARDUINO)
    #include <FS.h>
    #include <SPIFFS.h>
    #include <SD.h>
    // String class is already defined in esp32_common.h
#else
    #include <fstream>
    #include <iostream>
    #include <cstring>
    #include <cstdlib>
#endif

namespace WispEngine {
namespace App {

WispRuntimeLoader::WispRuntimeLoader() 
    : bundleData(nullptr), bundleSize(0), ownsData(false),
      entries(nullptr), configData(nullptr), assetDataStart(nullptr) {
    memset(&header, 0, sizeof(header));
}

WispRuntimeLoader::~WispRuntimeLoader() {
    unload();
}

void WispRuntimeLoader::unload() {
    if (bundleData && ownsData) {
        free(bundleData);
    }
    bundleData = nullptr;
    bundleSize = 0;
    ownsData = false;
    entries = nullptr;
    configData = nullptr;
    assetDataStart = nullptr;
    memset(&header, 0, sizeof(header));
}

WispLoadResult WispRuntimeLoader::loadFromFile(const String& filePath) {
    WispEngine::Core::Debug::info("WISP_LOADER", "Loading bundle from file");
    
    unload(); // Clean up any existing data
    
#if defined(ESP32) || defined(ARDUINO)
    // ESP32/Arduino file loading
    File file;
    
    // Try SPIFFS first, then SD
    if (SPIFFS.exists(filePath.c_str())) {
        file = SPIFFS.open(filePath.c_str(), "r");
    } else if (SD.exists(filePath.c_str())) {
        file = SD.open(filePath.c_str(), FILE_READ);
    }
    
    if (!file) {
        WispEngine::Core::Debug::error("WISP_LOADER", "Failed to open file");
        return WISP_LOAD_FILE_NOT_FOUND;
    }
    
    bundleSize = file.size();
    if (bundleSize < sizeof(WispBundleHeader)) {
        WispEngine::Core::Debug::error("WISP_LOADER", "File too small");
        file.close();
        return WISP_LOAD_INVALID_BUNDLE;
    }
    
    bundleData = (uint8_t*)malloc(bundleSize);
    if (!bundleData) {
        WispEngine::Core::Debug::error("WISP_LOADER", "Memory allocation failed");
        file.close();
        return WISP_LOAD_MEMORY_ERROR;
    }
    
    size_t bytesRead = file.readBytes((char*)bundleData, bundleSize);
    file.close();
    
    if (bytesRead != bundleSize) {
        WispEngine::Core::Debug::error("WISP_LOADER", "Read error");
        free(bundleData);
        bundleData = nullptr;
        return WISP_LOAD_IO_ERROR;
    }
    
#else
    // Desktop file loading
    std::ifstream file(filePath.c_str(), std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return WISP_LOAD_FILE_NOT_FOUND;
    }
    
    bundleSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    if (bundleSize < sizeof(WispBundleHeader)) {
        return WISP_LOAD_INVALID_BUNDLE;
    }
    
    bundleData = (uint8_t*)malloc(bundleSize);
    if (!bundleData) {
        return WISP_LOAD_MEMORY_ERROR;
    }
    
    file.read((char*)bundleData, bundleSize);
    if (!file.good()) {
        free(bundleData);
        bundleData = nullptr;
        return WISP_LOAD_IO_ERROR;
    }
    file.close();
#endif
    
    ownsData = true;
    
    // Validate and parse the bundle
    if (validateBundle()) {
        WispEngine::Core::Debug::info("WISP_LOADER", "Bundle loaded successfully");
        return WISP_LOAD_SUCCESS;
    } else {
        unload();
        return WISP_LOAD_INVALID_BUNDLE;
    }
}

WispLoadResult WispRuntimeLoader::loadFromMemory(const uint8_t* data, size_t size, bool copyData) {
    unload();
    
    if (!data || size < sizeof(WispBundleHeader)) {
        return WISP_LOAD_INVALID_BUNDLE;
    }
    
    if (copyData) {
        bundleData = (uint8_t*)malloc(size);
        if (!bundleData) {
            return WISP_LOAD_MEMORY_ERROR;
        }
        memcpy(bundleData, data, size);
        ownsData = true;
    } else {
        bundleData = const_cast<uint8_t*>(data);
        ownsData = false;
    }
    
    bundleSize = size;
    
    if (!validateBundle()) {
        unload();
        return WISP_LOAD_INVALID_BUNDLE;
    }
    
    return WISP_LOAD_SUCCESS;
}

bool WispRuntimeLoader::validateBundle() {
    if (bundleSize < sizeof(WispBundleHeader)) {
        return false;
    }
    
    // Parse header
    memcpy(&header, bundleData, sizeof(WispBundleHeader));
    
    // Validate magic number
    if (header.magic != 0x50534957) { // 'WISP'
        WispEngine::Core::Debug::error("WISP_LOADER", "Invalid magic number");
        return false;
    }
    
    // Calculate layout
    size_t headerSize = sizeof(WispBundleHeader);
    size_t configSize = header.configSize;
    size_t entryTableSize = header.entryCount * sizeof(WispAssetEntry);
    size_t expectedMinSize = headerSize + configSize + entryTableSize;
    
    if (bundleSize < expectedMinSize) {
        WispEngine::Core::Debug::error("WISP_LOADER", "Bundle too small");
        return false;
    }
    
    // Set up pointers
    uint8_t* ptr = bundleData + headerSize;
    
    if (configSize > 0) {
        configData = (const char*)ptr;
        ptr += configSize;
    } else {
        configData = nullptr;
    }
    
    if (header.entryCount > 0) {
        entries = (WispAssetEntry*)ptr;
        ptr += entryTableSize;
    } else {
        entries = nullptr;
    }
    
    assetDataStart = ptr;
    
    // Validate asset entries
    for (uint16_t i = 0; i < header.entryCount; i++) {
        WispAssetEntry& entry = entries[i];
        
        // Check bounds
        size_t assetStart = (size_t)assetDataStart + entry.offset;
        size_t assetEnd = assetStart + entry.size;
        
        if (assetEnd > (size_t)bundleData + bundleSize) {
            WispEngine::Core::Debug::error("WISP_LOADER", "Asset extends beyond bundle");
            return false;
        }
    }
    
    WispEngine::Core::Debug::info("WISP_LOADER", "Bundle validation successful");
    return true;
}

WispAssetEntry* WispRuntimeLoader::findAssetEntry(const String& assetName) {
    if (!entries || assetName.length() == 0) {
        return nullptr;
    }
    
    for (uint16_t i = 0; i < header.entryCount; i++) {
        if (strcmp(entries[i].name, assetName.c_str()) == 0) {
            return &entries[i];
        }
    }
    
    return nullptr;
}

bool WispRuntimeLoader::hasAsset(const String& assetName) {
    return findAssetEntry(assetName) != nullptr;
}

WispLoadResult WispRuntimeLoader::getAssetInfo(const String& assetName, WispAssetEntry& info) {
    WispAssetEntry* entry = findAssetEntry(assetName);
    if (!entry) {
        return WISP_LOAD_ASSET_NOT_FOUND;
    }
    
    memcpy(&info, entry, sizeof(WispAssetEntry));
    return WISP_LOAD_SUCCESS;
}

const uint8_t* WispRuntimeLoader::getAssetData(const String& assetName) {
    WispAssetEntry* entry = findAssetEntry(assetName);
    if (!entry) {
        return nullptr;
    }
    
    return assetDataStart + entry->offset;
}

WispLoadResult WispRuntimeLoader::extractAsset(const String& assetName, uint8_t** data, size_t* size) {
    WispAssetEntry* entry = findAssetEntry(assetName);
    if (!entry) {
        return WISP_LOAD_ASSET_NOT_FOUND;
    }
    
    *data = (uint8_t*)malloc(entry->size);
    if (!*data) {
        return WISP_LOAD_MEMORY_ERROR;
    }
    
    memcpy(*data, assetDataStart + entry->offset, entry->size);
    *size = entry->size;
    
    return WISP_LOAD_SUCCESS;
}

const WispAssetEntry* WispRuntimeLoader::getAssetEntry(uint16_t index) const {
    if (index >= header.entryCount || !entries) {
        return nullptr;
    }
    
    return &entries[index];
}

String WispRuntimeLoader::getConfigValue(const String& key) {
    if (!configData || header.configSize == 0) {
        return String();
    }
    
    // Simple YAML key-value parser - use C-style string operations for compatibility
    const char* configStr = configData;
    size_t configLen = header.configSize;
    
    // Create search key "key:"
    char searchKey[256];
    snprintf(searchKey, sizeof(searchKey), "%s:", key.c_str());
    
    // Find the key
    const char* keyPos = strstr(configStr, searchKey);
    if (!keyPos) {
        return String();
    }
    
    // Find value start (after colon and optional whitespace)
    const char* valueStart = keyPos + strlen(searchKey);
    while (valueStart < configStr + configLen && (*valueStart == ' ' || *valueStart == '\t')) {
        valueStart++;
    }
    
    // Find line end
    const char* lineEnd = strchr(valueStart, '\n');
    if (!lineEnd) {
        lineEnd = configStr + configLen;
    }
    
    // Extract value
    size_t valueLen = lineEnd - valueStart;
    char* valueBuffer = (char*)malloc(valueLen + 1);
    strncpy(valueBuffer, valueStart, valueLen);
    valueBuffer[valueLen] = '\0';
    
    // Remove quotes if present
    char* trimmed = valueBuffer;
    if (valueLen > 1 && trimmed[0] == '"' && trimmed[valueLen-1] == '"') {
        trimmed[valueLen-1] = '\0';
        trimmed++;
    }
    
    String result(trimmed);
    free(valueBuffer);
    return result;
}

void WispRuntimeLoader::freeExtractedAsset(uint8_t* data) {
    if (data) {
        free(data);
    }
}

} // namespace App
} // namespace WispEngine
