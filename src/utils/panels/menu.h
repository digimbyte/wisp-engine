// menu.h - Wisp Engine Menu Panel System
#pragma once
#include <LovyanGFX.hpp>
#include "definitions.h"
#include "wisp_curated_api.h"

// Menu panel interface that integrates with the curated API system
// Panels can pause app logic while keeping networking and audio active

namespace WispMenu {

// Menu panel base class
class MenuPanel {
protected:
    WispCuratedAPI* api;
    bool active;
    bool appFrozen;
    String panelName;
    
public:
    MenuPanel(const String& name) : api(nullptr), active(false), appFrozen(false), panelName(name) {}
    virtual ~MenuPanel() {}
    
    // Set API access
    void setAPI(WispCuratedAPI* apiPtr) { api = apiPtr; }
    
    // Panel lifecycle
    virtual bool init() = 0;
    virtual void update(const WispInputState& input) = 0;
    virtual void render() = 0;
    virtual void cleanup() = 0;
    
    // State management
    virtual void activate() { 
        active = true; 
        appFrozen = true;  // Default: freeze app when menu is active
    }
    
    virtual void deactivate() { 
        active = false; 
        appFrozen = false; 
    }
    
    bool isActive() const { return active; }
    bool isAppFrozen() const { return appFrozen; }
    const String& getName() const { return panelName; }
    
    // Allow panels to control app freezing
    void setAppFrozen(bool frozen) { appFrozen = frozen; }
};

// Main game menu panel
class MainMenuPanel : public MenuPanel {
private:
    struct MenuItem {
        String text;
        String appPath;
        String iconPath;
        bool isApp;
        bool isSettings;
        
        MenuItem(const String& t, const String& path = "", bool app = false, bool settings = false) :
            text(t), appPath(path), iconPath(""), isApp(app), isSettings(settings) {}
    };
    
    std::vector<MenuItem> menuItems;
    int selectedIndex;
    int scrollOffset;
    ResourceHandle backgroundSprite;
    ResourceHandle menuIcons[8]; // Icons for menu items
    
    // App info for currently selected app
    String selectedAppName;
    String selectedAppAuthor;
    String selectedAppVersion;
    ResourceHandle selectedAppIcon;
    
public:
    MainMenuPanel() : MenuPanel("Main Menu"), selectedIndex(0), scrollOffset(0), 
                      backgroundSprite(INVALID_RESOURCE), selectedAppIcon(INVALID_RESOURCE) {
        // Initialize menu icon array
        for (int i = 0; i < 8; i++) {
            menuIcons[i] = INVALID_RESOURCE;
        }
    }
    
    bool init() override {
        if (!api) return false;
        
        // Load menu background
        backgroundSprite = api->loadSprite("/ui/menu_background.spr");
        
        // Load menu icons
        menuIcons[0] = api->loadSprite("/ui/icons/launch.spr");
        menuIcons[1] = api->loadSprite("/ui/icons/display.spr");
        menuIcons[2] = api->loadSprite("/ui/icons/audio.spr");
        menuIcons[3] = api->loadSprite("/ui/icons/network.spr");
        menuIcons[4] = api->loadSprite("/ui/icons/system.spr");
        
        // Build menu items
        buildMenuItems();
        
        return true;
    }
    
    void update(const WispInputState& input) override {
        if (!active) return;
        
        // Handle menu navigation
        static bool upPressed = false;
        static bool downPressed = false;
        static bool selectPressed = false;
        
        // Up/Down navigation
        if (input.up && !upPressed) {
            selectedIndex = max(0, selectedIndex - 1);
            updateSelectedApp();
        }
        upPressed = input.up;
        
        if (input.down && !downPressed) {
            selectedIndex = min((int)menuItems.size() - 1, selectedIndex + 1);
            updateSelectedApp();
        }
        downPressed = input.down;
        
        // Selection
        if (input.buttonA && !selectPressed) {
            activateSelectedItem();
        }
        selectPressed = input.buttonA;
        
        // Back button deactivates menu (returns to app if running)
        if (input.buttonB) {
            deactivate();
        }
    }
    
