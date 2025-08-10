# Wisp Engine Asset Mapping & Integration

This document defines the standard asset mapping for the Wisp Engine and how it integrates with the SecureROMLoader's adaptive memory management system.

## 🎨 **Core Asset Structure**

### **Entity Assets** (`assets/art/entitity/`)
- **`npc.png` → `npc.spr`**
  - **Purpose**: Scripted entities with drivers/complex behavior
  - **Usage**: Player characters, NPCs, enemies, interactive objects
  - **Script Integration**: Entities using `npc.spr` typically have attached scripts
  - **Memory Profile**: Higher priority for caching (CACHED category in WispSegmentedLoader)

- **`item.png` → `item.spr`**
  - **Purpose**: Simple entities without drivers
  - **Usage**: Collectibles, static objects, environmental items, power-ups
  - **Script Integration**: Usually no scripts attached, simple collision/trigger behavior
  - **Memory Profile**: Lower priority, suitable for ON_DEMAND loading

### **Background Assets** (`assets/art/backdrop/`)
- **`void.png` → `void.spr`**
  - **Purpose**: Default skybox/background
  - **Usage**: Default backdrop when no specific background is specified
  - **Memory Profile**: IMMEDIATE category - always loaded for fallback
  - **Adaptive Fallback**: This is the fallback asset when memory is constrained

- **`logo.png` → `logo.spr`**
  - **Purpose**: Engine branding/splash screens
  - **Usage**: Bootloader splash, about screens, credits
  - **Memory Profile**: ON_DEMAND - loaded only when needed

### **UI Assets** (`assets/art/ui/`)
- **`light.png`** 
  - **Purpose**: Selected UI elements and active buttons
  - **Usage**: Highlighted menu items, selected buttons, active controls
  - **Script Integration**: UI scripts can toggle between light/dark states

- **`dark.png`**
  - **Purpose**: Unselected UI elements and disabled buttons  
  - **Usage**: Inactive menu items, disabled buttons, background UI elements
  - **Script Integration**: Default state for most UI elements

### **Tile Assets** (`assets/art/tiles/`)
- **`tile.png` → `tile.spr`**
  - **Purpose**: Level geometry and environmental tiles
  - **Usage**: Platforms, walls, floors, decorative elements
  - **Memory Profile**: CACHED category for active levels

## 🔧 **Asset Assignment Logic for SecureROMLoader**

### **Entity Type Resolution**
```cpp
// In SecureROMLoader::createPanelEntitiesSecure()
String determineEntitySprite(const EntityIntent& intent) {
    if (intent.scriptName.isEmpty()) {
        // No script = simple entity
        return "item.spr";
    } else {
        // Has script = complex NPC entity  
        return "npc.spr";
    }
}
```

### **UI State Management**
```cpp
// In UI panel scripts
void updateUIElementState(uint32_t elementUUID, bool isSelected, bool isEnabled) {
    if (!isEnabled) {
        setEntitySprite(elementUUID, "dark.png");  // Disabled state
    } else if (isSelected) {
        setEntitySprite(elementUUID, "light.png"); // Active state
    } else {
        setEntitySprite(elementUUID, "dark.png");  // Normal state
    }
}
```

## 📦 **Adaptive Memory Management Integration**

### **Asset Priority Classification**
```cpp
enum class AssetPriority {
    CRITICAL = 0,    // void.spr (fallback background)
    HIGH = 1,        // npc.spr (scripted entities)
    MEDIUM = 2,      // tile.spr (level geometry)
    LOW = 3,         // item.spr (simple entities)
    OPTIONAL = 4     // logo.spr (branding/splash)
};
```

### **Memory Constraint Fallback Strategy**

#### **High Memory (128KB+ available)**
- Load all assets at full quality
- Use `npc.spr` for scripted entities
- Use `item.spr` for simple entities  
- Full UI asset resolution (light.png/dark.png)

#### **Medium Memory (64-128KB available)**
- Reduce sprite frame counts by 50%
- Keep core entity assets (`npc.spr`, `item.spr`)
- Use compressed UI assets
- Stream audio instead of caching

#### **Low Memory (<64KB available)**
- Aggressive sprite compression (60% quality reduction)
- Merge `npc.spr` and `item.spr` into single sprite sheet
- Use single UI texture for both light/dark states (color modification)
- Force `void.spr` as background for all panels
- Disable particle effects and animations

### **Asset Fallback Implementation**
```cpp
// In SecureROMLoader::configureAssetFallbacks()
void configureEntityAssetFallbacks(uint8_t severity) {
    switch (severity) {
        case 0: // Minimal fallbacks
            compressSprites("npc.spr", 0.8f);
            compressSprites("item.spr", 0.8f);
            break;
            
        case 1: // Moderate fallbacks
            compressSprites("npc.spr", 0.6f);
            compressSprites("item.spr", 0.6f);
            reduceAnimationFrames("npc.spr", 0.5f);
            break;
            
        case 2: // Aggressive fallbacks
            compressSprites("npc.spr", 0.4f);
            mergeSprites({"npc.spr", "item.spr"}, "entities.spr");
            useMonochromeUI(true);
            break;
            
        case 3: // Maximum fallbacks
            useMinimalSprites(true);  // Single pixel representations
            disableAnimations(true);
            forceFallbackBackground("void.spr");
            break;
    }
}
```

