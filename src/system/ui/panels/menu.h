// menu.h - Base Menu Panel class for Wisp Engine
#ifndef MENU_PANEL_H
#define MENU_PANEL_H

#include "../../../engine/app/curated_api.h"
#include "../../esp32_common.h"
#include "../../system/definitions.h"

// Base class for all menu panels
class MenuPanel {
protected:
    WispCuratedAPI* api;
    bool active;
    std::string panelTitle;
    
public:
    MenuPanel(WispCuratedAPI* apiPtr) : api(apiPtr), active(false) {}
    MenuPanel(const std::string& title) : api(nullptr), active(false), panelTitle(title) {}
    
    virtual ~MenuPanel() {}
    
    // Core panel methods
    virtual void activate() { active = true; }
    virtual void deactivate() { active = false; }
    virtual bool isActive() const { return active; }
    
    // Must be implemented by derived classes
    virtual void update(const WispInputState& input) = 0;
    virtual void render() = 0;
    
    // Optional overrides
    virtual void cleanup() {}
    virtual bool init() { return true; }
    
    // Utility methods
    virtual const char* getTitle() const { return panelTitle.c_str(); }
    virtual void setAPI(WispCuratedAPI* apiPtr) { api = apiPtr; }
};

#endif // MENU_PANEL_H