    void render() override {
        if (!active) return;
        
        // Draw background
        if (backgroundSprite != INVALID_RESOURCE) {
            api->drawSprite(backgroundSprite, 0, 0, 10); // Background depth
        } else {
            // Simple gradient background
            api->drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WispColor(20, 30, 50), 10);
        }
        
        // Draw title
        api->drawText("WISP ENGINE", SCREEN_WIDTH / 2 - 50, 20, WispColor(255, 255, 255), 2);
        
        // Draw menu items
        renderMenuItems();
        
        // Draw selected app info
        renderAppInfo();
        
        // Draw navigation hints
        api->drawText("A: Select  B: Back", 10, SCREEN_HEIGHT - 20, WispColor(200, 200, 200), 1);
    }
    
    void cleanup() override {
        // Unload resources
        api->unloadSprite(backgroundSprite);
        api->unloadSprite(selectedAppIcon);
        
        for (int i = 0; i < 8; i++) {
            api->unloadSprite(menuIcons[i]);
        }
    }
    
private:
    void buildMenuItems() {
        menuItems.clear();
        
        // Add detected apps (this would come from app scanner)
        menuItems.push_back(MenuItem("Launch Game", "/apps/platformer.wapp", true));
        menuItems.push_back(MenuItem("Snake Game", "/apps/snake.wapp", true));
        menuItems.push_back(MenuItem("Demo App", "/apps/demo.wapp", true));
        
        // Add settings panels
        menuItems.push_back(MenuItem("Display Settings", "", false, true));
        menuItems.push_back(MenuItem("Audio Settings", "", false, true));
        menuItems.push_back(MenuItem("Network Settings", "", false, true));
        menuItems.push_back(MenuItem("System Settings", "", false, true));
    }
    
    void renderMenuItems() {
        int startY = 60;
        int itemHeight = 25;
        int maxVisible = 6;
        
        // Calculate scroll offset
        if (selectedIndex >= scrollOffset + maxVisible) {
            scrollOffset = selectedIndex - maxVisible + 1;
        } else if (selectedIndex < scrollOffset) {
            scrollOffset = selectedIndex;
        }
        
        // Render visible menu items
        for (int i = 0; i < maxVisible && (scrollOffset + i) < (int)menuItems.size(); i++) {
            int itemIndex = scrollOffset + i;
            const MenuItem& item = menuItems[itemIndex];
            
            int y = startY + i * itemHeight;
            bool selected = (itemIndex == selectedIndex);
            
            // Draw selection highlight
            if (selected) {
                api->drawRect(10, y - 2, SCREEN_WIDTH - 20, itemHeight - 2, 
                             WispColor(100, 150, 255, 128), 3);
            }
            
            // Draw icon
            ResourceHandle icon = INVALID_RESOURCE;
            if (item.isApp && menuIcons[0] != INVALID_RESOURCE) {
                icon = menuIcons[0]; // Launch icon
            } else if (item.isSettings) {
                if (item.text.indexOf("Display") >= 0) icon = menuIcons[1];
                else if (item.text.indexOf("Audio") >= 0) icon = menuIcons[2];
                else if (item.text.indexOf("Network") >= 0) icon = menuIcons[3];
                else if (item.text.indexOf("System") >= 0) icon = menuIcons[4];
            }
            
            if (icon != INVALID_RESOURCE) {
                api->drawSprite(icon, 15, y + 2, 2);
            }
            
            // Draw text
            WispColor textColor = selected ? WispColor(255, 255, 255) : WispColor(200, 200, 200);
            api->drawText(item.text, 50, y + 5, textColor, 2);
        }
        
        // Draw scroll indicators if needed
        if (scrollOffset > 0) {
            api->drawText("▲", SCREEN_WIDTH - 20, startY - 10, WispColor(150, 150, 150), 1);
        }
        if (scrollOffset + maxVisible < (int)menuItems.size()) {
            api->drawText("▼", SCREEN_WIDTH - 20, startY + maxVisible * itemHeight, 
                         WispColor(150, 150, 150), 1);
        }
    }
    
