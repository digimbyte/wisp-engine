// network_settings.h - Network Settings Panel for Wisp Engine
#ifndef NETWORK_SETTINGS_H
#define NETWORK_SETTINGS_H

#include "menu.h"
#include "../../system/definitions.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "lwip/ip4_addr.h"
#include <vector>

// WiFi connection status function
bool isConnected() {
    wifi_ap_record_t ap_info;
    return esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK;
}

class NetworkSettingsPanel : public MenuPanel {
private:
    struct NetworkSettings {
        String ssid = "";
        String password = "";
        bool autoConnect = true;
        bool enableBluetooth = true;
        bool enableHotspot = false;
        String hotspotName = "WispEngine";
        String hotspotPassword = "wisp1234";
        uint8_t wifiPower = 20;         // WiFi TX power (0-20)
        bool enableMDNS = true;
        String deviceName = "wisp-engine";
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
    
    std::vector<String> availableNetworks;
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
        if (millis() - lastSaveTime > 10000) { // Every 10 seconds
            saveSettings();
            lastSaveTime = millis();
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
        uint32_t currentTime = millis();
        
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
        uint32_t currentTime = millis();
        
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
            
            String valueText = getStatusText((NetworkMenuState)i);
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
            String displayName = availableNetworks[i];
            if (displayName.length() > 25) {
                displayName = displayName.substring(0, 22) + "...";
            }
            
            gfx->drawText(displayName.c_str(), 10, y, false);
            
            // Signal strength indicator (simplified)
            int rssi = -50; // Default fallback
            
            // Get RSSI from ESP-IDF if we can scan
            wifi_ap_record_t ap_info;
            if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
                rssi = ap_info.rssi;
            }
            
            String signal = "";
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
        gfx->drawText(("Power: " + String(settings.wifiPower) + " dBm").c_str(), SCREEN_WIDTH / 2, barY + barHeight + 10, true);
        
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
        if (settings.enableBluetooth) {
            // Enable ESP-IDF Bluetooth Classic/BLE
            // esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
        } else {
            // Disable ESP-IDF Bluetooth
            // esp_bt_controller_disable();
        }
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
        // Load from persistent storage
        // Example using Preferences:
        // Preferences prefs;
        // prefs.begin("network", false);
        // settings.ssid = prefs.getString("ssid", "");
        // settings.password = prefs.getString("password", "");
        // settings.autoConnect = prefs.getBool("autoConn", true);
        // settings.enableBluetooth = prefs.getBool("bluetooth", true);
        // settings.enableHotspot = prefs.getBool("hotspot", false);
        // settings.hotspotName = prefs.getString("hsName", "WispEngine");
        // settings.hotspotPassword = prefs.getString("hsPass", "wisp1234");
        // settings.wifiPower = prefs.getUChar("wifiPwr", 20);
        // settings.enableMDNS = prefs.getBool("mdns", true);
        // settings.deviceName = prefs.getString("deviceName", "wisp-engine");
        // prefs.end();
    }
    
    void saveSettings() {
        // Save to persistent storage
        // Example using Preferences:
        // Preferences prefs;
        // prefs.begin("network", false);
        // prefs.putString("ssid", settings.ssid);
        // prefs.putString("password", settings.password);
        // prefs.putBool("autoConn", settings.autoConnect);
        // prefs.putBool("bluetooth", settings.enableBluetooth);
        // prefs.putBool("hotspot", settings.enableHotspot);
        // prefs.putString("hsName", settings.hotspotName);
        // prefs.putString("hsPass", settings.hotspotPassword);
        // prefs.putUChar("wifiPwr", settings.wifiPower);
        // prefs.putBool("mdns", settings.enableMDNS);
        // prefs.putString("deviceName", settings.deviceName);
        // prefs.end();
        
        // Apply settings
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
