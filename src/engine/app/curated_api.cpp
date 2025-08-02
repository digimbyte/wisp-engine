// engine/curated_api.cpp
#include "curated_api.h"

// Save system integration methods
bool WispCuratedAPI::setAppIdentity(const String& uuid, const String& version, uint32_t saveFormatVersion) {
    if (uuid.isEmpty()) {
        recordError("App UUID cannot be empty");
        return false;
    }
    
    // Validate UUID format (basic reverse domain notation check)
    if (uuid.indexOf('.') < 0 || uuid.length() < 5) {
        recordError("App UUID should use reverse domain notation (e.g. com.developer.gamename)");
        return false;
    }
    
    if (!g_SaveSystem) {
        recordError("Save system not initialized");
        return false;
    }
    
    WispAppIdentity identity(uuid, version, saveFormatVersion);
    g_SaveSystem->setAppIdentity(identity);
    
    print("App identity set: " + uuid + " v" + version);
    return true;
}

bool WispCuratedAPI::registerSaveField(const String& key, bool* value) {
    if (!g_SaveSystem || !value) {
        recordError("Invalid save system or null pointer");
        return false;
    }
    
    if (!g_SaveSystem->registerField(key, value)) {
        recordError("Failed to register save field: " + key);
        return false;
    }
    
    return true;
}

bool WispCuratedAPI::registerSaveField(const String& key, int8_t* value) {
    if (!g_SaveSystem || !value) {
        recordError("Invalid save system or null pointer");
        return false;
    }
    
    return g_SaveSystem->registerField(key, value);
}

bool WispCuratedAPI::registerSaveField(const String& key, uint8_t* value) {
    if (!g_SaveSystem || !value) {
        recordError("Invalid save system or null pointer");
        return false;
    }
    
    return g_SaveSystem->registerField(key, value);
}

bool WispCuratedAPI::registerSaveField(const String& key, int16_t* value) {
    if (!g_SaveSystem || !value) {
        recordError("Invalid save system or null pointer");
        return false;
    }
    
    return g_SaveSystem->registerField(key, value);
}

bool WispCuratedAPI::registerSaveField(const String& key, uint16_t* value) {
    if (!g_SaveSystem || !value) {
        recordError("Invalid save system or null pointer");
        return false;
    }
    
    return g_SaveSystem->registerField(key, value);
}

bool WispCuratedAPI::registerSaveField(const String& key, int32_t* value) {
    if (!g_SaveSystem || !value) {
        recordError("Invalid save system or null pointer");
        return false;
    }
    
    return g_SaveSystem->registerField(key, value);
}

bool WispCuratedAPI::registerSaveField(const String& key, uint32_t* value) {
    if (!g_SaveSystem || !value) {
        recordError("Invalid save system or null pointer");
        return false;
    }
    
    return g_SaveSystem->registerField(key, value);
}

bool WispCuratedAPI::registerSaveField(const String& key, float* value) {
    if (!g_SaveSystem || !value) {
        recordError("Invalid save system or null pointer");
        return false;
    }
    
    return g_SaveSystem->registerField(key, value);
}

bool WispCuratedAPI::registerSaveField(const String& key, String* value, size_t maxLength) {
    if (!g_SaveSystem || !value) {
        recordError("Invalid save system or null pointer");
        return false;
    }
    
    return g_SaveSystem->registerStringField(key, value, maxLength);
}

bool WispCuratedAPI::registerSaveBlob(const String& key, void* data, size_t size) {
    if (!g_SaveSystem || !data || size == 0) {
        recordError("Invalid save system, null pointer, or zero size");
        return false;
    }
    
    return g_SaveSystem->registerBlobField(key, data, size);
}

template<typename T>
T* WispCuratedAPI::getSaveField(const String& key) {
    if (!g_SaveSystem) {
        recordError("Save system not initialized");
        return nullptr;
    }
    
    return g_SaveSystem->getField<T>(key);
}

String* WispCuratedAPI::getSaveString(const String& key) {
    if (!g_SaveSystem) {
        recordError("Save system not initialized");
        return nullptr;
    }
    
    return g_SaveSystem->getStringField(key);
}

void* WispCuratedAPI::getSaveBlob(const String& key, size_t* outSize) {
    if (!g_SaveSystem) {
        recordError("Save system not initialized");
        return nullptr;
    }
    
    return g_SaveSystem->getBlobField(key, outSize);
}

template<typename T>
bool WispCuratedAPI::setSaveField(const String& key, const T& value) {
    if (!g_SaveSystem) {
        recordError("Save system not initialized");
        return false;
    }
    
    return g_SaveSystem->setField(key, value);
}

