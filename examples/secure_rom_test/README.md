# Secure ROM Test - Security System Validation

This example tests the **SecureROMLoader** integration with the existing WISP ROM loading system. It validates that security systems properly integrate with the `AppManager` → `WispRuntimeLoader` pipeline without breaking existing functionality.

## Purpose

- Test **ROM security validation** with valid and invalid ROM files
- Test **asset validation** (npc.spr for scripted entities, item.spr for simple entities, light.png/dark.png for UI)
- Test **memory adaptation** under different memory constraint scenarios
- Test **integration compatibility** with existing ROM loading infrastructure

## Test Files

### ROM Files (Built from assets)
- **`security_test_valid.wisp`** - Valid ROM with proper asset assignments
- **`security_test_invalid.wisp`** - Invalid ROM with security violations
- **`memory_stress_test.wisp`** - Large ROM to test memory fallback systems

### Test Application
- **`secure_rom_test_app.cpp`** - Main test application that loads different ROM scenarios

## Asset Structure

```
assets/
├── src/                          # Source assets (PNG files)
│   ├── npc_sprite.png           # Will become npc.spr
│   ├── item_sprite.png          # Will become item.spr
│   ├── ui_button_light.png      # Will become light.png
│   ├── ui_button_dark.png       # Will become dark.png
│   └── large_background.png     # Large asset for memory testing
├── npc.spr                      # Converted sprite for scripted entities
├── item.spr                     # Converted sprite for simple entities
├── light.png                    # UI sprite (selected state)
├── dark.png                     # UI sprite (unselected state)
└── void.spr                     # Fallback background sprite
```

## Configuration Files

### `valid_config.yaml` - Valid ROM Configuration
```yaml
name: "Security Test Valid"
version: "1.0.0"
author: "Wisp Engine Security Test"
description: "Valid ROM for testing security validation"

# Script definitions with proper asset assignments
entities:
  - name: "test_npc"
    script: "npc_behavior.wash"    # Has script -> should use npc.spr
    sprite: "npc.spr"              # Correct assignment
  - name: "test_item"
    sprite: "item.spr"             # No script -> correct assignment
  
ui_elements:
  - name: "test_button"
    sprite: "light.png"            # UI element -> correct assignment

performance:
  fps: 16
  ram: 65536

system:
  wifi: false
  bluetooth: false
```

### `invalid_config.yaml` - Invalid ROM Configuration
```yaml
name: "Security Test Invalid"
version: "1.0.0"

# Intentionally invalid asset assignments for testing
entities:
  - name: "scripted_entity"
    script: "behavior.wash"        # Has script
    sprite: "item.spr"             # WRONG - scripted should use npc.spr
  - name: "simple_entity"
    sprite: "npc.spr"              # WRONG - simple should use item.spr

ui_elements:
  - name: "fake_ui"
    sprite: "npc.spr"              # WRONG - UI should use light.png/dark.png
```

## Test Scenarios

### 1. Valid ROM Loading
```cpp
void testValidROMLoading() {
    SecureROMLoader secureLoader;
    AppManager appManager;
    
    // Should load successfully with all validations passing
    bool result = appManager.loadAppSecurely("security_test_valid.wisp", &secureLoader);
    assert(result == true);
}
```

### 2. Invalid ROM Rejection
```cpp
void testInvalidROMRejection() {
    SecureROMLoader secureLoader;
    AppManager appManager;
    
    // Should reject ROM due to asset assignment violations
    bool result = appManager.loadAppSecurely("security_test_invalid.wisp", &secureLoader);
    assert(result == false);
}
```

### 3. Memory Adaptation Testing
```cpp
void testMemoryAdaptation() {
    SecureROMLoader secureLoader;
    
    // Simulate low memory conditions
    secureLoader.setSimulatedMemory(32768); // 32KB available
    
    // Should trigger asset fallback systems
    bool result = appManager.loadAppSecurely("memory_stress_test.wisp", &secureLoader);
    assert(result == true); // Should succeed with fallbacks
}
```

## Build Process

### 1. Build Assets
```powershell
# Convert PNG source files to engine formats
.\build_assets.ps1 -ExampleDir secure_rom_test
```

### 2. Build Test ROMs
```powershell
# Build valid test ROM
python tools\wisp_rom_builder.py --config examples\secure_rom_test\valid_config.yaml --assets examples\secure_rom_test\assets --output examples\secure_rom_test\security_test_valid.wisp

# Build invalid test ROM  
python tools\wisp_rom_builder.py --config examples\secure_rom_test\invalid_config.yaml --assets examples\secure_rom_test\assets --output examples\secure_rom_test\security_test_invalid.wisp
```

### 3. Run Security Tests
```cpp
// Compile and run the test application
// Tests will validate integration between SecureROMLoader and existing systems
```

## Expected Results

### Valid ROM Test
- ✅ ROM loads successfully
- ✅ All asset validations pass
- ✅ Memory limits applied appropriately
- ✅ Application runs normally

### Invalid ROM Test  
- ❌ ROM loading fails with security violations
- ⚠️ Asset assignment mismatches detected
- 📋 Security violation log entries created
- 🛡️ System remains stable after rejection

### Memory Stress Test
- 🧠 Memory constraints detected
- ⚙️ Asset fallback systems activated  
- 📉 Quality reduction applied as needed
- ✅ ROM loads successfully with limitations

## Integration Points

This test validates that:
1. **SecureROMLoader** integrates cleanly with **AppManager**
2. **WispRuntimeLoader** receives security validation without API changes
3. **Existing ROM loading** continues to work for non-security-enabled ROMs
4. **Asset validation rules** are properly enforced
5. **Memory adaptation** works with real asset data

## Usage

Run this test when:
- Validating SecurityROMLoader integration
- Testing memory constraint handling
- Verifying asset validation rules
- Ensuring backward compatibility with existing ROM loading
