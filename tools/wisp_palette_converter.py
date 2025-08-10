#!/usr/bin/env python3
"""
Wisp Palette Converter
Converts PNG palette images to optimized Wisp WLUT palette data format
Supports multiple output formats based on image size and memory profiles
Outputs .wlut files (Wisp Lookup Table format)
"""

import sys
import struct
from PIL import Image
import numpy as np
import argparse
import os

def rgb888_to_rgb565(r, g, b):
    """Convert RGB888 to RGB565 format"""
    r = (r >> 3) & 0x1F  # 5 bits
    g = (g >> 2) & 0x3F  # 6 bits  
    b = (b >> 3) & 0x1F  # 5 bits
    return (r << 11) | (g << 5) | b

def analyze_image(image_path):
    """Analyze PNG image and determine optimal palette format"""
    try:
        img = Image.open(image_path)
        width, height = img.size
        
        # Preserve RGBA if it has transparency, otherwise convert to RGB
        if img.mode not in ['RGB', 'RGBA']:
            img = img.convert('RGBA' if 'transparency' in img.info else 'RGB')
            
        print(f"Image: {width}×{height} pixels")
        
        # Determine format based on dimensions
        if width == 64 and height == 64:
            return "LUT_64x64", img
        elif width == 32 and height == 32:
            return "LUT_32x32", img
        elif width == 16 and height == 1:
            return "PALETTE_16", img
        elif width == 64 and height == 1:
            return "PALETTE_64", img
        elif width == 256 and height == 1:
            return "PALETTE_256", img
        elif width <= 16 and height <= 16:
            return "PALETTE_SMALL", img
        else:
            # Auto-detect based on color count
            colors = img.getcolors(maxcolors=256*256*256)
            color_count = len(colors) if colors else 65536
            
            if color_count <= 16:
                return "PALETTE_16", img
            elif color_count <= 64:
                return "PALETTE_64", img
            elif color_count <= 256:
                return "PALETTE_256", img
            else:
                return "LUT_64x64", img
                
    except Exception as e:
        print(f"Error analyzing image: {e}")
        return None, None

def convert_to_lut_64x64(img):
    """Convert image to 64×64 LUT format with RGB|null support"""
    # Resize to 64×64 if needed
    if img.size != (64, 64):
        print(f"Resizing from {img.size} to 64×64")
        img = img.resize((64, 64), Image.Resampling.LANCZOS)
    
    # Handle both RGB and RGBA modes
    if img.mode == 'RGBA':
        pixels = np.array(img)
        lut_data = []
        
        # Process pixels, but reserve last 5 positions for magic colors
        for y in range(64):
            for x in range(64):
                # Skip last 5 pixels (positions 4091-4095) - they'll be magic colors
                if y == 63 and x >= 59:  # Last row, last 5 pixels
                    continue
                    
                r, g, b, a = pixels[y, x]
                if a <= 127:  # 50% alpha cutoff - pixel is transparent
                    lut_data.append(0x1000)  # Transparent entry (magic number)
                else:
                    # RGB pixel -> convert to RGB565
                    rgb565 = rgb888_to_rgb565(r, g, b)
                    lut_data.append(rgb565)
    else:
        # RGB mode - no transparency, convert all pixels to RGB565
        pixels = np.array(img)
        lut_data = []
        
        # Process pixels, but reserve last 5 positions for magic colors
        for y in range(64):
            for x in range(64):
                # Skip last 5 pixels (positions 4091-4095) - they'll be magic colors
                if y == 63 and x >= 59:  # Last row, last 5 pixels
                    continue
                    
                r, g, b = pixels[y, x]
                rgb565 = rgb888_to_rgb565(r, g, b)
                lut_data.append(rgb565)
    
    # Add magic channel colors as the last 5 entries
    magic_colors = [0x1000, 0x1001, 0x1002, 0x1003, 0x1004]  # Channels 0-4
    lut_data.extend(magic_colors)
    print(f"Added magic channel colors at positions {len(lut_data)-5}-{len(lut_data)-1}: {[hex(c) for c in magic_colors]}")
    
    return lut_data, 64, 64

def convert_to_palette(img, max_colors):
    """Convert image to palette format with specified max colors"""
    # Reserve space for magic colors (5 entries)
    usable_colors = max_colors - 5
    
    # Quantize to reduce colors if needed
    if img.mode != 'P':
        img = img.quantize(colors=usable_colors-1)  # -1 for potential transparent
    
    # Get palette
    palette = img.getpalette()
    if not palette:
        # Create palette from unique colors
        colors = img.getcolors(maxcolors=usable_colors)
        if not colors:
            print("Error: Too many colors in image")
            return None
        
        palette_data = []
        for count, color in sorted(colors, key=lambda x: x[0], reverse=True)[:usable_colors]:
            if isinstance(color, int):
                # Grayscale
                palette_data.extend([color, color, color])
            else:
                # RGB
                palette_data.extend(color)
    else:
        palette_data = palette[:usable_colors*3]
    
    # Convert palette to RGB565
    rgb565_palette = []
    for i in range(0, len(palette_data), 3):
        if i + 2 < len(palette_data):
            r, g, b = palette_data[i], palette_data[i+1], palette_data[i+2]
            rgb565 = rgb888_to_rgb565(r, g, b)
            rgb565_palette.append(rgb565)
    
    # Pad regular colors to usable_colors size
    while len(rgb565_palette) < usable_colors:
        rgb565_palette.append(0x0000)
    
    # Add magic channel colors as the last 5 entries
    magic_colors = [0x1000, 0x1001, 0x1002, 0x1003, 0x1004]  # Channels 0-4
    rgb565_palette.extend(magic_colors)
    print(f"Added magic channel colors at positions {len(rgb565_palette)-5}-{len(rgb565_palette)-1}: {[hex(c) for c in magic_colors]}")
    
    return rgb565_palette

