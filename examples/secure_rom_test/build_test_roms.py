#!/usr/bin/env python3
"""
Build Test ROMs for Secure ROM Testing
Creates valid, invalid, and memory stress test ROM files
"""

import os
import sys
import struct
from pathlib import Path

# Add the tools directory to the path so we can import the ROM builder
script_dir = Path(__file__).parent.absolute()
wisp_root = script_dir.parent.parent
tools_dir = wisp_root / "tools"

sys.path.append(str(tools_dir))

try:
    from wisp_rom_builder import WispROMBuilder
except ImportError:
    print("Error: Could not import wisp_rom_builder from tools directory")
    print(f"Make sure wisp_rom_builder.py exists in: {tools_dir}")
    sys.exit(1)

def create_test_assets():
    """Create basic test assets for the ROM files"""
    
    assets_dir = script_dir / "assets"
    assets_dir.mkdir(exist_ok=True)
    
    print("Creating test assets...")
    
    # Create basic sprite assets
    create_test_sprite(assets_dir / "npc.spr", "NPC sprite for scripted entities")
    create_test_sprite(assets_dir / "item.spr", "Item sprite for simple entities")  
    create_test_sprite(assets_dir / "light.png", "UI sprite for selected state")
    create_test_sprite(assets_dir / "dark.png", "UI sprite for unselected state")
    create_test_sprite(assets_dir / "void.spr", "Fallback background sprite")
    
    # Create large assets for memory testing
    create_large_sprite(assets_dir / "large_background.spr", "Large background for memory testing")
    create_large_sprite(assets_dir / "large_sprite_sheet.spr", "Large sprite sheet")
    
    # Create test binary files
    create_test_binary(assets_dir / "main.wash", "Main application binary")
    create_test_binary(assets_dir / "player_behavior.wash", "Player behavior script")
    create_test_binary(assets_dir / "enemy_ai.wash", "Enemy AI script")
    
    print("Test assets created successfully")

def create_test_sprite(filepath, description):
    """Create a simple test sprite file"""
    
    # Simple sprite format: 16x16 pixels with basic color data
    width, height = 16, 16
    color_data = bytes([i % 16 for i in range(width * height)])  # Simple pattern
    
    # Basic sprite header (simplified)
    sprite_data = struct.pack('<HHH', width, height, len(color_data))
    sprite_data += color_data
    
    with open(filepath, 'wb') as f:
        f.write(sprite_data)
    
    print(f"Created {filepath} - {description}")

def create_large_sprite(filepath, description):
    """Create a large sprite for memory testing"""
    
    # Larger sprite: 128x128 pixels
    width, height = 128, 128
    color_data = bytes([(i + j) % 64 for i in range(height) for j in range(width)])
    
    sprite_data = struct.pack('<HHH', width, height, len(color_data))
    sprite_data += color_data
    
    with open(filepath, 'wb') as f:
        f.write(sprite_data)
    
    print(f"Created {filepath} - {description} ({len(sprite_data)} bytes)")

def create_test_binary(filepath, description):
    """Create a test WASH binary file"""
    
    # Simple test binary with magic header and placeholder code
    binary_data = b'WASH'  # Magic number
    binary_data += struct.pack('<I', 1)  # Version
    binary_data += b'// Test binary: ' + description.encode('utf-8') + b'\n'
    binary_data += b'function main() { return 0; }'  # Placeholder code
    
    with open(filepath, 'wb') as f:
        f.write(binary_data)
    
    print(f"Created {filepath} - {description}")

def build_valid_rom():
    """Build the valid test ROM"""
    
    print("\n=== Building Valid Test ROM ===")
    
    builder = WispROMBuilder()
    config_path = script_dir / "valid_config.yaml"
    assets_dir = script_dir / "assets"
    output_path = script_dir / "security_test_valid.wisp"
    
    try:
        # Set configuration
        builder.set_config(str(config_path))
        
        # Add assets
        for asset_file in assets_dir.glob("*"):
            if asset_file.is_file():
                builder.add_asset(str(asset_file), asset_file.name)
        
        # Build ROM
        builder.build_rom(str(output_path))
        print(f"✓ Valid test ROM built: {output_path}")
        
        return True
        
    except Exception as e:
        print(f"✗ Failed to build valid ROM: {e}")
        return False

def build_invalid_rom():
    """Build the invalid test ROM (with security violations)"""
    
    print("\n=== Building Invalid Test ROM ===")
    
    builder = WispROMBuilder()
    config_path = script_dir / "invalid_config.yaml"
    assets_dir = script_dir / "assets"
    output_path = script_dir / "security_test_invalid.wisp"
    
    try:
        # Set configuration
        builder.set_config(str(config_path))
        
        # Add assets (same assets, but config has wrong assignments)
        for asset_file in assets_dir.glob("*"):
            if asset_file.is_file():
                builder.add_asset(str(asset_file), asset_file.name)
        
        # Build ROM
        builder.build_rom(str(output_path))
        print(f"✓ Invalid test ROM built: {output_path}")
        
        return True
        
    except Exception as e:
        print(f"✗ Failed to build invalid ROM: {e}")
        return False

def build_memory_stress_rom():
    """Build the memory stress test ROM"""
    
    print("\n=== Building Memory Stress Test ROM ===")
    
    builder = WispROMBuilder()
    config_path = script_dir / "memory_stress_config.yaml"
    assets_dir = script_dir / "assets"
    output_path = script_dir / "memory_stress_test.wisp"
    
    try:
        # Set configuration
        builder.set_config(str(config_path))
        
        # Add all assets including large ones
        for asset_file in assets_dir.glob("*"):
            if asset_file.is_file():
                builder.add_asset(str(asset_file), asset_file.name)
        
        # Build ROM
        builder.build_rom(str(output_path))
        print(f"✓ Memory stress test ROM built: {output_path}")
        
        return True
        
    except Exception as e:
        print(f"✗ Failed to build memory stress ROM: {e}")
        return False

def main():
    """Main build script"""
    
    print("=== Secure ROM Test Builder ===")
    print(f"Working directory: {script_dir}")
    
    # Create test assets
    create_test_assets()
    
    # Build test ROMs
    valid_success = build_valid_rom()
    invalid_success = build_invalid_rom()
    stress_success = build_memory_stress_rom()
    
    # Summary
    print("\n=== Build Summary ===")
    print(f"Valid ROM:        {'✓' if valid_success else '✗'}")
    print(f"Invalid ROM:      {'✓' if invalid_success else '✗'}")
    print(f"Stress Test ROM:  {'✓' if stress_success else '✗'}")
    
    if all([valid_success, invalid_success, stress_success]):
        print("\n🎉 All test ROMs built successfully!")
        print("\nNext steps:")
        print("1. Compile the secure_rom_test_app.cpp")
        print("2. Run the test application to validate security integration")
        return 0
    else:
        print("\n⚠️  Some ROM builds failed - check errors above")
        return 1

if __name__ == "__main__":
    sys.exit(main())
