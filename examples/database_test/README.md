# Database Test Application

## Purpose
Tests database CRUD operations, field management, batch processing, and data integrity for the Wisp Engine database system.

## Files
- `database_test_app.cpp` - Main test application

## Controls
- **Up/Down**: Switch database test modes
- **A Button**: Execute current test
- **B Button**: Next record
- **Left/Right**: Adjust batch size (Batch mode only)
- **Start**: Begin batch test (Batch mode only)

## Database Test Modes

### 1. Basic CRUD
- CREATE: Insert new records
- READ: Retrieve records by ID
- UPDATE: Modify existing records
- DELETE: Remove records (planned)

### 2. Field Management
- Field type validation
- String length limits
- Numeric range checking
- Required field enforcement
- Type conversion testing

### 3. Batch Operations
- Bulk insert operations
- Performance testing with large datasets
- Progress tracking
- Throughput measurement

### 4. Data Integrity
- Primary key uniqueness
- Foreign key constraints
- Data consistency validation
- Constraint violation handling

## Features Tested
- ✅ Database creation and initialization
- ✅ Field registration and validation
- ✅ CRUD operation accuracy
- ✅ Batch processing performance
- ✅ Data type handling
- ✅ Constraint enforcement
- ✅ Error handling and recovery
- ✅ Memory usage optimization
- ✅ Transaction integrity

## Test Databases
The application creates three test databases:

### Pokemon Database
- ID (Primary Key)
- Name, Type1, Type2
- HP, Attack, Defense, Level
- Shiny flag, Experience

### Trainer Database
- ID (Primary Key)
- Name, Badge count
- Money, Pokemon IDs

### Item Database
- ID (Primary Key)
- Name, Quantity, Price

## Performance Metrics
Tracks important database metrics:
- Operation timing (saves/loads)
- Record count and database size
- Success/failure rates
- Memory usage patterns

## Usage
This test ensures the database system can handle game data reliably, maintain data integrity, and provide adequate performance for real-time game operations.
