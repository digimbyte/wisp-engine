// system_settings.h - System Settings Panel for Wisp Engine
#ifndef SYSTEM_SETTINGS_H
#define SYSTEM_SETTINGS_H

#include "menu.h"
#include "../../system/definitions.h"
#include <esp_system.h>
#include <esp_partition.h>
#include "esp_spiffs.h"
#include "soc/rtc.h"
#include <sys/stat.h>

class SystemSettingsPanel : public MenuPanel {
private:
    struct SystemSettings {
        uint8_t cpuFrequenc        if (settings.sleepTimeout < 60) {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%u seconds", settings.sleepTimeout);
            gfx->drawText(buffer, SCREEN_WIDTH / 2, barY + barHeight + 10, true);
        } else if (settings.sleepTimeout < 3600) {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%u minutes", settings.sleepTimeout / 60);
            gfx->drawText(buffer, SCREEN_WIDTH / 2, barY + barHeight + 10, true);
        } else {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%u hours", settings.sleepTimeout / 3600);
            gfx->drawText(buffer, SCREEN_WIDTH / 2, barY + barHeight + 10, true);
        }      // 0=80MHz, 1=160MHz, 2=240MHz
        uint8_t sleepMode = 1;          // 0=none, 1=light, 2=deep
        uint32_t sleepTimeout = 300;    // Seconds
        bool enableDeepSleep = true;
        bool enableWatchdog = true;
        uint8_t logLevel = 2;           // 0=none, 1=error, 2=warn, 3=info, 4=debug
        bool enableSerial = true;
        uint8_t powerProfile = 1;       // 0=performance, 1=balanced, 2=power_save
        bool enableOTA = true;
        String firmwareVersion = "1.0.0";
    } settings;
    
    enum SystemMenuState {
        DEVICE_INFO,
        CPU_FREQUENCY,
        POWER_PROFILE,
        SLEEP_MODE,
        SLEEP_TIMEOUT,
        WATCHDOG,
        LOG_LEVEL,
        SERIAL_DEBUG,
        OTA_UPDATES,
        STORAGE_INFO,
        FACTORY_RESET,
        RESTART_DEVICE,
        FIRMWARE_INFO,
        SAVE_SETTINGS,
        SYSTEM_MENU_COUNT
    };
    
    SystemMenuState currentSelection = DEVICE_INFO;
    bool inConfigMode = false;
    bool confirmationMode = false;
    uint32_t confirmationTimer = 0;
    
    const char* menuItems[SYSTEM_MENU_COUNT] = {
        "Device Info",
        "CPU Frequency",
        "Power Profile",
        "Sleep Mode",
        "Sleep Timeout",
        "Watchdog Timer",
        "Log Level",
        "Serial Debug",
        "OTA Updates",
        "Storage Info",
        "Factory Reset",
        "Restart Device",
        "Firmware Info",
        "Save & Exit"
    };

public:
    SystemSettingsPanel(WispCuratedAPI* api) : MenuPanel(api) {
        loadSettings();
        updateSystemInfo();
    }
    
    void activate() override {
        MenuPanel::activate();
        currentSelection = DEVICE_INFO;
        inConfigMode = false;
        confirmationMode = false;
        loadSettings();
        updateSystemInfo();
    }
    
    void update(const WispInputState& input) override {
        if (!isActive()) return;
        
        if (confirmationMode) {
            handleConfirmation(input);
        } else if (inConfigMode) {
            handleConfiguration(input);
        } else {
            handleNavigation(input);
        }
        
        // Auto-save periodically
        static uint32_t lastSaveTime = 0;
        if (millis() - lastSaveTime > 10000) { // Every 10 seconds
            saveSettings();
            lastSaveTime = millis();
        }
    }
    
    void render() override {
        if (!isActive()) return;
        
        auto* gfx = api->graphics();
        
        // Clear background
        gfx->fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_DARK_RED);
        
        // Title
        gfx->setTextColor(COLOR_WHITE);
        gfx->setTextSize(2);
        gfx->drawText("SYSTEM SETTINGS", SCREEN_WIDTH / 2, 15, true);
        