def generate_c_header(data, format_type, width=0, height=0, output_name="palette"):
    """Generate C header file"""
    header = f"""// Generated Wisp palette data from PNG
// Format: {format_type}
// Auto-generated by wisp_palette_converter.py

#pragma once
#include <stdint.h>

"""
    
    if format_type.startswith("LUT_"):
        header += f"""// {width}×{height} Color LUT
#define {output_name.upper()}_LUT_WIDTH {width}
#define {output_name.upper()}_LUT_HEIGHT {height}
#define {output_name.upper()}_LUT_SIZE {len(data)}

const uint16_t {output_name}_lut[{len(data)}] = {{
"""
        # Format as 2D array for readability
        for y in range(height):
            header += "    "
            for x in range(width):
                idx = y * width + x
                header += f"0x{data[idx]:04X}"
                if idx < len(data) - 1:
                    header += ", "
                if x < width - 1 and (x + 1) % 8 == 0:
                    header += "\n    "
            header += "\n" if y < height - 1 else ""
        header += "\n};\n"
        
    else:  # Palette format
        header += f"""// {len(data)} Color Palette
#define {output_name.upper()}_PALETTE_SIZE {len(data)}

const uint16_t {output_name}_palette[{len(data)}] = {{
    """
        for i, color in enumerate(data):
            header += f"0x{color:04X}"
            if i < len(data) - 1:
                header += ", "
            if (i + 1) % 8 == 0 and i < len(data) - 1:
                header += "\n    "
        header += "\n};\n"
    
    # Add usage example
    header += f"""
// Usage example:
/*
#include "{output_name}_data.h"
"""
    
    if format_type.startswith("LUT_"):
        header += f"""
HybridPaletteSystem palette;
palette.loadColorLUT({output_name}_lut);
"""
    else:
        header += f"""
OptimizedPaletteSystem palette;
palette.loadPalette(0, {output_name}_palette, {output_name.upper()}_PALETTE_SIZE);
"""
    
    header += "*/\n"
    return header

def generate_binary(data, format_type):
    """Generate WLUT binary data file"""
    binary_data = bytearray()
    
    # Header: 4 bytes magic, 4 bytes format, 2 bytes width, 2 bytes height, 2 bytes color count, 2 bytes reserved
    binary_data.extend(b'WLUT')  # Magic for Wisp Lookup Table format
    
    if format_type == "LUT_64x64":
        binary_data.extend(struct.pack('<I', 0x4C555436))  # 'LUT6' 
        binary_data.extend(struct.pack('<HH', 64, 64))     # Width, Height
    elif format_type == "LUT_32x32":
        binary_data.extend(struct.pack('<I', 0x4C555433))  # 'LUT3'
        binary_data.extend(struct.pack('<HH', 32, 32))     # Width, Height
    elif format_type == "PALETTE_16":
        binary_data.extend(struct.pack('<I', 0x50414C31))  # 'PAL1' 
        binary_data.extend(struct.pack('<HH', 0, 0))       # No width/height for palettes
    elif format_type == "PALETTE_64":
        binary_data.extend(struct.pack('<I', 0x50414C36))  # 'PAL6'
        binary_data.extend(struct.pack('<HH', 0, 0))       # No width/height for palettes
    elif format_type == "PALETTE_256":
        binary_data.extend(struct.pack('<I', 0x50414C38))  # 'PAL8'
        binary_data.extend(struct.pack('<HH', 0, 0))       # No width/height for palettes
    else:
        binary_data.extend(struct.pack('<I', 0x50414C00))  # 'PAL\0'
        binary_data.extend(struct.pack('<HH', 0, 0))       # No width/height for palettes
    
    binary_data.extend(struct.pack('<H', len(data)))      # Color count
    binary_data.extend(struct.pack('<H', 0))              # Reserved
    
    # Data: RGB565 values (little endian)
    for color in data:
        binary_data.extend(struct.pack('<H', color))
    
    return binary_data

