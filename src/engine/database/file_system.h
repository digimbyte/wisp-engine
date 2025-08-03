// file_system.h - ESP32 native file system wrapper
#pragma once
#include "../system/esp32_common.h"
#include <esp_vfs.h>
#include <esp_spiffs.h>
#include <stdio.h>
#include <string.h>

// File access modes compatible with Arduino
#define FILE_READ "r"
#define FILE_WRITE "w"
#define FILE_APPEND "a"

// ESP32 File wrapper class to replace Arduino File
class File {
private:
    FILE* file;
    char path[256];
    bool isOpen;

public:
    File() : file(nullptr), isOpen(false) {
        path[0] = '\0';
    }
    File(const char* filepath, const char* mode) : file(nullptr), isOpen(false) {
        strncpy(path, filepath, sizeof(path) - 1);
        path[sizeof(path) - 1] = '\0';
        open(filepath, mode);
    }
    
    bool open(const char* filepath, const char* mode) {
        file = fopen(filepath, mode);
        isOpen = (file != nullptr);
        return isOpen;
    }
    
    void close() {
        if (isOpen && file) {
            fclose(file);
            file = nullptr;
            isOpen = false;
        }
    }
    
    operator bool() const { return isOpen; }
    bool available() { return isOpen && file; }
    
    bool readString(char* buffer, size_t maxLen) {
        if (!isOpen || !file || !buffer) return false;
        return fgets(buffer, maxLen, file) != nullptr;
    }
    
    size_t write(const char* data) {
        if (!isOpen || !file) return 0;
        return fwrite(data, 1, strlen(data), file);
    }
    
    void flush() { 
        if (isOpen && file) fflush(file); 
    }
};

// ESP32 SPIFFS wrapper
class SPIFFSClass {
public:
    bool begin() {
        esp_vfs_spiffs_conf_t conf = {
            .base_path = "/spiffs",
            .partition_label = NULL,
            .max_files = 5,
            .format_if_mount_failed = true
        };
        esp_err_t ret = esp_vfs_spiffs_register(&conf);
        return (ret == ESP_OK);
    }
    
    void end() {
        esp_vfs_spiffs_unregister(NULL);
    }
    
    File open(const char* path, const char* mode = FILE_READ) {
        char fullPath[256];
        snprintf(fullPath, sizeof(fullPath), "/spiffs%s", path);
        return File(fullPath, mode);
    }
    
    bool exists(const char* path) {
        char fullPath[256];
        snprintf(fullPath, sizeof(fullPath), "/spiffs%s", path);
        FILE* file = fopen(fullPath, "r");
        if (file) {
            fclose(file);
            return true;
        }
        return false;
    }
};

// ESP32 SD wrapper (basic implementation)
class SDClass {
public:
    bool begin() {
        // Basic SD initialization - would need proper SD card setup
        return false; // Disabled for now
    }
    
    File open(const char* path, const char* mode = FILE_READ) {
        char fullPath[256];
        snprintf(fullPath, sizeof(fullPath), "/sd%s", path);
        return File(fullPath, mode);
    }
    
    bool exists(const char* path) {
        char fullPath[256];
        snprintf(fullPath, sizeof(fullPath), "/sd%s", path);
        FILE* file = fopen(fullPath, "r");
        if (file) {
            fclose(file);
            return true;
        }
        return false;
    }
};

// Global instances
extern SPIFFSClass SPIFFS;
extern SDClass SD;