        if (confirmationMode) {
            renderConfirmation();
        } else if (inConfigMode) {
            renderConfiguration();
        } else {
            renderMainMenu();
        }
        
        // System status bar
        renderSystemStatus();
    }
    
private:
    void handleNavigation(const WispInputState& input) {
        static uint32_t lastInputTime = 0;
        uint32_t currentTime = millis();
        
        if (currentTime - lastInputTime < 150) return; // Debounce
        
        if (input.up) {
            currentSelection = (SystemMenuState)((currentSelection - 1 + SYSTEM_MENU_COUNT) % SYSTEM_MENU_COUNT);
            lastInputTime = currentTime;
        } else if (input.down) {
            currentSelection = (SystemMenuState)((currentSelection + 1) % SYSTEM_MENU_COUNT);
            lastInputTime = currentTime;
        } else if (input.buttonA || input.select) {
            handleMenuAction();
            lastInputTime = currentTime;
        } else if (input.buttonB) {
            deactivate();
            lastInputTime = currentTime;
        }
    }
    
    void handleConfiguration(const WispInputState& input) {
        static uint32_t lastInputTime = 0;
        uint32_t currentTime = millis();
        
        if (currentTime - lastInputTime < 150) return;
        
        if (input.left) {
            adjustValue(currentSelection, -1);
            lastInputTime = currentTime;
        } else if (input.right) {
            adjustValue(currentSelection, 1);
            lastInputTime = currentTime;
        } else if (input.buttonA || input.select) {
            inConfigMode = false;
            applyCurrentSetting();
            lastInputTime = currentTime;
        } else if (input.buttonB) {
            inConfigMode = false;
            lastInputTime = currentTime;
        }
    }
    
    void handleConfirmation(const WispInputState& input) {
        static uint32_t lastInputTime = 0;
        uint32_t currentTime = millis();
        
        if (currentTime - lastInputTime < 150) return;
        
        // Auto-cancel confirmation after 10 seconds
        if (currentTime - confirmationTimer > 10000) {
            confirmationMode = false;
            return;
        }
        
        if (input.buttonA || input.select) {
            executeConfirmedAction();
            confirmationMode = false;
            lastInputTime = currentTime;
        } else if (input.buttonB) {
            confirmationMode = false;
            lastInputTime = currentTime;
        }
    }
    
    void handleMenuAction() {
        switch (currentSelection) {
            case CPU_FREQUENCY:
            case POWER_PROFILE:
            case SLEEP_MODE:
            case SLEEP_TIMEOUT:
            case LOG_LEVEL:
                inConfigMode = true;
                break;
                
            case WATCHDOG:
                settings.enableWatchdog = !settings.enableWatchdog;
                break;
                
            case SERIAL_DEBUG:
                settings.enableSerial = !settings.enableSerial;
                break;
                
            case OTA_UPDATES:
                settings.enableOTA = !settings.enableOTA;
                break;
                
            case FACTORY_RESET:
            case RESTART_DEVICE:
                confirmationMode = true;
                confirmationTimer = millis();
                break;
                
            case SAVE_SETTINGS:
                saveSettings();
                deactivate();
                break;
                
            default:
                break;
        }
    }
    
    void renderMainMenu() {
        auto* gfx = api->graphics();
        
        gfx->setTextSize(1);
        const int startY = 45;
        const int itemHeight = 17;
        
        for (int i = 0; i < SYSTEM_MENU_COUNT; i++) {
            int y = startY + i * itemHeight;
            
            // Skip items that don't fit on screen
            if (y > SCREEN_HEIGHT - 30) break;
            
            // Highlight current selection
            if (i == currentSelection) {
                uint16_t highlightColor = COLOR_LIGHT_RED;
                if (i == FACTORY_RESET || i == RESTART_DEVICE) {
                    highlightColor = COLOR_ORANGE; // Warning color
                }
                gfx->fillRect(5, y - 2, SCREEN_WIDTH - 10, itemHeight - 2, highlightColor);
                gfx->setTextColor(COLOR_BLACK);
            } else {
                gfx->setTextColor(COLOR_WHITE);
            }
            
            // Draw menu item and value
            gfx->drawText(menuItems[i], 10, y + 2, false);
            
            String valueText = getValueText((SystemMenuState)i);
            if (!valueText.empty()) {
                gfx->drawText(valueText.c_str(), SCREEN_WIDTH - 10, y + 2, false, true);
            }
        }
        
        // Instructions
        gfx->setTextColor(COLOR_LIGHT_GRAY);
        gfx->setTextSize(1);
        gfx->drawText("UP/DOWN: Navigate | SELECT: Action | BACK: Exit", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 12, true);
    }
    
    void renderConfiguration() {
        auto* gfx = api->graphics();
        
        gfx->setTextColor(COLOR_WHITE);
        gfx->setTextSize(1);
        
        String title = menuItems[currentSelection];
        gfx->drawText(title.c_str(), SCREEN_WIDTH / 2, 50, true);
        
        // Current value display
        String currentValue = getValueText(currentSelection);
        gfx->setTextSize(2);
        gfx->drawText(currentValue.c_str(), SCREEN_WIDTH / 2, 80, true);
        
        // Configuration-specific rendering
        switch (currentSelection) {
            case CPU_FREQUENCY:
                renderCPUFrequencyConfig();
                break;
                
            case POWER_PROFILE:
                renderPowerProfileConfig();
                break;
                
            case SLEEP_TIMEOUT:
                renderSleepTimeoutConfig();
                break;
                
            default:
                break;
        }
        
        gfx->setTextColor(COLOR_LIGHT_GRAY);
        gfx->setTextSize(1);
        gfx->drawText("LEFT/RIGHT: Adjust | SELECT: Confirm | BACK: Cancel", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 12, true);
    }
    
    void renderConfirmation() {
        auto* gfx = api->graphics();
        
        // Warning background
        gfx->fillRect(20, 60, SCREEN_WIDTH - 40, 80, COLOR_ORANGE);
        gfx->drawRect(20, 60, SCREEN_WIDTH - 40, 80, COLOR_RED);
        
        gfx->setTextColor(COLOR_BLACK);
        gfx->setTextSize(1);
        
        if (currentSelection == FACTORY_RESET) {
            gfx->drawText("FACTORY RESET", SCREEN_WIDTH / 2, 75, true);
            gfx->drawText("This will erase ALL", SCREEN_WIDTH / 2, 90, true);
            gfx->drawText("settings and data!", SCREEN_WIDTH / 2, 105, true);
        } else if (currentSelection == RESTART_DEVICE) {
            gfx->drawText("RESTART DEVICE", SCREEN_WIDTH / 2, 75, true);
            gfx->drawText("Device will reboot", SCREEN_WIDTH / 2, 90, true);
            gfx->drawText("immediately", SCREEN_WIDTH / 2, 105, true);
        }
        
        gfx->setTextColor(COLOR_WHITE);
        gfx->drawText("SELECT: Confirm | BACK: Cancel", SCREEN_WIDTH / 2, 125, true);
        
        // Countdown timer
        uint32_t remaining = 10 - ((millis() - confirmationTimer) / 1000);
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "Auto-cancel in %us", remaining);
        gfx->drawText(buffer, SCREEN_WIDTH / 2, SCREEN_HEIGHT - 25, true);
    }
    
    void renderCPUFrequencyConfig() {
        auto* gfx = api->graphics();
        
        const char* frequencies[] = {"80 MHz", "160 MHz", "240 MHz"};
        const char* descriptions[] = {"Low Power", "Balanced", "Performance"};
        
        gfx->setTextColor(COLOR_LIGHT_GRAY);
        gfx->setTextSize(1);
        gfx->drawText(descriptions[settings.cpuFrequency], SCREEN_WIDTH / 2, 110, true);
        
        // Power consumption estimate
        const char* powerEstimates[] = {"~50mA", "~80mA", "~120mA"};
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "Est. Power: %u", powerEstimates[settings.cpuFrequency]);
        gfx->drawText(buffer, 
                     SCREEN_WIDTH / 2, 125, true);
    }
    
    void renderPowerProfileConfig() {
        auto* gfx = api->graphics();
        
        const char* profiles[] = {"Performance", "Balanced", "Power Save"};
        const char* descriptions[] = {
            "Max performance, high power",
            "Optimal balance",
            "Min power, reduced performance"
        };
        
        gfx->setTextColor(COLOR_LIGHT_GRAY);
        gfx->setTextSize(1);
        gfx->drawText(descriptions[settings.powerProfile], SCREEN_WIDTH / 2, 110, true);
    }
    
    void renderSleepTimeoutConfig() {
        auto* gfx = api->graphics();
        
        // Timeout bar visualization
        const int barX = 20;
        const int barY = 110;
        const int barWidth = SCREEN_WIDTH - 40;
        const int barHeight = 15;
        
        gfx->drawRect(barX, barY, barWidth, barHeight, COLOR_WHITE);
        
        // Timeout options: 30s, 60s, 300s, 600s, 1800s (30min), 0 (never)
        uint32_t timeouts[] = {30, 60, 300, 600, 1800, 0};
        int currentIndex = 0;
        for (int i = 0; i < 6; i++) {
            if (timeouts[i] == settings.sleepTimeout) {
                currentIndex = i;
                break;
            }
        }
        
        int fillWidth = (currentIndex * barWidth) / 5;
        gfx->fillRect(barX + 1, barY + 1, fillWidth, barHeight - 2, COLOR_BLUE);
        
        gfx->setTextColor(COLOR_LIGHT_GRAY);
        gfx->setTextSize(1);
        if (settings.sleepTimeout == 0) {
            gfx->drawText("Never sleep", SCREEN_WIDTH / 2, barY + barHeight + 10, true);
        } else if (settings.sleepTimeout < 60) {
            gfx->drawText((String(settings.sleepTimeout) + " seconds").c_str(), SCREEN_WIDTH / 2, barY + barHeight + 10, true);
        } else if (settings.sleepTimeout < 3600) {
            gfx->drawText((String(settings.sleepTimeout / 60) + " minutes").c_str(), SCREEN_WIDTH / 2, barY + barHeight + 10, true);
        } else {
            gfx->drawText((String(settings.sleepTimeout / 3600) + " hours").c_str(), SCREEN_WIDTH / 2, barY + barHeight + 10, true);
        }
    }
    
    void renderSystemStatus() {
        auto* gfx = api->graphics();
        
        // Status bar at bottom
        gfx->fillRect(0, SCREEN_HEIGHT - 20, SCREEN_WIDTH, 20, COLOR_BLACK);
        
        gfx->setTextColor(COLOR_GREEN);
        gfx->setTextSize(1);
        
        // CPU info - ESP-IDF native
        rtc_cpu_freq_config_t freq_config;
        rtc_clk_cpu_freq_get_config(&freq_config);
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "CPU: %uMHz", freq_config.freq_mhz);
        gfx->drawText(buffer, 5, SCREEN_HEIGHT - 15, false);
        
        // Memory info - ESP-IDF native
        char memInfo[32];
        snprintf(memInfo, sizeof(memInfo), "%uKB Free", esp_get_free_heap_size() / 1024);
        gfx->drawText(memInfo.c_str(), SCREEN_WIDTH / 2, SCREEN_HEIGHT - 15, true);
        
        // Temperature (if available)
        // String tempInfo = String(temperatureRead()) + "°C";
        // gfx->drawText(tempInfo.c_str(), SCREEN_WIDTH - 5, SCREEN_HEIGHT - 15, false, true);
        
        // Uptime
        uint32_t uptime = millis() / 1000;
        char uptimeStr[32];
        snprintf(uptimeStr, sizeof(uptimeStr), "%uh %um", uptime / 3600, (uptime % 3600) / 60);
        gfx->drawText(uptimeStr.c_str(), SCREEN_WIDTH - 5, SCREEN_HEIGHT - 15, false, true);
    }
    
    String getValueText(SystemMenuState item) {
        switch (item) {
            case DEVICE_INFO: {
                esp_chip_info_t chip_info;
                esp_chip_info(&chip_info);
                return (chip_info.model == CHIP_ESP32C6 ? "ESP32-C6" : "ESP32-S3");
            }
                
            case CPU_FREQUENCY:
                switch (settings.cpuFrequency) {
                    case 0: return "80 MHz";
                    case 1: return "160 MHz";
                    case 2: return "240 MHz";
                    default: return "Unknown";
                }
                
            case POWER_PROFILE:
                switch (settings.powerProfile) {
                    case 0: return "Performance";
                    case 1: return "Balanced";
                    case 2: return "Power Save";
                    default: return "Unknown";
                }
                
            case SLEEP_MODE:
                switch (settings.sleepMode) {
                    case 0: return "None";
                    case 1: return "Light";
                    case 2: return "Deep";
                    default: return "Unknown";
                }
                
            case SLEEP_TIMEOUT:
                if (settings.sleepTimeout == 0) return "Never";
                else if (settings.sleepTimeout < 60) return String(settings.sleepTimeout) + "s";
                else if (settings.sleepTimeout < 3600) return String(settings.sleepTimeout / 60) + "m";
                else return String(settings.sleepTimeout / 3600) + "h";
                
            case WATCHDOG:
                return settings.enableWatchdog ? "Enabled" : "Disabled";
                
            case LOG_LEVEL:
                switch (settings.logLevel) {
                    case 0: return "None";
                    case 1: return "Error";
                    case 2: return "Warning";
                    case 3: return "Info";
                    case 4: return "Debug";
                    default: return "Unknown";
                }
                
            case SERIAL_DEBUG:
                return settings.enableSerial ? "Enabled" : "Disabled";
                
            case OTA_UPDATES:
                return settings.enableOTA ? "Enabled" : "Disabled";
                
            case STORAGE_INFO: {
                size_t total = 0, used = 0;
                esp_spiffs_info(NULL, &total, &used);
                return String(used / 1024) + "/" + String(total / 1024) + "KB";
            }
                
            case FIRMWARE_INFO:
                return settings.firmwareVersion;
                
            default:
                return "";
        }
    }
    
    bool adjustValue(SystemMenuState item, int delta) {
        switch (item) {
            case CPU_FREQUENCY:
                settings.cpuFrequency = constrain(settings.cpuFrequency + delta, 0, 2);
                return true;
                
            case POWER_PROFILE:
                settings.powerProfile = constrain(settings.powerProfile + delta, 0, 2);
                return true;
                
            case SLEEP_MODE:
                settings.sleepMode = constrain(settings.sleepMode + delta, 0, 2);
                return true;
                
            case SLEEP_TIMEOUT: {
                uint32_t timeouts[] = {0, 30, 60, 300, 600, 1800}; // Never, 30s, 1m, 5m, 10m, 30m
                int currentIndex = 0;
                for (int i = 0; i < 6; i++) {
                    if (timeouts[i] == settings.sleepTimeout) {
                        currentIndex = i;
                        break;
                    }
                }
                currentIndex = constrain(currentIndex + delta, 0, 5);
                settings.sleepTimeout = timeouts[currentIndex];
                return true;
            }
                
            case LOG_LEVEL:
                settings.logLevel = constrain(settings.logLevel + delta, 0, 4);
                return true;
                
            default:
                return false;
        }
    }
    
    void applyCurrentSetting() {
        switch (currentSelection) {
            case CPU_FREQUENCY:
                applyCpuFrequency();
                break;
                
            case POWER_PROFILE:
                applyPowerProfile();
                break;
                
            case SLEEP_MODE:
                applySleepMode();
                break;
                
            default:
                break;
        }
    }
    
    void executeConfirmedAction() {
        switch (currentSelection) {
            case FACTORY_RESET:
                performFactoryReset();
                break;
                
            case RESTART_DEVICE:
                esp_restart();
                break;
                
            default:
                break;
        }
    }
    
    void applyCpuFrequency() {
        uint32_t frequencies[] = {80, 160, 240};
        setCpuFrequencyMhz(frequencies[settings.cpuFrequency]);
    }
    
    void applyPowerProfile() {
        // Apply power profile settings
        switch (settings.powerProfile) {
            case 0: // Performance
                setCpuFrequencyMhz(240);
                esp_pm_config_esp32_t pm_config = {
                    .max_freq_mhz = 240,
                    .min_freq_mhz = 240,
                    .light_sleep_enable = false
                };
                esp_pm_configure(&pm_config);
                break;
                
            case 1: // Balanced
                setCpuFrequencyMhz(160);
                break;
                
            case 2: // Power Save
                setCpuFrequencyMhz(80);
                break;
        }
    }
    
    void applySleepMode() {
        // Configure sleep mode settings
        if (settings.sleepMode == 0) {
            // Disable sleep
        } else if (settings.sleepMode == 1) {
            // Light sleep configuration
        } else if (settings.sleepMode == 2) {
            // Deep sleep configuration
        }
    }
    
    void performFactoryReset() {
        // Clear all settings
        // Preferences prefs;
        // prefs.begin("system", false);
        // prefs.clear();
        // prefs.end();
        
        // Clear other preference namespaces
        // (display, audio, network, etc.)
        
        // Format SPIFFS using ESP-IDF
        esp_vfs_spiffs_unregister(NULL);
        esp_partition_t* partition = (esp_partition_t*)esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL);
        if (partition) {
            esp_partition_erase_range(partition, 0, partition->size);
        }
        
        // Restart system
        esp_restart();
    }
    
    void updateSystemInfo() {
        // Update dynamic system information
        settings.firmwareVersion = "1.0.0"; // This would come from build system
    }
    
    void loadSettings() {
        // Load from persistent storage
        // Example using Preferences:
        // Preferences prefs;
        // prefs.begin("system", false);
        // settings.cpuFrequency = prefs.getUChar("cpuFreq", 2);
        // settings.sleepMode = prefs.getUChar("sleepMode", 1);
        // settings.sleepTimeout = prefs.getUInt("sleepTimeout", 300);
        // settings.enableDeepSleep = prefs.getBool("deepSleep", true);
        // settings.enableWatchdog = prefs.getBool("watchdog", true);
        // settings.logLevel = prefs.getUChar("logLevel", 2);
        // settings.enableSerial = prefs.getBool("serial", true);
        // settings.powerProfile = prefs.getUChar("powerProfile", 1);
        // settings.enableOTA = prefs.getBool("ota", true);
        // prefs.end();
    }
    
    void saveSettings() {
        // Save to persistent storage
        // Example using Preferences:
        // Preferences prefs;
        // prefs.begin("system", false);
        // prefs.putUChar("cpuFreq", settings.cpuFrequency);
        // prefs.putUChar("sleepMode", settings.sleepMode);
        // prefs.putUInt("sleepTimeout", settings.sleepTimeout);
        // prefs.putBool("deepSleep", settings.enableDeepSleep);
        // prefs.putBool("watchdog", settings.enableWatchdog);
        // prefs.putUChar("logLevel", settings.logLevel);
        // prefs.putBool("serial", settings.enableSerial);
        // prefs.putUChar("powerProfile", settings.powerProfile);
        // prefs.putBool("ota", settings.enableOTA);
        // prefs.end();
        
        // Apply settings immediately
        applySystemSettings();
    }
    
    void applySystemSettings() {
        // Apply all current settings
        applyCpuFrequency();
        applyPowerProfile();
        applySleepMode();
        
        // Configure watchdog
        if (settings.enableWatchdog) {
            // esp_task_wdt_init(30, true); // 30 second timeout
        }
        
        // Configure logging
        esp_log_level_set("*", (esp_log_level_t)settings.logLevel);
    }
};

#endif // SYSTEM_SETTINGS_H