def main():
    parser = argparse.ArgumentParser(description='Convert PNG to Wisp WLUT palette format')
    parser.add_argument('input', help='Input PNG file')
    parser.add_argument('-o', '--output', help='Output filename (without extension)')
    parser.add_argument('-f', '--format', choices=['auto', 'lut64', 'lut32', 'pal16', 'pal64', 'pal256'], 
                       default='auto', help='Force specific format')
    parser.add_argument('--header-only', action='store_true', help='Generate only C header')
    parser.add_argument('--binary-only', action='store_true', help='Generate only WLUT binary file')
    parser.add_argument('--preview', action='store_true', help='Show color preview')
    
    args = parser.parse_args()
    
    if not os.path.exists(args.input):
        print(f"Error: Input file '{args.input}' not found")
        return 1
    
    # Analyze image
    format_type, img = analyze_image(args.input)
    if not format_type:
        print("Error: Could not analyze image")
        return 1
    
    # Override format if specified
    format_map = {
        'lut64': 'LUT_64x64',
        'lut32': 'LUT_32x32', 
        'pal16': 'PALETTE_16',
        'pal64': 'PALETTE_64',
        'pal256': 'PALETTE_256'
    }
    if args.format != 'auto':
        format_type = format_map[args.format]
    
    print(f"Converting to format: {format_type}")
    
    # Convert based on format
    if format_type == "LUT_64x64":
        data, width, height = convert_to_lut_64x64(img)
    elif format_type == "LUT_32x32":
        # Resize to 32×32
        img_32 = img.resize((32, 32), Image.Resampling.LANCZOS)
        # Convert to 32x32 format with magic colors
        if img_32.mode == 'RGBA':
            pixels = np.array(img_32)
            lut_data = []
            
            # Process pixels, but reserve last 5 positions for magic colors
            for y in range(32):
                for x in range(32):
                    # Skip last 5 pixels (positions 1019-1023) - they'll be magic colors
                    if y == 31 and x >= 27:  # Last row, last 5 pixels
                        continue
                        
                    r, g, b, a = pixels[y, x]
                    if a <= 127:  # 50% alpha cutoff - pixel is transparent
                        lut_data.append(0x1000)  # Transparent entry (magic number)
                    else:
                        # RGB pixel -> convert to RGB565
                        rgb565 = rgb888_to_rgb565(r, g, b)
                        lut_data.append(rgb565)
        else:
            # RGB mode - no transparency, convert all pixels to RGB565
            pixels = np.array(img_32)
            lut_data = []
            
            # Process pixels, but reserve last 5 positions for magic colors
            for y in range(32):
                for x in range(32):
                    # Skip last 5 pixels (positions 1019-1023) - they'll be magic colors
                    if y == 31 and x >= 27:  # Last row, last 5 pixels
                        continue
                        
                    r, g, b = pixels[y, x]
                    rgb565 = rgb888_to_rgb565(r, g, b)
                    lut_data.append(rgb565)
        
        # Add magic channel colors as the last 5 entries
        magic_colors = [0x1000, 0x1001, 0x1002, 0x1003, 0x1004]  # Channels 0-4
        lut_data.extend(magic_colors)
        print(f"Added magic channel colors at positions {len(lut_data)-5}-{len(lut_data)-1}: {[hex(c) for c in magic_colors]}")
        
        data, width, height = lut_data, 32, 32
    elif format_type == "PALETTE_16":
        data = convert_to_palette(img, 16)
        width = height = 0
    elif format_type == "PALETTE_64":
        data = convert_to_palette(img, 64)
        width = height = 0
    elif format_type == "PALETTE_256":
        data = convert_to_palette(img, 256)
        width = height = 0
    else:
        print(f"Error: Unsupported format {format_type}")
        return 1
    
    if not data:
        print("Error: Conversion failed")
        return 1
    
    # Generate output filename
    if args.output:
        output_base = args.output
    else:
        output_base = os.path.splitext(os.path.basename(args.input))[0]
    
    # Show preview if requested
    if args.preview:
        print(f"\nColor preview ({len(data)} colors):")
        for i, color in enumerate(data[:16]):  # Show first 16 colors
            r = (color >> 11) & 0x1F
            g = (color >> 5) & 0x3F
            b = color & 0x1F
            r = (r * 255) // 31
            g = (g * 255) // 63
            b = (b * 255) // 31
            print(f"  {i:2d}: 0x{color:04X} → RGB({r:3d},{g:3d},{b:3d})")
        if len(data) > 16:
            print(f"  ... and {len(data) - 16} more colors")
    
    # Generate outputs
    # Ensure exports directory exists
    os.makedirs("../exports", exist_ok=True)
    
    if not args.binary_only:
        header = generate_c_header(data, format_type, width, height, output_base)
        header_file = f"../exports/{output_base}_data.h"
        with open(header_file, 'w') as f:
            f.write(header)
        print(f"Generated C header: {header_file}")
    
    if not args.header_only:
        binary = generate_binary(data, format_type)
        binary_file = f"../exports/{output_base}.wlut"
        with open(binary_file, 'wb') as f:
            f.write(binary)
        print(f"Generated WLUT binary: {binary_file}")
    
    # Show memory usage
    memory_kb = len(data) * 2 / 1024
    print(f"\nMemory usage: {len(data)} colors × 2 bytes = {memory_kb:.1f}KB")
    
    if format_type.startswith("LUT_"):
        print("Usage: HybridPaletteSystem::loadColorLUT()")
    else:
        print("Usage: OptimizedPaletteSystem::loadPalette()")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
