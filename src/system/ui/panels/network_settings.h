// network_settings.h - Network Settings Panel for Wisp Engine
#ifndef NETWORK_SETTINGS_H
#define NETWORK_SETTINGS_H

#include "menu.h"
#include "../../system/definitions.h"
#include "../../system/bluetooth_manager.h"
#include "../../system/settings_manager.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "lwip/ip4_addr.h"
#include <vector>
#include <string>

// WiFi connection status function
bool isConnected() {
    wifi_ap_record_t ap_info;
    return esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK;
}

class NetworkSettingsPanel : public MenuPanel {
private:
    struct NetworkSettings {
        std::string ssid = "";
        std::string password = "";
        bool autoConnect = true;
        bool enableBluetooth = true;
        bool enableHotspot = false;
        std::string hotspotName = "WispEngine";
        std::string hotspotPassword = "wisp1234";
        uint8_t wifiPower = 20;         // WiFi TX power (0-20)
        bool enableMDNS = true;
        std::string deviceName = "wisp-engine";
        bool bluetoothAudioStreaming = false;
    } settings;
    
    enum NetworkMenuState {
        WIFI_STATUS,
        WIFI_SCAN,
        WIFI_CONNECT,
        WIFI_DISCONNECT,
        WIFI_POWER,
        AUTO_CONNECT,
        BLUETOOTH_STATUS,
        BLUETOOTH_TOGGLE,
        BLUETOOTH_AUDIO,
        HOTSPOT_STATUS,
        HOTSPOT_TOGGLE,
        HOTSPOT_CONFIG,
        DEVICE_NAME,
        MDNS_TOGGLE,
        NETWORK_INFO,
        SAVE_SETTINGS,
        NETWORK_MENU_COUNT
    };
    
    NetworkMenuState currentSelection = WIFI_STATUS;
    bool inConfigMode = false;
    bool isScanning = false;
    uint32_t lastScanTime = 0;
    
    char availableNetworks[10][64]; // Max 10 networks, 64 chars each
    int networkCount;
    int selectedNetwork = -1;
    
    const char* menuItems[NETWORK_MENU_COUNT] = {
        "WiFi Status",
        "Scan Networks",
        "Connect WiFi",
        "Disconnect WiFi",
        "WiFi Power",
        "Auto Connect",
        "Bluetooth Status",
        "Toggle Bluetooth",
        "Bluetooth Audio",
        "Hotspot Status",
        "Toggle Hotspot",
        "Hotspot Config",
        "Device Name",
        "Enable mDNS",
        "Network Info",
        "Save & Exit"
    };

public:
    NetworkSettingsPanel(WispCuratedAPI* api) : MenuPanel(api) {
        loadSettings();
    }
    
    void activate() override {
        MenuPanel::activate();
        currentSelection = WIFI_STATUS;
        inConfigMode = false;
        loadSettings();
        refreshNetworkStatus();
    }
    
    void update(const WispInputState& input) override {
        if (!isActive()) return;
        
        if (inConfigMode) {
            handleConfiguration(input);
        } else {
            handleNavigation(input);
        }
        
        // Update WiFi scan if in progress
        if (isScanning) {
            updateWiFiScan();
        }
        
        // Auto-save periodically
        static uint32_t lastSaveTime = 0;
        if (get_millis() - lastSaveTime > 10000) { // Every 10 seconds
            saveSettings();
            lastSaveTime = get_millis();
        }
    }
    
    void render() override {
        if (!isActive()) return;
        
        auto* gfx = api->graphics();
        
        // Clear background
        gfx->fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_DARK_GREEN);
        
        // Title
        gfx->setTextColor(COLOR_WHITE);
        gfx->setTextSize(2);
        gfx->drawText("NETWORK SETTINGS", SCREEN_WIDTH / 2, 15, true);
        
        if (inConfigMode) {
            renderConfiguration();
        } else {
            renderMainMenu();
        }
        
        // Status indicators
        renderStatusIndicators();
    }
    