bool WispCuratedAPI::setSaveString(const String& key, const String& value) {
    if (!g_SaveSystem) {
        recordError("Save system not initialized");
        return false;
    }
    
    return g_SaveSystem->setStringField(key, value);
}

bool WispCuratedAPI::setSaveBlob(const String& key, const void* data, size_t size) {
    if (!g_SaveSystem) {
        recordError("Save system not initialized");
        return false;
    }
    
    return g_SaveSystem->setBlobField(key, data, size);
}

bool WispCuratedAPI::save() {
    if (!g_SaveSystem) {
        recordError("Save system not initialized");
        return false;
    }
    
    WispSaveResult result = g_SaveSystem->save();
    
    if (result != SAVE_SUCCESS) {
        recordError("Save failed: " + String(getSaveResultString(result)));
        return false;
    }
    
    print("Game saved successfully");
    return true;
}

bool WispCuratedAPI::load() {
    if (!g_SaveSystem) {
        recordError("Save system not initialized");
        return false;
    }
    
    WispSaveResult result = g_SaveSystem->load();
    
    if (result != SAVE_SUCCESS) {
        // For load operations, some errors are not critical
        if (result == SAVE_ERROR_NOT_FOUND) {
            print("No save file found - starting fresh");
            return true; // This is OK for first run
        } else {
            printWarning("Load failed: " + String(getSaveResultString(result)));
            return false;
        }
    }
    
    print("Game loaded successfully");
    return true;
}

bool WispCuratedAPI::resetSaveData() {
    if (!g_SaveSystem) {
        recordError("Save system not initialized");
        return false;
    }
    
    WispSaveResult result = g_SaveSystem->reset();
    
    if (result != SAVE_SUCCESS) {
        recordError("Reset failed: " + String(getSaveResultString(result)));
        return false;
    }
    
    print("Save data reset to defaults");
    return true;
}

bool WispCuratedAPI::hasSaveFile() const {
    if (!g_SaveSystem) {
        return false;
    }
    
    return g_SaveSystem->hasSaveFile();
}

bool WispCuratedAPI::deleteSaveFile() {
    if (!g_SaveSystem) {
        recordError("Save system not initialized");
        return false;
    }
    
    if (g_SaveSystem->deleteSaveFile()) {
        print("Save file deleted");
        return true;
    } else {
        recordError("Failed to delete save file");
        return false;
    }
}

void WispCuratedAPI::enableAutoSave(bool enabled, uint32_t intervalMs) {
    if (!g_SaveSystem) {
        recordError("Save system not initialized");
        return;
    }
    
    g_SaveSystem->setAutoSave(enabled, intervalMs);
    
    if (enabled) {
        print("Auto-save enabled (interval: " + String(intervalMs) + "ms)");
    } else {
        print("Auto-save disabled");
    }
}

bool WispCuratedAPI::isSaveSystemReady() const {
    return g_SaveSystem && g_SaveSystem->isInitialized();
}

uint64_t WispCuratedAPI::getSaveTimestamp() const {
    if (!g_SaveSystem) {
        return 0;
    }
    
    return g_SaveSystem->getSaveTimestamp();
}

size_t WispCuratedAPI::getSaveFileSize() const {
    if (!g_SaveSystem) {
        return 0;
    }
    
    return g_SaveSystem->getSaveFileSize();
}

// Explicit template instantiations for common types
template bool* WispCuratedAPI::getSaveField<bool>(const String& key);
template int8_t* WispCuratedAPI::getSaveField<int8_t>(const String& key);
template uint8_t* WispCuratedAPI::getSaveField<uint8_t>(const String& key);
template int16_t* WispCuratedAPI::getSaveField<int16_t>(const String& key);
template uint16_t* WispCuratedAPI::getSaveField<uint16_t>(const String& key);
template int32_t* WispCuratedAPI::getSaveField<int32_t>(const String& key);
template uint32_t* WispCuratedAPI::getSaveField<uint32_t>(const String& key);
template float* WispCuratedAPI::getSaveField<float>(const String& key);

template bool WispCuratedAPI::setSaveField<bool>(const String& key, const bool& value);
template bool WispCuratedAPI::setSaveField<int8_t>(const String& key, const int8_t& value);
template bool WispCuratedAPI::setSaveField<uint8_t>(const String& key, const uint8_t& value);
template bool WispCuratedAPI::setSaveField<int16_t>(const String& key, const int16_t& value);
template bool WispCuratedAPI::setSaveField<uint16_t>(const String& key, const uint16_t& value);
template bool WispCuratedAPI::setSaveField<int32_t>(const String& key, const int32_t& value);
template bool WispCuratedAPI::setSaveField<uint32_t>(const String& key, const uint32_t& value);
template bool WispCuratedAPI::setSaveField<float>(const String& key, const float& value);
