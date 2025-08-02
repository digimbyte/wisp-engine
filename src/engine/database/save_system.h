// engine/wisp_save_system.h - ESP32-C6/S3 Save System using ESP-IDF
// Persistent storage system using SPIFFS and LP-SRAM for ESP32
#pragma once
#include "../../system/esp32_common.h"  // Pure ESP-IDF native headers
#include <map>
#include <vector>
// ESP-IDF native includes - no Arduino SPIFFS or SD libs
#include "wisp_debug_system.h"

// Save data types supported by the system
enum WispSaveDataType {
    SAVE_TYPE_BOOL,
    SAVE_TYPE_INT8,
    SAVE_TYPE_UINT8, 
    SAVE_TYPE_INT16,
    SAVE_TYPE_UINT16,
    SAVE_TYPE_INT32,
    SAVE_TYPE_UINT32,
    SAVE_TYPE_FLOAT,
    SAVE_TYPE_STRING,
    SAVE_TYPE_BLOB          // Raw binary data
};

// Individual save field definition
struct WispSaveField {
    String key;                 // Field identifier (e.g. "player_level", "high_score")
    WispSaveDataType type;      // Data type
    void* data;                 // Pointer to actual data
    size_t size;                // Size in bytes (for strings/blobs)
    bool isDirty;               // Has been modified since last save
    
    WispSaveField() : type(SAVE_TYPE_BOOL), data(nullptr), size(0), isDirty(false) {}
    
    WispSaveField(const String& k, WispSaveDataType t, void* d, size_t s = 0) 
        : key(k), type(t), data(d), size(s), isDirty(false) {}
};

// App identity for save file fingerprinting
struct WispAppIdentity {
    String uuid;                // Unique app identifier (e.g. "com.developer.gamename")
    String version;             // App version for save compatibility
    uint32_t saveFormatVersion; // Save format version for migration
    
    WispAppIdentity() : saveFormatVersion(1) {}
    WispAppIdentity(const String& u, const String& v, uint32_t sfv = 1) 
        : uuid(u), version(v), saveFormatVersion(sfv) {}
    
    // Generate fingerprint for save file validation
    uint32_t generateFingerprint() const {
        uint32_t hash = 5381; // djb2 hash
        for (char c : uuid) {
            hash = ((hash << 5) + hash) + c;
        }
        return hash ^ saveFormatVersion;
    }
};

// Save file header for validation and metadata
struct WispSaveHeader {
    uint32_t magic;             // File magic number (0x57495350 = "WISP")
    uint32_t fingerprint;       // App identity fingerprint
    uint32_t saveFormatVersion; // Save format version
    uint32_t dataSize;          // Size of save data section
    uint32_t checksum;          // CRC32 of data section
    uint64_t timestamp;         // Save timestamp (millis since boot)
    char appUuid[64];           // Null-terminated app UUID
    char appVersion[16];        // Null-terminated app version
    
    WispSaveHeader() : magic(0x57495350), fingerprint(0), saveFormatVersion(1), 
                      dataSize(0), checksum(0), timestamp(0) {
        memset(appUuid, 0, sizeof(appUuid));
        memset(appVersion, 0, sizeof(appVersion));
    }
};

// Save system result codes
enum WispSaveResult {
    SAVE_SUCCESS,
    SAVE_ERROR_NO_STORAGE,
    SAVE_ERROR_WRITE_FAILED,
    SAVE_ERROR_READ_FAILED,
    SAVE_ERROR_INVALID_FILE,
    SAVE_ERROR_WRONG_APP,
    SAVE_ERROR_VERSION_MISMATCH,
    SAVE_ERROR_CORRUPTED,
    SAVE_ERROR_NOT_FOUND,
    SAVE_ERROR_MEMORY_FULL
};

// Main save system class
class WispSaveSystem {
private:
    WispAppIdentity currentApp;
    std::map<String, WispSaveField> saveFields;
    String saveDirectory;
    bool useSDCard;
    bool autoSave;
    uint32_t autoSaveInterval;
    uint32_t lastAutoSave;
    
    // Internal methods
    String getSaveFilePath() const;
    String getBackupFilePath() const;
    uint32_t calculateChecksum(const uint8_t* data, size_t size) const;
    bool validateSaveFile(const String& filePath, WispSaveHeader& header) const;
    WispSaveResult writeSaveData(const String& filePath) const;
    WispSaveResult readSaveData(const String& filePath);
    void createBackup() const;
    bool restoreFromBackup();
    
public:
    WispSaveSystem() : useSDCard(false), autoSave(false), autoSaveInterval(30000), lastAutoSave(0) {
        saveDirectory = "/saves";
    }
    
