#!/usr/bin/env python3
"""
WLUT File Format Verifier
Verifies and displays information about Wisp Lookup Table (.wlut) files
"""

import sys
import struct
import os

def verify_wlut_file(filepath):
    """Verify and display WLUT file information"""
    if not os.path.exists(filepath):
        print(f"Error: File '{filepath}' not found")
        return False
    
    with open(filepath, 'rb') as f:
        data = f.read()
    
    if len(data) < 16:
        print("Error: File too small to be a valid WLUT file")
        return False
    
    # Read header
    magic = data[0:4]
    format_type = struct.unpack('<I', data[4:8])[0]
    width = struct.unpack('<H', data[8:10])[0]
    height = struct.unpack('<H', data[10:12])[0]
    color_count = struct.unpack('<H', data[12:14])[0]
    reserved = struct.unpack('<H', data[14:16])[0]
    
    # Verify magic
    if magic != b'WLUT':
        print(f"Error: Invalid magic bytes. Expected 'WLUT', got '{magic.decode('ascii', errors='ignore')}'")
        return False
    
    print("=== WLUT File Information ===")
    print(f"File: {filepath}")
    print(f"Size: {len(data)} bytes")
    print(f"Magic: {magic.decode('ascii')}")
    
    # Decode format type
    format_names = {
        0x4C555436: "LUT_64x64",
        0x4C555433: "LUT_32x32", 
        0x50414C31: "PAL_16",
        0x50414C36: "PAL_64",
        0x50414C38: "PAL_256"
    }
    
    format_name = format_names.get(format_type, f"Unknown (0x{format_type:08X})")
    print(f"Format: {format_name}")
    
    if width > 0 and height > 0:
        print(f"Dimensions: {width}×{height}")
        expected_colors = width * height
    else:
        print("Type: Palette (no dimensions)")
        expected_colors = color_count
    
    print(f"Color Count: {color_count}")
    print(f"Reserved: 0x{reserved:04X}")
    
    # Verify data size
    expected_data_size = 16 + (color_count * 2)  # Header + RGB565 data
    if len(data) != expected_data_size:
        print(f"Warning: Expected {expected_data_size} bytes, got {len(data)} bytes")
    
    # Display some sample colors
    print(f"\nSample Colors (first 8):")
    for i in range(min(8, color_count)):
        offset = 16 + (i * 2)
        if offset + 1 < len(data):
            color = struct.unpack('<H', data[offset:offset+2])[0]
            r = (color >> 11) & 0x1F
            g = (color >> 5) & 0x3F
            b = color & 0x1F
            r = (r * 255) // 31
            g = (g * 255) // 63
            b = (b * 255) // 31
            print(f"  {i:2d}: 0x{color:04X} → RGB({r:3d},{g:3d},{b:3d})")
    
    if color_count > 8:
        print(f"  ... and {color_count - 8} more colors")
    
    # Memory usage
    memory_kb = (color_count * 2) / 1024
    print(f"\nMemory Usage: {color_count} colors × 2 bytes = {memory_kb:.1f}KB")
    
    return True

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python wlut_verifier.py <file.wlut>")
        sys.exit(1)
    
    success = verify_wlut_file(sys.argv[1])
    sys.exit(0 if success else 1)
