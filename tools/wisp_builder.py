import os
import struct
import yaml  # For YAML configuration parsing
from PIL import Image
import numpy as np

MAGIC = b'WISP'  # Master bundle format
ENTRY_STRUCT = '<32sIIBB6s'  # name, offset, size, type, flags, reserved
HEADER_STRUCT = '<4sHHI'    # magic, version, entry_count, config_size
# New structure: WISP magic + version + entry_count + embedded_config_size

# Asset type mapping for WISP bundle files
# See docs/wisp_file_formats.md for complete specification

ASSET_TYPE = {
    '.wlut': 0x01,     # Wisp Lookup Table (palettes/LUTs)
    '.art': 0x02,      # Sprite graphics data
    '.png': 0x02,      # Convert PNG to .art sprite format  
    '.tilemap': 0x03,  # Tile-based maps
    '.sfx': 0x04,      # Audio files
    '.wav': 0x04,      # Convert WAV to .sfx format
    '.ogg': 0x04,      # Convert OGG to .sfx format
    '.font': 0x05,     # Font data
    '.yaml': 0x06,     # Configuration files (YAML format)
    '.yml': 0x06,      # Configuration files (YAML format, alternate extension)
    '.ash': 0x07,      # Uncompiled C++ source
    '.wash': 0x08,     # Compiled bytecode/binary
    '.level': 0x09,    # Game level data
    '.depth': 0x0A,    # Depth map data (for sprite compilation)
}

def collect_assets(root_folder):
    entries = []
    payloads = []
    offset = 0
    config_data = None

    # Find and extract config file (but don't add as asset)
    config_files = ['config.yaml', 'config.yml']
    config_file = None
    for cf in config_files:
        config_path = os.path.join(root_folder, cf)
        if os.path.exists(config_path):
            config_file = cf
            with open(config_path, 'rb') as f:
                config_data = f.read()
            break
    
    if not config_data:
        raise FileNotFoundError("No configuration file found (config.yaml or config.yml)")

    # Collect all other assets (excluding config files)
    for dirpath, _, filenames in os.walk(root_folder):
        for fname in sorted(filenames):
            # Skip config files as they're embedded in header
            if fname in ['config.yaml', 'config.yml', 'config.json']:
                continue
                
            fpath = os.path.join(dirpath, fname)
            _, ext = os.path.splitext(fname)
            ext = ext.lower()

            if ext not in ASSET_TYPE:
                continue

            # Special handling for PNG files - convert to sprite format
            if ext == '.png':
                sprite_data = compile_png_to_sprite(fpath, root_folder)
                if sprite_data:
                    data = sprite_data
                else:
                    # Fall back to raw PNG data if compilation fails
                    with open(fpath, 'rb') as f:
                        data = f.read()
            else:
                with open(fpath, 'rb') as f:
                    data = f.read()

            # Create relative path for nested files
            rel_path = os.path.relpath(fpath, root_folder)
            rel_path = rel_path.replace('\\', '/')  # Normalize path separators
            
            entry = {
                'name': rel_path.encode('utf-8')[:31].ljust(32, b'\x00'),
                'offset': offset,
                'size': len(data),
                'type': ASSET_TYPE[ext],
                'flags': 0,  # No special flags for now
                'reserved': b'\x00\x00\x00\x00\x00\x00',
            }
            entries.append(entry)
            payloads.append(data)
            offset += len(data)

    return entries, payloads, config_data

def validate_app_config(root_folder):
    """Validate that the app has a proper config.yaml or config.yml"""
    config_files = ['config.yaml', 'config.yml']
    config = None
    config_file = None
    
    # Try to find a YAML config file
    for cf in config_files:
        config_path = os.path.join(root_folder, cf)
        if os.path.exists(config_path):
            config_file = cf
            break
    
    if not config_file:
        print(f"Warning: No config.yaml or config.yml found in {root_folder}")
        return False
    
    try:
        config_path = os.path.join(root_folder, config_file)
        with open(config_path, 'r') as f:
            config = yaml.safe_load(f)
        
        # Validate required fields
        required_fields = ['name', 'version']
        for field in required_fields:
            if field not in config:
                print(f"Error: Missing required field '{field}' in {config_file}")
                return False
        
        print(f"App: {config['name']} v{config['version']}")
        if 'author' in config:
            print(f"Author: {config['author']}")
        
        return True
    except yaml.YAMLError as e:
        print(f"Error: Invalid YAML in {config_file}: {e}")
        return False