## 🎮 **Script Integration Examples**

### **NPC Entity Script** (uses `npc.spr`)
```ash
// player_character.ash
entity player_character {
    sprite: "npc.spr",
    frame: 0,
    
    on_spawn() {
        set_animation("idle");
        enable_input(true);
    }
    
    on_update() {
        if (input_pressed(UP)) {
            set_animation("walk_up");
            move_entity(self, 0, -2);
        }
    }
    
    on_collision(other) {
        if (get_entity_type(other) == "enemy") {
            damage_player(10);
        }
    }
}
```

### **Simple Item Script** (uses `item.spr`)
```ash
// health_potion.ash
entity health_potion {
    sprite: "item.spr",
    frame: 5,  // Health potion frame
    
    on_collision(other) {
        if (get_entity_type(other) == "player") {
            heal_player(25);
            destroy_entity(self);
        }
    }
}
```

### **UI Panel Script** (uses `light.png`/`dark.png`)
```ash
// main_menu.ash
panel main_menu {
    ui_elements: [
        {id: "play_button", sprite: "dark.png"},
        {id: "options_button", sprite: "dark.png"},
        {id: "quit_button", sprite: "dark.png"}
    ],
    
    current_selection: 0,
    
    on_input(input_type, pressed) {
        if (pressed) {
            if (input_type == DOWN) {
                select_next_item();
            } else if (input_type == UP) {
                select_prev_item();  
            } else if (input_type == CONFIRM) {
                activate_current_item();
            }
        }
    }
    
    select_item(index) {
        // Deselect previous item
        set_ui_sprite(ui_elements[current_selection].id, "dark.png");
        
        // Select new item
        current_selection = index;
        set_ui_sprite(ui_elements[current_selection].id, "light.png");
    }
}
```

## 🛡️ **Security Integration**

### **Asset Validation in SecureROMLoader**
```cpp
bool SecureROMLoader::validateAssetIntent(const EntityIntent& intent) {
    // Validate sprite assignment matches script complexity
    if (!intent.scriptName.isEmpty()) {
        // Scripted entities should use npc.spr
        if (intent.metadata.contains("sprite:item.spr")) {
            recordSecurityViolation("ASSET_SCRIPT_MISMATCH", 
                "Scripted entity using simple sprite");
            return false;
        }
    }
    
    // Validate UI asset usage
    if (intent.entityType.startsWith("ui_")) {
        String allowedSprites[] = {"light.png", "dark.png"};
        bool validSprite = false;
        for (auto& sprite : allowedSprites) {
            if (intent.metadata.contains(sprite)) {
                validSprite = true;
                break;
            }
        }
        if (!validSprite) {
            recordSecurityViolation("INVALID_UI_SPRITE", 
                "UI element using non-UI sprite");
            return false;
        }
    }
    
    return true;
}
```

### **Asset Memory Tracking**
```cpp
struct AssetMemoryProfile {
    String assetName;
    uint32_t memorySizeKB;
    AssetPriority priority;
    bool isLoaded;
    uint32_t lastAccessed;
    uint8_t compressionLevel;  // 0=none, 3=maximum
};

// Track asset memory usage for adaptive management
std::vector<AssetMemoryProfile> assetProfiles = {
    {"npc.spr", 32, AssetPriority::HIGH, false, 0, 0},
    {"item.spr", 16, AssetPriority::MEDIUM, false, 0, 0}, 
    {"void.spr", 8, AssetPriority::CRITICAL, true, 0, 0},
    {"tile.spr", 24, AssetPriority::MEDIUM, false, 0, 0},
    {"light.png", 4, AssetPriority::LOW, false, 0, 0},
    {"dark.png", 4, AssetPriority::LOW, false, 0, 0}
};
```

## 📊 **Performance Optimization**

### **Asset Loading Strategy**
1. **IMMEDIATE**: `void.spr` (always loaded for fallback)
2. **CACHED**: `npc.spr`, `tile.spr` (high-usage assets)
3. **ON_DEMAND**: `item.spr`, UI assets (loaded per panel)
4. **STREAM**: Large background assets, audio (streamed when needed)

### **Memory Budget Allocation**
- **Critical Assets** (void.spr): 8KB (always reserved)
- **High Priority** (npc.spr): 32KB (scripted entities)
- **Medium Priority** (tile.spr, item.spr): 40KB (level assets)
- **UI Assets** (light.png, dark.png): 8KB (interface)
- **Fallback Reserve**: 32KB (for adaptive compression/optimization)

### **Quality Scaling Matrix**

| Memory Available | Sprite Quality | Animation Frames | UI Resolution |
|------------------|----------------|------------------|---------------|
| 128KB+           | 100%          | Full             | Full          |
| 64-128KB         | 80%           | 75%              | Full          |
| 32-64KB          | 60%           | 50%              | Reduced       |
| <32KB            | 40%           | 25%              | Monochrome    |

This asset mapping system ensures that the Wisp Engine can provide rich visual experiences while maintaining strict memory constraints and security through the SecureROMLoader's adaptive memory management system.
