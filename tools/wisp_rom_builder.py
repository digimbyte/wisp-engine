#!/usr/bin/env python3
"""
WISP ROM Builder
Creates GBA-like ROM files for the WISP engine cartridge system.
"""

import os
import struct
import yaml
import hashlib
from pathlib import Path

class WispROMBuilder:
    MAGIC_WISP = 0x50534957  # "WISP" in little-endian
    
    # Asset types matching unified src/system/asset_types.h
    ASSET_TYPES = {
        '.wlut': 0x01,  # ASSET_PALETTE
        '.art': 0x02,   # ASSET_SPRITE  
        '.sfx': 0x04,   # ASSET_AUDIO
        '.json': 0x06,  # ASSET_CONFIG
        '.ash': 0x07,   # ASSET_SOURCE
        '.wash': 0x08,  # ASSET_BINARY
        '.dat': 0x06    # ASSET_CONFIG (default)
    }
    
    def __init__(self):
        self.assets = []
        self.config_data = ""
        
    def add_asset(self, file_path, asset_name=None):
        """Add an asset file to the ROM"""
        if not os.path.exists(file_path):
            raise FileNotFoundError(f"Asset file not found: {file_path}")
            
        if asset_name is None:
            asset_name = os.path.basename(file_path)
            
        # Determine asset type from extension
        ext = os.path.splitext(file_path)[1].lower()
        asset_type = self.ASSET_TYPES.get(ext, 0x06)  # Default to ASSET_CONFIG
        
        with open(file_path, 'rb') as f:
            data = f.read()
            
        asset = {
            'name': asset_name[:31],  # Truncate to 31 chars (null-terminated)
            'data': data,
            'type': asset_type,
            'flags': 0
        }
        
        self.assets.append(asset)
        print(f"Added asset: {asset_name} ({len(data)} bytes, type {asset_type})")
        
    def set_config(self, config_file):
        """Set the embedded YAML configuration"""
        if not os.path.exists(config_file):
            raise FileNotFoundError(f"Config file not found: {config_file}")
            
        with open(config_file, 'r') as f:
            self.config_data = f.read()
            
    def build_rom(self, output_path):
        """Build the ROM file"""
        print(f"Building ROM: {output_path}")
        
        # Encode config data
        config_bytes = self.config_data.encode('utf-8')
        
        # Calculate offsets
        header_size = 12
        config_size = len(config_bytes)
        entry_table_size = len(self.assets) * 48
        data_start = header_size + config_size + entry_table_size
        
        # Build entry table and collect asset data
        entry_table = bytearray()
        asset_data = bytearray()
        current_offset = 0
        
        for asset in self.assets:
            # Asset entry (48 bytes):
            # name[32] + offset[4] + size[4] + type[1] + flags[1] + reserved[6]
            name_bytes = asset['name'].encode('utf-8')[:31]
            name_padded = name_bytes + b'\x00' * (32 - len(name_bytes))
            
            entry = struct.pack('<32sIIBB6x',
                              name_padded,
                              current_offset,
                              len(asset['data']),
                              asset['type'],
                              asset['flags'])
            
            entry_table.extend(entry)
            asset_data.extend(asset['data'])
            current_offset += len(asset['data'])
            
        # Build header
        header = struct.pack('<IIHHI',
                           self.MAGIC_WISP,    # magic
                           1,                  # version
                           len(self.assets),   # entry count
                           config_size,        # config size
                           0)                  # reserved
        
        # Write ROM file
        with open(output_path, 'wb') as f:
            f.write(header)
            f.write(config_bytes)
            f.write(entry_table)
            f.write(asset_data)
            
        total_size = len(header) + len(config_bytes) + len(entry_table) + len(asset_data)
        print(f"ROM built successfully: {total_size} bytes")
        print(f"  Header: {len(header)} bytes")
        print(f"  Config: {len(config_bytes)} bytes")
        print(f"  Entry table: {len(entry_table)} bytes")
        print(f"  Asset data: {len(asset_data)} bytes")

def create_sample_config():
    """Create a sample configuration file"""
    config = {
        'name': 'Sample WISP App',
        'version': '1.0.0',
        'author': 'WISP Developer',
        'description': 'A sample application for testing the WISP cartridge system',
        'performance': {
            'fps': 16,
            'ram': 131072
        },
        'system': {
            'wifi': False,
            'bluetooth': False,
            'eeprom': True
        },
        'assets': {
            'preload': ['splash.art', 'main_palette.wlut'],
            'streaming': ['level1.dat', 'music.sfx']
        }
    }
    
    with open('sample_config.yaml', 'w') as f:
        yaml.dump(config, f, default_flow_style=False)
        
    print("Created sample_config.yaml")

def create_sample_assets():
    """Create sample asset files for testing"""
    os.makedirs('sample_assets', exist_ok=True)
    
    # Create a simple sprite file
    sprite_data = bytes([i % 256 for i in range(64 * 64)])  # 64x64 sprite
    with open('sample_assets/sprite.art', 'wb') as f:
        f.write(sprite_data)
        
    # Create a simple palette file
    palette_data = struct.pack('<' + 'H' * 64, *range(64))  # 64 colors
    with open('sample_assets/palette.wlut', 'wb') as f:
        f.write(palette_data)
        
    # Create a main binary
    with open('sample_assets/main.wash', 'wb') as f:
        f.write(b'WASH_BINARY_PLACEHOLDER')
        
    print("Created sample assets in sample_assets/")

def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='WISP ROM Builder')
    parser.add_argument('--config', '-c', help='YAML configuration file')
    parser.add_argument('--assets', '-a', help='Assets directory')
    parser.add_argument('--output', '-o', default='output.wisp', help='Output ROM file')
    parser.add_argument('--create-sample', action='store_true', help='Create sample files')
    
    args = parser.parse_args()
    
    if args.create_sample:
        create_sample_config()
        create_sample_assets()
        return
        
    if not args.config or not args.assets:
        print("Error: --config and --assets are required (or use --create-sample)")
        return
        
    builder = WispROMBuilder()
    
    # Set configuration
    builder.set_config(args.config)
    
    # Add all assets from directory
    assets_path = Path(args.assets)
    if not assets_path.exists():
        print(f"Error: Assets directory not found: {args.assets}")
        return
        
    for asset_file in assets_path.rglob('*'):
        if asset_file.is_file():
            relative_name = str(asset_file.relative_to(assets_path))
            builder.add_asset(str(asset_file), relative_name)
            
    # Build ROM
    builder.build_rom(args.output)

if __name__ == '__main__':
    main()