    // System initialization
    bool init(bool preferSDCard = true);
    void setAppIdentity(const WispAppIdentity& identity);
    void setAutoSave(bool enabled, uint32_t intervalMs = 30000);
    
    // Save field registration (must be done before loading)
    template<typename T>
    bool registerField(const String& key, T* dataPtr);
    bool registerStringField(const String& key, String* stringPtr, size_t maxLength = 256);
    bool registerBlobField(const String& key, void* dataPtr, size_t size);
    
    // Save field access (null-safe)
    template<typename T>
    T* getField(const String& key);
    String* getStringField(const String& key);
    void* getBlobField(const String& key, size_t* outSize = nullptr);
    
    // Save field modification (marks dirty for auto-save)
    template<typename T>
    bool setField(const String& key, const T& value);
    bool setStringField(const String& key, const String& value);
    bool setBlobField(const String& key, const void* data, size_t size);
    
    // Manual save/load operations
    WispSaveResult save();
    WispSaveResult load();
    WispSaveResult reset(); // Clear all data to defaults
    
    // Save file management
    bool hasSaveFile() const;
    bool deleteSaveFile();
    uint64_t getSaveTimestamp() const;
    size_t getSaveFileSize() const;
    
    // System update (call in main loop for auto-save)
    void update();
    
    // Debug and monitoring
    void printSaveStatus() const;
    void printFieldStatus() const;
    size_t getMemoryUsage() const;
    
    // Field validation
    bool hasField(const String& key) const;
    WispSaveDataType getFieldType(const String& key) const;
    bool isFieldDirty(const String& key) const;
    void markFieldClean(const String& key);
    void markAllFieldsClean();
    
    // Save system status
    bool isInitialized() const { return !currentApp.uuid.isEmpty(); }
    const WispAppIdentity& getAppIdentity() const { return currentApp; }
    size_t getFieldCount() const { return saveFields.size(); }
};

// Template implementations for type-safe field registration
template<typename T>
bool WispSaveSystem::registerField(const String& key, T* dataPtr) {
    if (!dataPtr || hasField(key)) {
        WISP_DEBUG_ERROR("SAVE", "Invalid data pointer or field already exists: " + key);
        return false;
    }
    
    WispSaveDataType type;
    size_t size = sizeof(T);
    
    // Determine type based on template parameter
    if (std::is_same<T, bool>::value) type = SAVE_TYPE_BOOL;
    else if (std::is_same<T, int8_t>::value) type = SAVE_TYPE_INT8;
    else if (std::is_same<T, uint8_t>::value) type = SAVE_TYPE_UINT8;
    else if (std::is_same<T, int16_t>::value) type = SAVE_TYPE_INT16;
    else if (std::is_same<T, uint16_t>::value) type = SAVE_TYPE_UINT16;
    else if (std::is_same<T, int32_t>::value) type = SAVE_TYPE_INT32;
    else if (std::is_same<T, uint32_t>::value) type = SAVE_TYPE_UINT32;
    else if (std::is_same<T, float>::value) type = SAVE_TYPE_FLOAT;
    else {
        WISP_DEBUG_ERROR("SAVE", "Unsupported data type for field: " + key);
        return false;
    }
    
    saveFields[key] = WispSaveField(key, type, dataPtr, size);
    WISP_DEBUG_INFO("SAVE", "Registered field: " + key);
    return true;
}

template<typename T>
T* WispSaveSystem::getField(const String& key) {
    auto it = saveFields.find(key);
    if (it == saveFields.end()) {
        WISP_DEBUG_WARNING("SAVE", "Field not found: " + key);
        return nullptr;
    }
    
    // Type safety check would go here in a more robust implementation
    return static_cast<T*>(it->second.data);
}

template<typename T>
bool WispSaveSystem::setField(const String& key, const T& value) {
    T* fieldPtr = getField<T>(key);
    if (!fieldPtr) {
        return false;
    }
    
    *fieldPtr = value;
    saveFields[key].isDirty = true;
    
    WISP_DEBUG_INFO("SAVE", "Field updated: " + key);
    return true;
}

// Save system error messages
const char* getSaveResultString(WispSaveResult result);

// Global save system instance (initialized by bootloader)
extern WispSaveSystem* g_SaveSystem;
