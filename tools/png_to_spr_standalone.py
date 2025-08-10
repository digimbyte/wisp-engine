#!/usr/bin/env python3
"""
PNG to SPR (Sprite) Format Converter
Converts PNG images to Wisp Engine sprite format (.spr files)
Based on sprite compilation code from wisp_builder.py
"""

import os
import struct
import sys
import argparse
import glob
from PIL import Image
import numpy as np

# Global WLUT data
WLUT_DATA = None

def load_wlut(wlut_path):
    """Load WLUT file and return color lookup table"""
    global WLUT_DATA
    try:
        with open(wlut_path, 'rb') as f:
            # Read WLUT header
            magic = f.read(4)
            if magic != b'WLUT':
                print(f"Error: Invalid WLUT magic: {magic}")
                return None
            
            format_type = struct.unpack('<I', f.read(4))[0]
            width, height = struct.unpack('<HH', f.read(4))
            color_count = struct.unpack('<H', f.read(2))[0]
            reserved = struct.unpack('<H', f.read(2))[0]
            
            # Read color data
            colors = []
            for i in range(color_count):
                color = struct.unpack('<H', f.read(2))[0]
                colors.append(color)
            
            WLUT_DATA = {
                'width': width,
                'height': height,
                'colors': colors,
                'count': color_count
            }
            
            print(f"Loaded WLUT: {width}x{height}, {color_count} colors")
            return True
    except Exception as e:
        print(f"Error loading WLUT: {e}")
        return False

def find_closest_transparent_index():
    """Find closest transparent (0x1000) entry in WLUT"""
    global WLUT_DATA
    if not WLUT_DATA:
        return 0  # Fallback to index 0 if no WLUT loaded
    
    # Search for 0x1000 (transparent magic number) entries
    for i, color in enumerate(WLUT_DATA['colors']):
        if color == 0x1000:
            return i
    
    # No transparent entry found - return index 0 as fallback
    return 0

def find_closest_color_index(target_rgb565):
    """Find closest RGB color match in WLUT"""
    global WLUT_DATA
    if not WLUT_DATA:
        return 0  # Fallback to index 0 if no WLUT loaded
    
    # Extract RGB components from target
    target_r = (target_rgb565 >> 11) & 0x1F
    target_g = (target_rgb565 >> 5) & 0x3F
    target_b = target_rgb565 & 0x1F
    
    best_index = 0
    best_distance = float('inf')
    
    # Search for closest non-magic color
    for i, color in enumerate(WLUT_DATA['colors']):
        if color >= 0x1000 and color <= 0x1111:  # Skip magic number entries
            continue
            
        # Extract RGB components
        r = (color >> 11) & 0x1F
        g = (color >> 5) & 0x3F
        b = color & 0x1F
        
        # Calculate Euclidean distance in RGB space
        dr = target_r - r
        dg = target_g - g
        db = target_b - b
        distance = dr*dr + dg*dg + db*db
        
        if distance < best_distance:
            best_distance = distance
            best_index = i
    
    return best_index

def discover_wlut_files():
    """Search for WLUT files in current directory and common locations"""
    search_paths = [
        "*.wlut",           # Current directory
        "../*.wlut",        # Parent directory 
        "../exports/*.wlut", # Exports directory
        "exports/*.wlut"    # Local exports directory
    ]
    
    wlut_files = []
    for pattern in search_paths:
        wlut_files.extend(glob.glob(pattern))
    
    # Remove duplicates and normalize paths
    unique_files = list(set(os.path.normpath(f) for f in wlut_files))
    return unique_files

def select_wlut_file(wlut_files):
    """Present WLUT files to user for selection"""
    if len(wlut_files) == 0:
        return None
    elif len(wlut_files) == 1:
        print(f"Found WLUT file: {wlut_files[0]}")
        return wlut_files[0]
    else:
        print("\nMultiple WLUT files found:")
        for i, wlut_file in enumerate(wlut_files):
            print(f"  {i+1}: {wlut_file}")
        
        while True:
            try:
                choice = input(f"\nSelect WLUT file (1-{len(wlut_files)}) or 'n' to skip: ")
                if choice.lower() == 'n':
                    return None
                
                index = int(choice) - 1
                if 0 <= index < len(wlut_files):
                    return wlut_files[index]
                else:
                    print(f"Invalid choice. Please enter 1-{len(wlut_files)} or 'n'")
            except (ValueError, KeyboardInterrupt):
                print(f"Invalid input. Please enter 1-{len(wlut_files)} or 'n'")