    void renderAppInfo() {
        if (selectedIndex < 0 || selectedIndex >= (int)menuItems.size()) return;
        
        const MenuItem& item = menuItems[selectedIndex];
        if (!item.isApp) return;
        
        // Draw app info panel on the right side
        int panelX = SCREEN_WIDTH - 120;
        int panelY = 80;
        int panelWidth = 110;
        int panelHeight = 100;
        
        // Background panel
        api->drawRect(panelX, panelY, panelWidth, panelHeight, 
                     WispColor(30, 40, 60, 200), 4);
        
        // App icon
        if (selectedAppIcon != INVALID_RESOURCE) {
            api->drawSprite(selectedAppIcon, panelX + 10, panelY + 10, 2);
        } else {
            // Placeholder icon
            api->drawRect(panelX + 10, panelY + 10, 32, 32, WispColor(100, 100, 100), 2);
        }
        
        // App name
        api->drawText(selectedAppName, panelX + 50, panelY + 15, WispColor(255, 255, 255), 1);
        
        // App author
        api->drawText("by " + selectedAppAuthor, panelX + 10, panelY + 50, 
                     WispColor(180, 180, 180), 1);
        
        // App version
        api->drawText("v" + selectedAppVersion, panelX + 10, panelY + 70, 
                     WispColor(150, 150, 150), 1);
    }
    
    void updateSelectedApp() {
        if (selectedIndex < 0 || selectedIndex >= (int)menuItems.size()) return;
        
        const MenuItem& item = menuItems[selectedIndex];
        if (item.isApp) {
            // Load app metadata (this would parse app manifest)
            selectedAppName = item.text;
            selectedAppAuthor = "Unknown";
            selectedAppVersion = "1.0";
            
            // Try to load app icon
            if (selectedAppIcon != INVALID_RESOURCE) {
                api->unloadSprite(selectedAppIcon);
            }
            selectedAppIcon = api->loadSprite(item.appPath + "/icon.spr");
        }
    }
    
    void activateSelectedItem() {
        if (selectedIndex < 0 || selectedIndex >= (int)menuItems.size()) return;
        
        const MenuItem& item = menuItems[selectedIndex];
        
        if (item.isApp) {
            // Launch app
            api->print("Launching app: " + item.text);
            // TODO: Trigger app launch through bootloader
            deactivate();
            
        } else if (item.isSettings) {
            // Open settings panel
            openSettingsPanel(item.text);
        }
    }
    
    void openSettingsPanel(const String& panelName) {
        api->print("Opening settings panel: " + panelName);
        // TODO: Switch to appropriate settings panel
        // For now, just show a message
        deactivate();
    }
};

// Global menu system
static MainMenuPanel* mainMenu = nullptr;
static MenuPanel* currentPanel = nullptr;

// Initialize menu system
inline bool init(WispCuratedAPI* api) {
    if (!api) return false;
    
    mainMenu = new MainMenuPanel();
    mainMenu->setAPI(api);
    
    if (!mainMenu->init()) {
        delete mainMenu;
        mainMenu = nullptr;
        return false;
    }
    
    currentPanel = mainMenu;
    return true;
}

// Activate main menu
inline void activate() {
    if (mainMenu) {
        currentPanel = mainMenu;
        currentPanel->activate();
    }
}

// Deactivate current panel
inline void deactivate() {
    if (currentPanel) {
        currentPanel->deactivate();
        currentPanel = nullptr;
    }
}

// Check if any menu is active
inline bool isActive() {
    return currentPanel && currentPanel->isActive();
}

// Check if app should be frozen
inline bool isAppFrozen() {
    return currentPanel && currentPanel->isAppFrozen();
}

// Update current panel
inline void update(const WispInputState& input) {
    if (currentPanel && currentPanel->isActive()) {
        currentPanel->update(input);
    }
}

// Render current panel
inline void render() {
    if (currentPanel && currentPanel->isActive()) {
        currentPanel->render();
    }
}

// Cleanup menu system
inline void cleanup() {
    if (mainMenu) {
        mainMenu->cleanup();
        delete mainMenu;
        mainMenu = nullptr;
    }
    currentPanel = nullptr;
}

} // namespace WispMenu
