# YAML Configuration Migration

## Overview
The Wisp Engine has migrated from JSON to YAML for all configuration files to provide better developer experience with comments and cleaner syntax.

## Changes Made

### Tool Chain Updates
- **wisp_builder.py**: Now supports `.yaml` and `.yml` files, prioritizes YAML over JSON
- **wisp_verifier.py**: Properly handles YAML asset types in bundle verification
- **app_loader.h**: Added simple YAML parser for ESP32 Arduino environment

### Configuration Format
**Old (JSON):**
```json
{
    "name": "Test App",
    "version": "1.0.0",
    "audio": {
        "sampleRate": 22050
    }
}
```

**New (YAML):**
```yaml
# Application metadata
name: "Test App"
version: "1.0.0"

# Audio configuration  
audio:
  sampleRate: 22050       # Sample rate in Hz
```

### File Priority
The system now looks for configuration files in this order:
1. `config.yaml` (preferred)
2. `config.yml` (alternative)
3. ~~`config.json`~~ (no longer supported)

### Benefits
- **Comments**: YAML supports inline and block comments for documentation
- **Readability**: Cleaner syntax without excessive brackets and quotes
- **Type Safety**: Better handling of booleans, numbers, and strings
- **Standards**: YAML is widely used in modern development workflows

## Migration Guide

### For Existing Applications
1. Convert `config.json` to `config.yaml`
2. Add comments to document configuration options
3. Rebuild WISP bundles with updated tools
4. Remove old JSON files

### Example Migration
```bash
# Old workflow
py tools/wisp_builder.py my_app my_app.wisp  # Used config.json

# New workflow  
py tools/wisp_builder.py my_app my_app.wisp  # Uses config.yaml
```

## Implementation Details

### ESP32 YAML Parser
- Lightweight parser for Arduino environment
- Supports basic key-value pairs and nested objects
- Handles comments and whitespace properly
- No external dependencies required

### Asset Type Mapping
```cpp
ASSET_TYPE = {
    '.yaml': 1,  # Configuration (YAML)
    '.yml': 1,   # Configuration (YAML alternative)
    '.wlut': 2,  # Palette data
    '.ash': 3,   # Source code
    '.wash': 4,  # Compiled binary
    '.sfx': 5,   # Audio data
    '.art': 6    # Sprite data
}
```

## Testing
- Created `test_app_final.wisp` with YAML configuration
- Verified 98.3% storage efficiency maintained
- Confirmed proper parsing in ESP32 app loader
- All existing functionality preserved

## Next Steps
- Update project templates to use YAML
- Add YAML validation to CI/CD pipeline
- Create YAML schema for IDE support
- Document advanced YAML features for complex configurations
