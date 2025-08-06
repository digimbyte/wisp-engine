# Save System Test Application

## Purpose
Tests save/load operations, field serialization, file management, and data persistence for the Wisp Engine save system.

## Files
- `save_test_app.cpp` - Main test application

## Controls
- **Up/Down**: Switch save test modes
- **A Button**: Save game to current slot
- **B Button**: Load game from current slot
- **Left/Right**: Change save slot (0-2)
- **Start**: Toggle auto-save
- **Select**: Generate random test data

## Save Test Modes

### 1. Basic Save/Load
- Save current game state
- Load saved game state
- Save slot management
- Auto-save functionality

### 2. Field Types
- String field serialization
- Integer field handling
- Float/decimal precision
- Boolean flag storage
- Array/vector serialization

### 3. File Management
- Multiple save slots
- File size tracking
- Save slot information
- File operations

### 4. Corruption Test
- Corruption detection
- Recovery mechanisms
- Data validation
- Integrity checking

## Features Tested
- ✅ Save field registration
- ✅ Multi-slot save system
- ✅ Data serialization/deserialization
- ✅ File integrity validation
- ✅ Auto-save functionality
- ✅ Corruption detection
- ✅ Performance monitoring
- ✅ Memory usage tracking
- ✅ Error handling

## Save Data Structure
The test saves comprehensive game data:

### Player Data
- Player name and level
- Play time and experience
- Money and items
- Progress flags

### Game Progress
- Gym badges (array of booleans)
- Towns visited (array of booleans)
- Inventory items and quantities

### Settings
- Audio volume levels
- Text speed preference
- Animation toggles

### Statistics
- Battles won
- Pokemon caught
- Distance walked
- Timestamps

## Performance Metrics
Tracks save system performance:
- Save operation timing
- Load operation timing
- File size measurements
- Success/failure rates
- Total operations count

## Usage
This test validates that the save system can reliably store and retrieve complex game state, handle multiple save slots, and maintain data integrity across save/load cycles.
