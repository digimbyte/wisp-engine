// engine/save_system.h - ESP32-C6/S3 Save System using ESP-IDF
// Persistent storage system using SPIFFS and LP-SRAM for ESP32
#pragma once
#include "../../system/esp32_common.h"  // Pure ESP-IDF native headers
// ESP-IDF native includes - no Arduino SPIFFS or SD libs
#include "../../engine/core/debug.h"

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
    char key[64];               // Field identifier (e.g. "player_level", "high_score")
    WispSaveDataType type;      // Data type
    void* data;                 // Pointer to actual data
    size_t size;                // Size in bytes (for strings/blobs)
    bool isDirty;               // Has been modified since last save
    
    WispSaveField() : type(SAVE_TYPE_BOOL), data(nullptr), size(0), isDirty(false) {
        key[0] = '\0';
    }
    
    WispSaveField(const char* k, WispSaveDataType t, void* d, size_t s = 0) 
        : type(t), data(d), size(s), isDirty(false) {
        strncpy(key, k, sizeof(key) - 1);
        key[sizeof(key) - 1] = '\0';
    }
};

// App identity for save file fingerprinting
struct WispAppIdentity {
    char uuid[64];              // Unique app identifier (e.g. "com.developer.gamename")
    char version[16];           // App version for save compatibility
    uint32_t saveFormatVersion; // Save format version for migration
    
    WispAppIdentity() : saveFormatVersion(1) {
        uuid[0] = '\0';
        version[0] = '\0';
    }
    WispAppIdentity(const char* u, const char* v, uint32_t sfv = 1) 
        : saveFormatVersion(sfv) {
        strncpy(uuid, u, sizeof(uuid) - 1);
        uuid[sizeof(uuid) - 1] = '\0';
        strncpy(version, v, sizeof(version) - 1);
        version[sizeof(version) - 1] = '\0';
    }
    
    // Generate fingerprint for save file validation
    uint32_t generateFingerprint() const {
        uint32_t hash = 5381; // djb2 hash
        for (int i = 0; uuid[i] != '\0'; i++) {
            hash = ((hash << 5) + hash) + uuid[i]; // hash * 33 + c
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
    WispSaveField saveFields[64]; // Fixed array for save fields
    int saveFieldCount;
    char saveDirectory[256];
    bool useSDCard;
    bool autoSave;
    uint32_t autoSaveInterval;
    uint32_t lastAutoSave;
    
    // Internal methods
    void getSaveFilePath(char* buffer, size_t bufferSize) const;
    void getBackupFilePath(char* buffer, size_t bufferSize) const;
    uint32_t calculateChecksum(const uint8_t* data, size_t size) const;
    bool validateSaveFile(const char* filePath, WispSaveHeader& header) const;
    WispSaveResult writeSaveData(const char* filePath) const;
    WispSaveResult readSaveData(const char* filePath);
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
    bool registerField(const char* key, T* dataPtr);
    bool registerStringField(const char* key, char* stringPtr, size_t maxLength = 256);
    bool registerBlobField(const char* key, void* dataPtr, size_t size);
    
    // Save field access (null-safe)
    template<typename T>
    T* getField(const char* key);
    char* getStringField(const char* key);
    void* getBlobField(const char* key, size_t* outSize = nullptr);
    
    // Save field modification (marks dirty for auto-save)
    template<typename T>
    bool setField(const char* key, const T& value);
    bool setStringField(const char* key, const char* value);
    bool setBlobField(const char* key, const void* data, size_t size);
    
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
    bool hasField(const char* key) const;
    WispSaveDataType getFieldType(const char* key) const;
    bool isFieldDirty(const char* key) const;
    void markFieldClean(const char* key);
    void markAllFieldsClean();
    
    // Save system status
    bool isInitialized() const { return currentApp.uuid[0] != '\0'; }
    const WispAppIdentity& getAppIdentity() const { return currentApp; }
    size_t getFieldCount() const { return saveFieldCount; }
};

// Template implementations for type-safe field registration
template<typename T>
bool WispSaveSystem::registerField(const char* key, T* dataPtr) {
    if (!dataPtr || hasField(key)) {
        WISP_DEBUG_ERROR("SAVE", "Invalid data pointer or field already exists");
        return false;
    }
    
    WispSaveDataType type;
    size_t size = sizeof(T);
    
    // Determine type based on template parameter
    if (sizeof(T) == sizeof(bool)) type = SAVE_TYPE_BOOL;
    else if (sizeof(T) == sizeof(int8_t) && (T)(-1) < 0) type = SAVE_TYPE_INT8;
    else if (sizeof(T) == sizeof(uint8_t) && (T)(-1) > 0) type = SAVE_TYPE_UINT8;
    else if (sizeof(T) == sizeof(int16_t) && (T)(-1) < 0) type = SAVE_TYPE_INT16;
    else if (sizeof(T) == sizeof(uint16_t) && (T)(-1) > 0) type = SAVE_TYPE_UINT16;
    else if (sizeof(T) == sizeof(int32_t) && (T)(-1) < 0) type = SAVE_TYPE_INT32;
    else if (sizeof(T) == sizeof(uint32_t) && (T)(-1) > 0) type = SAVE_TYPE_UINT32;
    else if (sizeof(T) == sizeof(float)) type = SAVE_TYPE_FLOAT;
    else {
        WISP_DEBUG_ERROR("SAVE", "Unsupported data type for field");
        return false;
    }
    
    saveFields[saveFieldCount] = WispSaveField(key, type, dataPtr, size);
    saveFieldCount++;
    WISP_DEBUG_INFO("SAVE", "Registered field");
    return true;
}

template<typename T>
T* WispSaveSystem::getField(const char* key) {
    for (int i = 0; i < saveFieldCount; i++) {
        if (strcmp(saveFields[i].key, key) == 0) {
            return static_cast<T*>(saveFields[i].data);
        }
    }
    WISP_DEBUG_WARNING("SAVE", "Field not found");
    return nullptr;
}

template<typename T>
bool WispSaveSystem::setField(const char* key, const T& value) {
    T* fieldPtr = getField<T>(key);
    if (!fieldPtr) {
        return false;
    }
    
    *fieldPtr = value;
    
    // Find and mark field as dirty
    for (int i = 0; i < saveFieldCount; i++) {
        if (strcmp(saveFields[i].key, key) == 0) {
            saveFields[i].isDirty = true;
            break;
        }
    }
    
    WISP_DEBUG_INFO("SAVE", "Field updated");
    return true;
}

// Save system error messages
const char* getSaveResultString(WispSaveResult result);

// Global save system instance (initialized by bootloader)
extern WispSaveSystem* g_SaveSystem;