def write_wisp_bundle(entries, payloads, config_data, output_file):
    """Write WISP bundle format with embedded config"""
    with open(output_file, 'wb') as f:
        # Write header (12 bytes: magic + version + entry_count + config_size)
        version = 0x0100  # Version 1.0
        config_size = len(config_data) if config_data else 0
        f.write(struct.pack(HEADER_STRUCT, MAGIC, version, len(entries), config_size))
        
        # Write embedded config immediately after header
        if config_data:
            f.write(config_data)

        # Write entry table (48 bytes per entry)
        for e in entries:
            f.write(struct.pack(ENTRY_STRUCT, e['name'], e['offset'], e['size'], 
                              e['type'], e['flags'], e['reserved']))

        # Write data payloads
        for p in payloads:
            f.write(p)

    print(f'Packed {len(entries)} assets into {output_file}')
    
    # Display bundle structure
    print(f"\nWISP ROM Contents:")
    if config_data:
        print(f"  [EMBEDDED CONFIG]           {len(config_data) / 1024:.1f}KB .yml")
    
    total_size = 0
    for e in entries:
        name = e['name'].rstrip(b'\x00').decode('utf-8')
        size_kb = e['size'] / 1024
        total_size += e['size']
        asset_types = {v: k for k, v in ASSET_TYPE.items()}
        type_name = asset_types.get(e['type'], f"0x{e['type']:02X}")
        print(f"  {name:<25} {size_kb:>8.1f}KB {type_name}")
    
    header_size = 12  # New header size
    config_size = len(config_data) if config_data else 0
    entry_table_size = len(entries) * 48
    bundle_size_kb = (header_size + config_size + entry_table_size + total_size) / 1024
    
    print(f"\nTotal ROM size: {bundle_size_kb:.1f}KB")
    print(f"Header (12 bytes) + Config: {(header_size + config_size) / 1024:.1f}KB") 
    print(f"Entry table: {entry_table_size / 1024:.1f}KB")
    print(f"Asset data: {total_size / 1024:.1f}KB")

def compile_png_to_sprite(png_path, root_folder):
    """
    Convert PNG + optional depth map to Wisp Engine sprite format
    Looks for matching .depth file or generates flat depth
    """
    try:
        # Try to import PIL for image processing
        from PIL import Image
        import numpy as np
    except ImportError:
        print(f"Warning: PIL/numpy not available, skipping sprite compilation for {png_path}")
        return None
        
    try:
        # Load the PNG image
        img = Image.open(png_path).convert('RGBA')
        width, height = img.size
        
        if width > 512 or height > 512:
            print(f"Warning: {png_path} is {width}x{height}, max recommended is 512x512")
        
        # Convert RGBA to color indices (simple quantization to 256 colors)
        img_rgb = img.convert('RGB')
        pixels = np.array(img_rgb)
        alpha = np.array(img.convert('RGBA'))[:, :, 3]
        
        # Simple color quantization - map RGB to color index
        # This is a placeholder - you'd want a proper quantization algorithm
        color_data = []
        for y in range(height):
            for x in range(width):
                if alpha[y, x] < 128:  # Transparent pixel
                    color_data.append(0)
                else:
                    # Simple color index mapping (needs proper implementation)
                    r, g, b = pixels[y, x]
                    # Map RGB to 0-255 color index (placeholder algorithm)
                    color_index = ((r >> 5) << 5) | ((g >> 5) << 2) | (b >> 6)
                    color_data.append(min(255, max(1, color_index)))  # Avoid 0 (transparent)
        
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
        palette_id = 0      # Default palette
        flags = 0x00        # No flags by default
        frame_rows = 1      # Single frame by default
        frame_cols = 1
        frame_width = width
        frame_height = height
        
        # Pack header: width, height, colorSize, depthSize, palette, flags, frameRows, frameCols, frameWidth, frameHeight
        sprite_data.extend(struct.pack('<HHHBBBHH', 
            width, height, color_data_size, depth_data_size, 
            palette_id, flags, frame_rows, frame_cols, frame_width, frame_height))
        
        # Color data
        sprite_data.extend(bytes(color_data))
        
        # Depth runs
        for depth, distance in depth_runs:
            sprite_data.extend(struct.pack('<BH', depth, distance))
        
        print(f"Compiled sprite: {os.path.basename(png_path)} ({width}x{height}) -> {len(sprite_data)} bytes")
        print(f"  Color data: {color_data_size} bytes, Depth runs: {len(depth_runs)} ({depth_data_size} bytes)")
        
        return bytes(sprite_data)
        
    except Exception as e:
        print(f"Error compiling sprite {png_path}: {e}")
        return None

if __name__ == '__main__':
    import sys
    if len(sys.argv) < 3:
        print("Usage: python wisp_builder.py <app_folder> <output.wisp>")
        print("       Creates a WISP bundle from app folder")
        exit(1)

    folder = sys.argv[1]
    out = sys.argv[2]
    
    # Validate app configuration
    if not validate_app_config(folder):
        exit(1)
    
    entries, payloads, config_data = collect_assets(folder)
    if not entries and not config_data:
        print("No assets or config found to pack!")
        exit(1)
        
    write_wisp_bundle(entries, payloads, config_data, out)