def compile_png_to_spr(png_path, output_path, palette_id=0):
    """
    Convert PNG to Wisp Engine SPR sprite format
    """
    try:
        # Load the PNG image
        img = Image.open(png_path).convert('RGBA')
        width, height = img.size
        
        if width > 512 or height > 512:
            print(f"Warning: {png_path} is {width}x{height}, max recommended is 512x512")
        
        # Convert RGBA with 50% alpha cutoff and WLUT lookup
        rgba_array = np.array(img)
        
        # Process each pixel with 50% alpha cutoff
        color_data = []
        for y in range(height):
            for x in range(width):
                r, g, b, a = rgba_array[y, x]
                
                if a <= 127:  # 50% alpha cutoff - pixel is transparent
                    # Find closest transparent entry (0x0000) in WLUT
                    palette_index = find_closest_transparent_index()
                    color_data.append(palette_index)
                else:  # Pixel is visible - find closest RGB color in WLUT
                    # Convert RGB to RGB565 format
                    r_565 = (r >> 3) & 0x1F  # 5 bits
                    g_565 = (g >> 2) & 0x3F  # 6 bits
                    b_565 = (b >> 3) & 0x1F  # 5 bits
                    rgb565 = (r_565 << 11) | (g_565 << 5) | b_565
                    
                    # Find closest color match in WLUT
                    palette_index = find_closest_color_index(rgb565)
                    color_data.append(palette_index)
        
        # Look for matching depth file
        depth_path = os.path.splitext(png_path)[0] + '.depth'
        if os.path.exists(depth_path):
            # Load depth map (grayscale image, 0-12 values)
            depth_img = Image.open(depth_path).convert('L')
            if depth_img.size != (width, height):
                print(f"Warning: Depth map {depth_path} size mismatch, using flat depth")
                depth_data = [6] * (width * height)  # Mid depth
            else:
                depth_pixels = np.array(depth_img)
                depth_data = []
                for y in range(height):
                    for x in range(width):
                        # Scale 0-255 to 0-12
                        depth_value = (depth_pixels[y, x] * 12) // 255
                        depth_data.append(depth_value)
        else:
            # Generate flat depth map
            depth_data = [6] * (width * height)  # Mid depth
        
        # Run-length encode the depth data
        depth_runs = []
        current_depth = depth_data[0]
        run_length = 1
        
        for i in range(1, len(depth_data)):
            if depth_data[i] == current_depth and run_length < 65535:
                run_length += 1
            else:
                depth_runs.append((current_depth, run_length))
                current_depth = depth_data[i]
                run_length = 1
        
        # Add final run
        depth_runs.append((current_depth, run_length))
        
        # Build sprite binary data
        sprite_data = bytearray()
        
        # Header (16 bytes - updated for sprite sheets)
        color_data_size = len(color_data)
        depth_data_size = len(depth_runs) * 3  # depth(1) + distance(2)
        flags = 0x00        # No flags by default
        frame_rows = 1      # Single frame by default
        frame_cols = 1
        frame_width = width
        frame_height = height
        
        # Pack header: width, height, colorSize, depthSize, palette, flags, frameRows, frameCols, frameWidth, frameHeight
        sprite_data.extend(struct.pack('<HHIIBBBBHH', 
            width, height, color_data_size, depth_data_size, 
            palette_id, flags, frame_rows, frame_cols, frame_width, frame_height))
        
        # Color data
        sprite_data.extend(bytes(color_data))
        
        # Depth runs
        for depth, distance in depth_runs:
            sprite_data.extend(struct.pack('<BH', depth, distance))
        
        # Write to file
        with open(output_path, 'wb') as f:
            f.write(sprite_data)
        
        print(f"Compiled sprite: {os.path.basename(png_path)} ({width}x{height}) -> {len(sprite_data)} bytes")
        print(f"  Color data: {color_data_size} bytes, Depth runs: {len(depth_runs)} ({depth_data_size} bytes)")
        print(f"  Output: {output_path}")
        
        return True
        
    except Exception as e:
        print(f"Error compiling sprite {png_path}: {e}")
        return False

def main():
    parser = argparse.ArgumentParser(description='Convert PNG to Wisp SPR sprite format')
    parser.add_argument('input', help='Input PNG file')
    parser.add_argument('output', help='Output SPR file')
    parser.add_argument('-p', '--palette', type=int, default=0, help='Palette ID (default: 0)')
    parser.add_argument('-w', '--wlut', help='WLUT palette file (required for color matching)')
    
    args = parser.parse_args()
    
    if not os.path.exists(args.input):
        print(f"Error: Input file '{args.input}' not found")
        return 1
    
    # Determine which WLUT file to use
    wlut_file = None
    if args.wlut:
        # Use specified WLUT file
        if not os.path.exists(args.wlut):
            print(f"Error: WLUT file '{args.wlut}' not found")
            return 1
        wlut_file = args.wlut
    else:
        # Auto-discover WLUT files
        print("No WLUT file specified, searching for available WLUT files...")
        wlut_files = discover_wlut_files()
        wlut_file = select_wlut_file(wlut_files)
        
        if not wlut_file:
            print("Warning: No WLUT file selected - will use fallback index 0 for all pixels")
    
    # Load the WLUT file if we have one
    if wlut_file:
        if not load_wlut(wlut_file):
            print(f"Error: Failed to load WLUT file '{wlut_file}'")
            return 1
    
    # Ensure output directory exists
    os.makedirs(os.path.dirname(args.output), exist_ok=True)
    
    # Convert
    success = compile_png_to_spr(args.input, args.output, args.palette)
    
    if success:
        print(f"\nConversion completed successfully!")
        return 0
    else:
        print(f"Conversion failed!")
        return 1

if __name__ == "__main__":
    sys.exit(main())