private:
    void handleNavigation(const WispInputState& input) {
        static uint32_t lastInputTime = 0;
        uint32_t currentTime = get_millis();
        
        if (currentTime - lastInputTime < 150) return; // Debounce
        
        if (input.up) {
            currentSelection = (NetworkMenuState)((currentSelection - 1 + NETWORK_MENU_COUNT) % NETWORK_MENU_COUNT);
            lastInputTime = currentTime;
        } else if (input.down) {
            currentSelection = (NetworkMenuState)((currentSelection + 1) % NETWORK_MENU_COUNT);
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
        uint32_t currentTime = get_millis();
        
        if (currentTime - lastInputTime < 150) return;
        
        if (input.buttonB) {
            inConfigMode = false;
            lastInputTime = currentTime;
        } else if (input.buttonA || input.select) {
            executeConfigAction();
            lastInputTime = currentTime;
        } else if (input.up || input.down) {
            navigateConfigOptions(input.up ? -1 : 1);
            lastInputTime = currentTime;
        }
    }
    
    void handleMenuAction() {
        switch (currentSelection) {
            case WIFI_SCAN:
                startWiFiScan();
                break;
                
            case WIFI_CONNECT:
                if (!availableNetworks.empty()) {
                    inConfigMode = true;
                    selectedNetwork = 0;
                }
                break;
                
            case WIFI_DISCONNECT:
                esp_wifi_disconnect();
                break;
                
            case WIFI_POWER:
                inConfigMode = true;
                break;
                
            case AUTO_CONNECT:
                settings.autoConnect = !settings.autoConnect;
                break;
                
            case BLUETOOTH_TOGGLE:
                settings.enableBluetooth = !settings.enableBluetooth;
                toggleBluetooth();
                break;
                
            case BLUETOOTH_AUDIO:
                inConfigMode = true;
                break;
                
            case HOTSPOT_TOGGLE:
                settings.enableHotspot = !settings.enableHotspot;
                toggleHotspot();
                break;
                
            case HOTSPOT_CONFIG:
                inConfigMode = true;
                break;
                
            case DEVICE_NAME:
                inConfigMode = true;
                break;
                
            case MDNS_TOGGLE:
                settings.enableMDNS = !settings.enableMDNS;
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
        const int itemHeight = 18;
        
        for (int i = 0; i < NETWORK_MENU_COUNT; i++) {
            int y = startY + i * itemHeight;
            
            // Skip items that don't fit on screen
            if (y > SCREEN_HEIGHT - 30) break;
            
            // Highlight current selection
            if (i == currentSelection) {
                gfx->fillRect(5, y - 2, SCREEN_WIDTH - 10, itemHeight - 2, COLOR_LIGHT_GREEN);
                gfx->setTextColor(COLOR_BLACK);
            } else {
                gfx->setTextColor(COLOR_WHITE);
            }
            
            // Draw menu item and value
            gfx->drawText(menuItems[i], 10, y + 3, false);
            
            std::string valueText = getStatusText((NetworkMenuState)i);
            if (!valueText.empty()) {
                gfx->drawText(valueText.c_str(), SCREEN_WIDTH - 10, y + 3, false, true);
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
        
        switch (currentSelection) {
            case WIFI_CONNECT:
                renderWiFiSelection();
                break;
                
            case WIFI_POWER:
                renderPowerConfiguration();
                break;
                
            case HOTSPOT_CONFIG:
                renderHotspotConfiguration();
                break;
                
            case DEVICE_NAME:
                renderDeviceNameConfiguration();
                break;
                
            case BLUETOOTH_AUDIO:
                renderBluetoothAudioConfig();
                break;
                
            default:
                inConfigMode = false;
                break;
        }
    }
    
    void renderWiFiSelection() {
        auto* gfx = api->graphics();
        
        gfx->drawText("Select WiFi Network:", 10, 50, false);
        
        const int startY = 70;
        const int itemHeight = 15;
        
        for (int i = 0; i < availableNetworks.size() && i < 8; i++) {
            int y = startY + i * itemHeight;
            
            if (i == selectedNetwork) {
                gfx->fillRect(5, y - 2, SCREEN_WIDTH - 10, itemHeight - 2, COLOR_YELLOW);
                gfx->setTextColor(COLOR_BLACK);
            } else {
                gfx->setTextColor(COLOR_WHITE);
            }
            
            // Truncate long SSIDs
            std::string displayName = availableNetworks[i];
            if (displayName.length() > 25) {
                displayName = displayName.substr(0, 22) + "...";
            }
            
            gfx->drawText(displayName.c_str(), 10, y, false);
            
            // Signal strength indicator (simplified)
            int rssi = -50; // Default fallback
            
            // Get RSSI from ESP-IDF if we can scan
            wifi_ap_record_t ap_info;
            if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
                rssi = ap_info.rssi;
            }
            
            std::string signal = "";
            if (rssi > -50) signal = "****";
            else if (rssi > -60) signal = "*** ";
            else if (rssi > -70) signal = "**  ";
            else signal = "*   ";
            
            gfx->drawText(signal.c_str(), SCREEN_WIDTH - 40, y, false);
        }
        
        gfx->setTextColor(COLOR_LIGHT_GRAY);
        gfx->drawText("UP/DOWN: Select | SELECT: Connect | BACK: Cancel", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 12, true);
    }
    
    void renderPowerConfiguration() {
        auto* gfx = api->graphics();
        
        gfx->drawText("WiFi Power Level", 10, 50, false);
        
        // Power level bar
        const int barX = 20;
        const int barY = 80;
        const int barWidth = SCREEN_WIDTH - 40;
        const int barHeight = 20;
        
        gfx->drawRect(barX, barY, barWidth, barHeight, COLOR_WHITE);
        
        int fillWidth = (settings.wifiPower * barWidth) / 20;
        gfx->fillRect(barX + 1, barY + 1, fillWidth, barHeight - 2, COLOR_GREEN);
        
        // Current value
        gfx->drawText(("Power: " + std::to_string(settings.wifiPower) + " dBm").c_str(), SCREEN_WIDTH / 2, barY + barHeight + 10, true);
        
        // Range indicators
        gfx->setTextColor(COLOR_LIGHT_GRAY);
        gfx->drawText("Low", barX, barY + barHeight + 25, false);
        gfx->drawText("High", barX + barWidth, barY + barHeight + 25, false, true);
        
        gfx->drawText("LEFT/RIGHT: Adjust | SELECT: Confirm | BACK: Cancel", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 12, true);
    }
    
    void renderHotspotConfiguration() {
        auto* gfx = api->graphics();
        
        gfx->drawText("Hotspot Configuration", 10, 50, false);
        gfx->drawText(("Name: " + settings.hotspotName).c_str(), 10, 70, false);
        gfx->drawText(("Password: " + settings.hotspotPassword).c_str(), 10, 90, false);
        
        gfx->setTextColor(COLOR_LIGHT_GRAY);
        gfx->drawText("Configuration requires text input", SCREEN_WIDTH / 2, 120, true);
        gfx->drawText("Use mobile app or web interface", SCREEN_WIDTH / 2, 135, true);
        gfx->drawText("BACK: Return to menu", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 12, true);
    }
    
    void renderDeviceNameConfiguration() {
        auto* gfx = api->graphics();
        
        gfx->drawText("Device Name", 10, 50, false);
        gfx->drawText(("Current: " + settings.deviceName).c_str(), 10, 70, false);
        
        gfx->setTextColor(COLOR_LIGHT_GRAY);
        gfx->drawText("Name configuration requires", SCREEN_WIDTH / 2, 100, true);
        gfx->drawText("text input interface", SCREEN_WIDTH / 2, 115, true);
        gfx->drawText("BACK: Return to menu", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 12, true);
    }
    
    void renderBluetoothAudioConfig() {
        auto* gfx = api->graphics();
        
        gfx->drawText("Bluetooth Audio Configuration", 10, 50, false);
        
#ifdef WISP_HAS_BTE
        // Check if Bluetooth is enabled
        if (!settings.enableBluetooth) {
            gfx->setTextColor(COLOR_ORANGE);
            gfx->drawText("Bluetooth is disabled", 10, 80, false);
            gfx->drawText("Enable Bluetooth first", 10, 95, false);
            
            gfx->setTextColor(COLOR_LIGHT_GRAY);
            gfx->drawText("BACK: Return to menu", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 12, true);
            return;
        }
        
        // Check for Bluetooth Classic support
        gfx->drawText("Bluetooth Classic: Available", 10, 75, false);
        
        // Simulated device connection status
        bool deviceConnected = false; // TODO: Check actual BT connection
        if (deviceConnected) {
            gfx->setTextColor(COLOR_GREEN);
            gfx->drawText("Device: Connected", 10, 95, false);
            gfx->drawText("Name: Unknown Audio Device", 10, 110, false); // TODO: Get device name
            
            // Audio streaming toggle
            gfx->setTextColor(COLOR_WHITE);
            if (settings.bluetoothAudioStreaming) {
                gfx->setTextColor(COLOR_GREEN);
                gfx->drawText("[X] Audio Streaming", 10, 130, false);
            } else {
                gfx->drawText("[ ] Audio Streaming", 10, 130, false);
            }
            
            gfx->setTextColor(COLOR_LIGHT_GRAY);
            gfx->drawText("SELECT: Toggle Streaming | BACK: Exit", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 12, true);
        } else {
            gfx->setTextColor(COLOR_ORANGE);
            gfx->drawText("Device: Not Connected", 10, 95, false);
            gfx->drawText("Pair a Bluetooth audio device", 10, 110, false);
            gfx->drawText("to enable audio streaming", 10, 125, false);
            
            gfx->setTextColor(COLOR_LIGHT_GRAY);
            gfx->drawText("BACK: Return to menu", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 12, true);
        }
#else
        // ESP32-C6 or board without Bluetooth Classic support
        gfx->setTextColor(COLOR_RED);
        gfx->drawText("Bluetooth Classic: Not Supported", 10, 75, false);
        gfx->drawText("This board only supports BLE", 10, 95, false);
        gfx->drawText("Audio streaming requires BT Classic", 10, 110, false);
        
        gfx->setTextColor(COLOR_LIGHT_GRAY);
        gfx->drawText("BACK: Return to menu", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 12, true);
#endif
    }
    
    void renderStatusIndicators() {
        auto* gfx = api->graphics();
        
        // WiFi status indicator
        int x = SCREEN_WIDTH - 50;
        int y = 35;
        
        if (isConnected()) {
            gfx->fillCircle(x, y, 5, COLOR_GREEN);
            gfx->setTextColor(COLOR_GREEN);
            gfx->setTextSize(1);
            gfx->drawText("WiFi", x, y + 8, true);
        } else {
            gfx->fillCircle(x, y, 5, COLOR_RED);
            gfx->setTextColor(COLOR_RED);
            gfx->setTextSize(1);
            gfx->drawText("WiFi", x, y + 8, true);
        }
        
        // Bluetooth status indicator
        x -= 30;
        if (settings.enableBluetooth) {
            gfx->fillCircle(x, y, 5, COLOR_BLUE);
            gfx->setTextColor(COLOR_BLUE);
            gfx->drawText("BT", x, y + 8, true);
        } else {
            gfx->fillCircle(x, y, 5, COLOR_GRAY);
            gfx->setTextColor(COLOR_GRAY);
            gfx->drawText("BT", x, y + 8, true);
        }
    }
    
    String getStatusText(NetworkMenuState item) {
        switch (item) {
            case WIFI_STATUS: {
                wifi_ap_record_t ap_info;
                if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
                    return "Connected: " + String((char*)ap_info.ssid);
                } else {
                    return "Disconnected";
                }
            }
                
            case WIFI_POWER:
                return String(settings.wifiPower) + " dBm";
                
            case AUTO_CONNECT:
                return settings.autoConnect ? "Enabled" : "Disabled";
                
            case BLUETOOTH_STATUS:
                return settings.enableBluetooth ? "Enabled" : "Disabled";
                
            case BLUETOOTH_AUDIO: {
                // Use BluetoothManager for status - efficient caching
                return BluetoothManager::getInstance().getStatusString().c_str();
            }
                
            case HOTSPOT_STATUS:
                return settings.enableHotspot ? "Active" : "Inactive";
                
            case DEVICE_NAME:
                return settings.deviceName;
                
            case MDNS_TOGGLE:
                return settings.enableMDNS ? "Enabled" : "Disabled";
                
            case NETWORK_INFO: {
                esp_netif_ip_info_t ip_info;
                esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
                if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
                    return String(ip4addr_ntoa((ip4_addr_t*)&ip_info.ip));
                } else {
                    return "No IP";
                }
            }
                
            default:
                return "";
        }
    }
    
    void startWiFiScan() {
        if (isScanning) return;
        
        isScanning = true;
        lastScanTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
        availableNetworks.clear();
        
        // Start ESP-IDF WiFi scan
        wifi_scan_config_t scan_config = {};
        scan_config.ssid = NULL;
        scan_config.bssid = NULL;
        scan_config.channel = 0;
        scan_config.show_hidden = false;
        scan_config.scan_type = WIFI_SCAN_TYPE_ACTIVE;
        scan_config.scan_time.active.min = 100;
        scan_config.scan_time.active.max = 300;
        
        esp_wifi_scan_start(&scan_config, false); // Non-blocking scan
    }
    
    void updateWiFiScan() {
        if (!isScanning) return;
        
        uint16_t ap_count = 0;
        esp_wifi_scan_get_ap_num(&ap_count);
        
        if (ap_count > 0) {
            // Scan complete
            wifi_ap_record_t* ap_records = (wifi_ap_record_t*)malloc(ap_count * sizeof(wifi_ap_record_t));
            if (ap_records) {
                esp_wifi_scan_get_ap_records(&ap_count, ap_records);
                
                availableNetworks.clear();
                for (int i = 0; i < ap_count; i++) {
                    availableNetworks.push_back(String((char*)ap_records[i].ssid));
                }
                
                free(ap_records);
            }
            isScanning = false;
        }
    }
    
    void navigateConfigOptions(int direction) {
        switch (currentSelection) {
            case WIFI_CONNECT:
                if (!availableNetworks.empty()) {
                    selectedNetwork = (selectedNetwork + direction + availableNetworks.size()) % availableNetworks.size();
                }
                break;
                
            case WIFI_POWER:
                settings.wifiPower = constrain(settings.wifiPower + direction, 0, 20);
                break;
                
            default:
                break;
        }
    }
    
    void executeConfigAction() {
        switch (currentSelection) {
            case WIFI_CONNECT:
                if (selectedNetwork >= 0 && selectedNetwork < availableNetworks.size()) {
                    connectToWiFi(availableNetworks[selectedNetwork]);
                }
                inConfigMode = false;
                break;
                
            case WIFI_POWER:
                esp_wifi_set_max_tx_power(settings.wifiPower);
                inConfigMode = false;
                break;
                
            case BLUETOOTH_AUDIO:
#ifdef WISP_HAS_BTE
                if (settings.enableBluetooth) {
                    // Toggle Bluetooth audio streaming if device is connected
                    bool deviceConnected = false; // TODO: Check actual BT connection
                    if (deviceConnected) {
                        settings.bluetoothAudioStreaming = !settings.bluetoothAudioStreaming;
                        // TODO: Start/stop actual audio streaming to BT device
                        if (settings.bluetoothAudioStreaming) {
                            // Start streaming audio to Bluetooth device
                        } else {
                            // Stop streaming audio to Bluetooth device
                        }
                    }
                }
#endif
                inConfigMode = false;
                break;
                
            default:
                inConfigMode = false;
                break;
        }
    }
    
    void connectToWiFi(const String& ssid) {
        // In a real implementation, this would need password input
        // For now, try connecting with saved password or open network
        wifi_config_t wifi_config = {};
        strncpy((char*)wifi_config.sta.ssid, ssid.c_str(), sizeof(wifi_config.sta.ssid));
        strncpy((char*)wifi_config.sta.password, settings.password.c_str(), sizeof(wifi_config.sta.password));
        
        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
        esp_wifi_start();
        esp_wifi_connect();
        
        // The connection attempt will be monitored in the status display
    }
    
    void toggleBluetooth() {
        // Use BluetoothManager for efficient BT handling
        BluetoothManager::getInstance().setEnabled(settings.enableBluetooth);
    }
    
    void toggleHotspot() {
        if (settings.enableHotspot) {
            // Enable WiFi AP mode
            wifi_config_t ap_config = {};
            strncpy((char*)ap_config.ap.ssid, settings.hotspotName.c_str(), sizeof(ap_config.ap.ssid));
            strncpy((char*)ap_config.ap.password, settings.hotspotPassword.c_str(), sizeof(ap_config.ap.password));
            ap_config.ap.max_connection = 4;
            ap_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
            
            esp_wifi_set_mode(WIFI_MODE_AP);
            esp_wifi_set_config(WIFI_IF_AP, &ap_config);
            esp_wifi_start();
        } else {
            esp_wifi_stop();
            esp_wifi_set_mode(WIFI_MODE_STA);
        }
    }
    
    void refreshNetworkStatus() {
        // Update current network status
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            settings.ssid = String((char*)ap_info.ssid);
        }
    }
    
    void loadSettings() {
        // Load settings from SettingsManager (handles flash storage automatically)
        auto& settingsManager = WispEngine::System::SettingsManager::getInstance();
        
        // Load network settings
        settings.ssid = settingsManager.getWiFiSSID();
        settings.password = settingsManager.getWiFiPassword();
        settings.autoConnect = settingsManager.getWiFiAutoConnect();
        settings.wifiPower = settingsManager.getWiFiPower();
        
        // Load Bluetooth settings
        settings.enableBluetooth = settingsManager.getBluetoothEnabled();
        settings.bluetoothAudioStreaming = settingsManager.getBluetoothAudioStreaming();
        
        // Load hotspot settings
        settings.enableHotspot = settingsManager.getHotspotEnabled();
        settings.hotspotName = settingsManager.getHotspotName();
        settings.hotspotPassword = settingsManager.getHotspotPassword();
        
        // Load system settings
        settings.deviceName = settingsManager.getDeviceName();
        settings.enableMDNS = settingsManager.getMDNSEnabled();
        
        // Handle any load errors gracefully
        if (settingsManager.getLastError() != WispEngine::System::SettingsError::SUCCESS) {
            // Settings load failed - show error or use current defaults
            ESP_LOGW("Settings", "Settings load error: %s", 
                    settingsManager.getErrorString(settingsManager.getLastError()).c_str());
        }
    }
    
    void saveSettings() {
        // Save settings using SettingsManager (handles flash storage automatically)
        auto& settingsManager = WispEngine::System::SettingsManager::getInstance();
        
        // Save network settings
        settingsManager.setWiFiSSID(settings.ssid);
        settingsManager.setWiFiPassword(settings.password);
        settingsManager.setWiFiAutoConnect(settings.autoConnect);
        settingsManager.setWiFiPower(settings.wifiPower);
        
        // Save Bluetooth settings
        settingsManager.setBluetoothEnabled(settings.enableBluetooth);
        settingsManager.setBluetoothAudioStreaming(settings.bluetoothAudioStreaming);
        
        // Save hotspot settings
        settingsManager.setHotspotEnabled(settings.enableHotspot);
        settingsManager.setHotspotName(settings.hotspotName);
        settingsManager.setHotspotPassword(settings.hotspotPassword);
        
        // Save system settings
        settingsManager.setDeviceName(settings.deviceName);
        settingsManager.setMDNSEnabled(settings.enableMDNS);
        
        // Commit all changes to flash storage
        auto result = settingsManager.saveSettings();
        if (result != WispEngine::System::SettingsError::SUCCESS) {
            // Handle save errors gracefully
            if (result == WispEngine::System::SettingsError::FLASH_READ_ONLY) {
                ESP_LOGW("Settings", "Flash is read-only - settings not saved");
                // TODO: Show user notification that flash is read-only
            } else if (result == WispEngine::System::SettingsError::OUT_OF_SPACE) {
                ESP_LOGW("Settings", "Flash storage full - settings not saved");
                // TODO: Show user notification about storage space
            } else {
                ESP_LOGE("Settings", "Settings save error: %s", 
                        settingsManager.getErrorString(result).c_str());
                // TODO: Show generic save error to user
            }
        } else {
            ESP_LOGI("Settings", "Network settings saved successfully");
        }
        
        // Apply settings to network systems
        applyNetworkSettings();
    }
    
    void applyNetworkSettings() {
        // Apply current settings to network systems
        esp_wifi_set_max_tx_power(settings.wifiPower);
        
        if (settings.autoConnect && !settings.ssid.empty()) {
            wifi_config_t wifi_config = {};
            strncpy((char*)wifi_config.sta.ssid, settings.ssid.c_str(), sizeof(wifi_config.sta.ssid));
            strncpy((char*)wifi_config.sta.password, settings.password.c_str(), sizeof(wifi_config.sta.password));
            
            esp_wifi_set_mode(WIFI_MODE_STA);
            esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
            esp_wifi_start();
            esp_wifi_connect();
        }
        
        if (settings.enableMDNS) {
            // MDNS.begin(settings.deviceName.c_str());
        }
    }
};

#endif // NETWORK_SETTINGS_H
