#!/usr/bin/env python3
"""
WISP Bundle Verifier
Verifies and displays information about Wisp bundle (.wisp) files
"""

import sys
import struct
import os

def verify_wisp_file(filepath):
    """Verify and display WISP bundle information"""
    if not os.path.exists(filepath):
        print(f"Error: File '{filepath}' not found")
        return False
    
    with open(filepath, 'rb') as f:
        data = f.read()
    
    if len(data) < 12:
        print("Error: File too small to be a valid WISP bundle")
        return False
    
    # Read header (12 bytes: magic + version + entry_count + config_size)
    magic = data[0:4]
    version = struct.unpack('<H', data[4:6])[0]
    entry_count = struct.unpack('<H', data[6:8])[0]
    config_size = struct.unpack('<I', data[8:12])[0]
    
    # Verify magic
    if magic != b'WISP':
        print(f"Error: Invalid magic bytes. Expected 'WISP', got '{magic.decode('ascii', errors='ignore')}'")
        return False
    
    print("=== WISP ROM Information ===")
    print(f"File: {filepath}")
    print(f"Size: {len(data)} bytes")
    print(f"Magic: {magic.decode('ascii')}")
    print(f"Version: {version >> 8}.{version & 0xFF}")
    print(f"Entry Count: {entry_count}")
    print(f"Embedded Config Size: {config_size} bytes")
    
    # Read embedded config if present
    config_data = None
    if config_size > 0:
        config_data = data[12:12+config_size]
        print(f"\nEmbedded Config Preview:")
        try:
            config_text = config_data.decode('utf-8')
            # Show first few lines of config
            lines = config_text.split('\n')[:5]
            for line in lines:
                if line.strip():
                    print(f"  {line}")
            if len(config_text.split('\n')) > 5:
                print("  ...")
        except:
            print("  [Binary config data]")
    
    # Calculate expected sizes
    header_size = 12
    entry_table_start = header_size + config_size
    entry_table_size = entry_count * 48  # Each entry is 48 bytes
    data_start = entry_table_start + entry_table_size
    
    if len(data) < data_start:
        print(f"Error: File too small for {entry_count} entries")
        return False
    
    print(f"\nStructure:")
    print(f"  Header: {header_size} bytes")
    if config_size > 0:
        print(f"  Embedded Config: {config_size} bytes")
    print(f"  Entry Table: {entry_table_size} bytes ({entry_count} × 48 bytes)")
    print(f"  Data Section: {len(data) - data_start} bytes")
    
    # Read and display entries
    print(f"\nROM Contents:")
    total_asset_size = 0
    
    for i in range(entry_count):
        entry_offset = entry_table_start + (i * 48)
        
        # Read entry (48 bytes)
        name = data[entry_offset:entry_offset+32].rstrip(b'\x00').decode('utf-8', errors='ignore')
        offset = struct.unpack('<I', data[entry_offset+32:entry_offset+36])[0]
        size = struct.unpack('<I', data[entry_offset+36:entry_offset+40])[0]
        asset_type = data[entry_offset+40]
        flags = data[entry_offset+41] 
        entry_reserved = data[entry_offset+42:entry_offset+48]
        
        # Asset type names
        asset_types = {
            0x01: '.wlut (Palette)',
            0x02: '.art (Sprite)', 
            0x03: '.tilemap',
            0x04: '.sfx (Audio)',
            0x05: '.font',
            0x06: '.json (Config)',
            0x07: '.ash (Source)',
            0x08: '.wash (Binary)',
            0x09: '.level',
            0x0A: '.depth'
        }
        
        type_name = asset_types.get(asset_type, f"Unknown (0x{asset_type:02X})")
        size_kb = size / 1024
        total_asset_size += size
        
        print(f"  {i+1:2d}: {name:<25} {size_kb:>8.1f}KB {type_name}")
        
        if flags != 0:
            print(f"      Flags: 0x{flags:02X}")
        
        # Verify data bounds
        asset_start = data_start + offset
        asset_end = asset_start + size
        if asset_end > len(data):
            print(f"      WARNING: Asset extends beyond file end!")
    
    # Summary
    bundle_overhead = header_size + config_size + entry_table_size
    efficiency = (total_asset_size / len(data)) * 100
    
    print(f"\nROM Summary:")
    print(f"  Total Assets: {total_asset_size / 1024:.1f}KB")
    if config_size > 0:
        print(f"  Embedded Config: {config_size / 1024:.1f}KB")
    print(f"  ROM Overhead: {bundle_overhead} bytes")
    print(f"  Storage Efficiency: {efficiency:.1f}%")
    
    return True

def extract_asset(wisp_file, asset_name, output_path):
    """Extract a specific asset from WISP bundle"""
    with open(wisp_file, 'rb') as f:
        data = f.read()
    
    # Read header
    magic = data[0:4]
    if magic != b'WISP':
        print("Error: Not a valid WISP file")
        return False
    
    entry_count = struct.unpack('<H', data[6:8])[0]
    header_size = 16
    data_start = header_size + (entry_count * 48)  # 48 bytes per entry
    
    # Find asset
    for i in range(entry_count):
        entry_offset = header_size + (i * 48)
        name = data[entry_offset:entry_offset+32].rstrip(b'\x00').decode('utf-8')
        
        if name == asset_name:
            offset = struct.unpack('<I', data[entry_offset+32:entry_offset+36])[0]
            size = struct.unpack('<I', data[entry_offset+36:entry_offset+40])[0]
            
            asset_start = data_start + offset
            asset_data = data[asset_start:asset_start+size]
            
            with open(output_path, 'wb') as out:
                out.write(asset_data)
            
            print(f"Extracted '{asset_name}' ({size} bytes) to '{output_path}'")
            return True
    
    print(f"Asset '{asset_name}' not found in bundle")
    return False

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("WISP Bundle Verifier")
        print("Usage:")
        print("  python wisp_verifier.py <file.wisp>                 # Verify bundle")
        print("  python wisp_verifier.py <file.wisp> extract <name>  # Extract asset")
        sys.exit(1)
    
    wisp_file = sys.argv[1]
    
    if len(sys.argv) >= 4 and sys.argv[2] == "extract":
        asset_name = sys.argv[3]
        output_path = asset_name
        success = extract_asset(wisp_file, asset_name, output_path)
    else:
        success = verify_wisp_file(wisp_file)
    
    sys.exit(0 if success else 1)
