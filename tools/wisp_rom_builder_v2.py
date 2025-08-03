#!/usr/bin/env python3
"""
Wisp Database ROM Builder V2
Generates optimized ROM data from YAML/JSON configuration files with partitioning support
"""

import yaml
import json
import struct
import argparse
import os
from typing import Dict, List, Any, Tuple

class WispROMBuilderV2:
    def __init__(self):
        self.items = []
        self.quests = []
        self.maps = []
        self.pokemon = []
        self.strings = {}
        self.binary_data = {}
        self.string_pool = {}  # For deduplication
        
    def load_yaml(self, filename: str) -> bool:
        """Load game data from YAML file"""
        try:
            with open(filename, 'r') as f:
                data = yaml.safe_load(f)
                
            if 'items' in data:
                self.items.extend(data['items'])
            if 'quests' in data:
                self.quests.extend(data['quests'])
            if 'maps' in data:
                self.maps.extend(data['maps'])
            if 'pokemon' in data:
                self.pokemon.extend(data['pokemon'])
            if 'strings' in data:
                self.strings.update(data['strings'])
                
            print(f"Loaded {filename}: {len(self.items)} items, {len(self.quests)} quests, {len(self.maps)} maps")
            return True
            
        except Exception as e:
            print(f"Error loading {filename}: {e}")
            return False
    
    def load_directory(self, directory: str) -> bool:
        """Load all YAML/JSON files from directory"""
        success = True
        for filename in os.listdir(directory):
            if filename.endswith(('.yml', '.yaml')):
                success &= self.load_yaml(os.path.join(directory, filename))
            elif filename.endswith('.json'):
                success &= self.load_json(os.path.join(directory, filename))
        return success
    
    def add_string(self, text: str) -> int:
        """Add string to pool and return offset"""
        if text in self.string_pool:
            return self.string_pool[text]
        
        offset = len(self.string_pool)
        self.string_pool[text] = offset
        return offset
    
    def generate_rom(self, max_size: int = 6144) -> bytes:
        """Generate optimized ROM data"""
        rom_data = bytearray()
        
        # ROM header (32 bytes to match WispPartitionHeader)
        rom_data.extend(b'WRPV')  # Magic: Wisp ROM Partition V2
        rom_data.extend(struct.pack('<H', 2))  # version
        
        # Reserve space for entry count and data size
        header_offset = len(rom_data)
        rom_data.extend(b'\x00' * 26)  # Rest of header (will fill later)
        
        # String pool section
        string_pool_offset = len(rom_data)
        string_data = bytearray()
        for text in sorted(self.string_pool.keys(), key=lambda x: self.string_pool[x]):
            string_data.extend(text.encode('utf-8'))
            string_data.append(0)  # null terminator
        
        rom_data.extend(string_data)
        
        # Entry data section
        entries = []
        
        # Add items to ROM
        for item in self.items:
            key = self._make_key(0x01, 0x01, item['id'])  # NS_GAME.CAT_ITEMS.id
            entry_data = self._pack_item(item)
            
            # Entry header: key(4) + type(1) + flags(1) + size(2) = 8 bytes
            entry_header = struct.pack('<LBBH', key, 0x06, 0x01, len(entry_data))  # STRUCT, READ_ONLY
            rom_data.extend(entry_header)
            rom_data.extend(entry_data)
            entries.append(key)
        
        # Add quests to ROM
        for quest in self.quests:
            key = self._make_key(0x01, 0x02, quest['id'])  # NS_GAME.CAT_QUESTS.id
            
            # Quest data: title_offset(2) + required_level(2) + description_offset(2)
            title_offset = self.add_string(quest.get('title', ''))
            desc_offset = self.add_string(quest.get('description', ''))
            entry_data = struct.pack('<HHH', title_offset, quest.get('required_level', 1), desc_offset)
            
            entry_header = struct.pack('<LBBH', key, 0x06, 0x01, len(entry_data))
            rom_data.extend(entry_header)
            rom_data.extend(entry_data)
            entries.append(key)
        
        # Add maps to ROM
        for map_def in self.maps:
            key = self._make_key(0x03, 0x04, map_def['id'])  # NS_WORLD.CAT_LOCATIONS.id
            name_offset = self.add_string(map_def.get('name', ''))
            entry_data = struct.pack('<HHH', name_offset, map_def.get('width', 20), map_def.get('height', 20))
            
            entry_header = struct.pack('<LBBH', key, 0x06, 0x01, len(entry_data))
            rom_data.extend(entry_header)
            rom_data.extend(entry_data)
            entries.append(key)
        
        # Add Pokemon to ROM
        for poke in self.pokemon:
            key = self._make_key(0x01, 0x07, poke['id'])  # NS_GAME.CAT_POKEMON.id
            name_offset = self.add_string(poke.get('name', ''))
            entry_data = struct.pack('<HBBHHHH', 
                name_offset,
                poke.get('type1', 0),
                poke.get('type2', 0),
                poke.get('base_hp', 50),
                poke.get('base_attack', 50),
                poke.get('base_defense', 50),
                poke.get('base_speed', 50)
            )
            
            entry_header = struct.pack('<LBBH', key, 0x06, 0x01, len(entry_data))
            rom_data.extend(entry_header)
            rom_data.extend(entry_data)
            entries.append(key)
        
        # Fill in header information
        entry_count = len(entries)
        data_size = len(rom_data) - 32  # Subtract header size
        checksum = sum(rom_data[32:]) & 0xFFFFFFFF  # Simple checksum
        
        # Pack header data (starting after magic and version)
        header_data = struct.pack('<HLLLL6L', 
            entry_count,         # entryCount
            data_size,           # dataSize  
            checksum,            # checksum
            0,                   # lastModified
            data_size - len(string_data),  # freeSpace
            0,                   # fragmentation
            0, 0, 0, 0, 0, 0     # reserved
        )
        
        # Write header data
        rom_data[6:32] = header_data
        
        if len(rom_data) > max_size:
            print(f"WARNING: ROM data ({len(rom_data)} bytes) exceeds max size ({max_size} bytes)")
        
        return bytes(rom_data)
    
    def _make_key(self, namespace: int, category: int, id: int) -> int:
        """Create nested key"""
        return (namespace << 24) | (category << 16) | id
    
    def _pack_item(self, item: Dict[str, Any]) -> bytes:
        """Pack item data into binary format"""
        name_offset = self.add_string(item.get('name', ''))
        desc_offset = self.add_string(item.get('description', ''))
        
        return struct.pack('<HBBHHH',
            name_offset,              # name string offset
            item.get('type', 1),      # item type
            item.get('rarity', 1),    # rarity
            item.get('value', 0),     # value
            item.get('properties', 0), # properties
            desc_offset               # description string offset
        )
    
    def generate_header(self, filename: str) -> bool:
        """Generate C header file with constants"""
        try:
            with open(filename, 'w') as f:
                f.write("// Auto-generated Wisp ROM constants\n")
                f.write("#pragma once\n\n")
                f.write("// Generated by WispROMBuilderV2\n\n")
                
                # Namespace and category constants
                f.write("// Namespaces\n")
                f.write("#define NS_SYSTEM   0x00\n")
                f.write("#define NS_GAME     0x01\n")
                f.write("#define NS_PLAYER   0x02\n")
                f.write("#define NS_WORLD    0x03\n\n")
                
                f.write("// Categories\n")
                f.write("#define CAT_ITEMS     0x01\n")
                f.write("#define CAT_QUESTS    0x02\n")
                f.write("#define CAT_LOCATIONS 0x04\n")
                f.write("#define CAT_POKEMON   0x07\n\n")
                
                # Item constants
                f.write("// Item IDs\n")
                for item in sorted(self.items, key=lambda x: x['id']):
                    name = item.get('name', f"ITEM_{item['id']}").upper().replace(' ', '_').replace('-', '_')
                    f.write(f"#define ITEM_{name} {item['id']}\n")
                
                f.write("\n// Quest IDs\n")
                for quest in sorted(self.quests, key=lambda x: x['id']):
                    name = quest.get('title', f"QUEST_{quest['id']}").upper().replace(' ', '_').replace('-', '_')
                    f.write(f"#define QUEST_{name} {quest['id']}\n")
                
                f.write("\n// Map IDs\n")
                for map_def in sorted(self.maps, key=lambda x: x['id']):
                    name = map_def.get('name', f"MAP_{map_def['id']}").upper().replace(' ', '_').replace('-', '_')
                    f.write(f"#define MAP_{name} {map_def['id']}\n")
                
                f.write("\n// Pokemon IDs\n")
                for poke in sorted(self.pokemon, key=lambda x: x['id']):
                    name = poke.get('name', f"POKEMON_{poke['id']}").upper().replace(' ', '_').replace('-', '_')
                    f.write(f"#define POKEMON_{name} {poke['id']}\n")
                
                # Key generation macros
                f.write("\n// Key generation macros\n")
                f.write("#define WISP_ITEM_KEY(id)     WISP_KEY_MAKE(NS_GAME, CAT_ITEMS, id)\n")
                f.write("#define WISP_QUEST_KEY(id)    WISP_KEY_MAKE(NS_GAME, CAT_QUESTS, id)\n")
                f.write("#define WISP_MAP_KEY(id)      WISP_KEY_MAKE(NS_WORLD, CAT_LOCATIONS, id)\n")
                f.write("#define WISP_POKEMON_KEY(id)  WISP_KEY_MAKE(NS_GAME, CAT_POKEMON, id)\n")
                
            return True
        except Exception as e:
            print(f"Error generating header: {e}")
            return False
    
    def write_rom_file(self, filename: str) -> bool:
        """Write ROM data to binary file"""
        try:
            rom_data = self.generate_rom()
            with open(filename, 'wb') as f:
                f.write(rom_data)
            print(f"Generated ROM file: {filename} ({len(rom_data)} bytes)")
            return True
        except Exception as e:
            print(f"Error writing ROM file: {e}")
            return False
    
    def write_c_array(self, filename: str, array_name: str = "WISP_ROM_DATA") -> bool:
        """Write ROM data as C array"""
        try:
            rom_data = self.generate_rom()
            with open(filename, 'w') as f:
                f.write(f"// Auto-generated Wisp ROM data ({len(rom_data)} bytes)\n")
                f.write(f"#include \"esp32_common.h\"  // ESP-IDF native headers\n\n")
                f.write(f"const uint8_t {array_name}[] PROGMEM = {{\n")
                
                for i in range(0, len(rom_data), 16):
                    chunk = rom_data[i:i+16]
                    hex_values = ', '.join(f'0x{b:02X}' for b in chunk)
                    f.write(f"    {hex_values},\n")
                
                f.write("};\n\n")
                f.write(f"const uint16_t {array_name}_SIZE = {len(rom_data)};\n")
                
            print(f"Generated C array: {filename}")
            return True
        except Exception as e:
            print(f"Error writing C array: {e}")
            return False
    
    def print_stats(self):
        """Print generation statistics"""
        print("\n=== Wisp ROM Builder V2 Statistics ===")
        print(f"Items: {len(self.items)}")
        print(f"Quests: {len(self.quests)}")
        print(f"Maps: {len(self.maps)}")
        print(f"Pokemon: {len(self.pokemon)}")
        print(f"Unique Strings: {len(self.string_pool)}")
        
        rom_data = self.generate_rom()
        total_entries = len(self.items) + len(self.quests) + len(self.maps) + len(self.pokemon)
        print(f"Total Entries: {total_entries}")
        print(f"ROM Size: {len(rom_data)} bytes")
        print(f"Average Entry Size: {len(rom_data) / max(total_entries, 1):.1f} bytes")
        print(f"String Pool Size: {sum(len(s) + 1 for s in self.string_pool.keys())} bytes")
        
        # Memory efficiency
        uncompressed_size = total_entries * 50  # Estimate 50 bytes per entry uncompressed
        efficiency = (1 - len(rom_data) / uncompressed_size) * 100 if uncompressed_size > 0 else 0
        print(f"Compression Efficiency: {efficiency:.1f}%")

def main():
    parser = argparse.ArgumentParser(description='Wisp Database ROM Builder V2')
    parser.add_argument('input', help='Input YAML file or directory')
    parser.add_argument('-o', '--output', default='wisp_game_rom.bin', help='Output ROM file')
    parser.add_argument('--header', help='Output C header file')
    parser.add_argument('--c-array', help='Output as C array file')
    parser.add_argument('--array-name', default='WISP_ROM_DATA', help='C array name')
    parser.add_argument('--stats', action='store_true', help='Print statistics')
    parser.add_argument('--max-size', type=int, default=6144, help='Maximum ROM size in bytes')
    
    args = parser.parse_args()
    
    builder = WispROMBuilderV2()
    
    # Load input data
    if os.path.isdir(args.input):
        if not builder.load_directory(args.input):
            return 1
    else:
        if not builder.load_yaml(args.input):
            return 1
    
    # Generate outputs
    if not builder.write_rom_file(args.output):
        return 1
    
    if args.header:
        builder.generate_header(args.header)
    
    if args.c_array:
        builder.write_c_array(args.c_array, args.array_name)
    
    if args.stats:
        builder.print_stats()
    
    print("\nROM generation complete!")
    return 0

if __name__ == '__main__':
    exit(main())
