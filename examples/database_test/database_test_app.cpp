// examples/database_test_app.cpp - Database System Test
// Tests CRUD operations, field management, and database integrity

#include "../src/engine/app/interface.h"

class DatabaseTestApp : public WispAppBase {
private:
    // Database test modes
    enum DatabaseTestMode {
        TEST_BASIC_CRUD,    // Create, Read, Update, Delete
        TEST_FIELD_MGMT,    // Field management and validation
        TEST_BATCH_OPS,     // Batch operations and performance
        TEST_DATA_INTEGRITY, // Data integrity and validation
        TEST_COUNT
    };
    
    DatabaseTestMode currentMode = TEST_BASIC_CRUD;
    
    // Test data structures
    struct TestPokemon {
        uint32_t id;
        std::string name;
        std::string type1;
        std::string type2;
        uint16_t hp;
        uint16_t attack;
        uint16_t defense;
        uint8_t level;
        bool shiny;
        float experience;
    };
    
    struct TestTrainer {
        uint32_t id;
        std::string name;
        uint8_t badges;
        uint32_t money;
        std::vector<uint32_t> pokemonIds;
    };
    
    // Database handles
    DatabaseHandle pokemonDB;
    DatabaseHandle trainerDB;
    DatabaseHandle itemDB;
    
    // Test state
    uint32_t currentRecordId = 1;
    uint32_t totalRecords = 0;
    uint32_t lastOperationTime = 0;
    std::string lastOperationResult = "";
    bool databaseInitialized = false;
    
    // Batch test state
    uint32_t batchSize = 100;
    uint32_t batchProgress = 0;
    bool batchInProgress = false;
    uint32_t batchStartTime = 0;
    
    // Sample data
    std::vector<std::string> pokemonNames = {
        "Pikachu", "Charizard", "Blastoise", "Venusaur", "Alakazam",
        "Machamp", "Gengar", "Dragonite", "Mewtwo", "Mew"
    };
    
    std::vector<std::string> pokemonTypes = {
        "Electric", "Fire", "Water", "Grass", "Psychic",
        "Fighting", "Ghost", "Dragon", "Normal", "Flying"
    };
    
    std::vector<std::string> trainerNames = {
        "Ash", "Misty", "Brock", "Gary", "Prof Oak",
        "Team Rocket", "Elite Four", "Gym Leader"
    };

public:
    bool init() override {
        setAppInfo("Database Test", "1.0.0", "Wisp Engine Team");
        
        // Initialize database system
        if (initializeDatabases()) {
            databaseInitialized = true;
            api->print("Database Test App initialized");
        } else {
            api->print("Failed to initialize databases");
            return false;
        }
        
        api->print("Controls: Up/Down - Mode, A - Execute, B - Next Record");
        api->print("Left/Right - Batch Size, Start - Batch Test");
        return true;
    }
    
    bool initializeDatabases() {
        // Create Pokemon database
        pokemonDB = api->createDatabase("test_pokemon.wdb");
        if (pokemonDB.isValid()) {
            // Register Pokemon fields
            api->registerField(pokemonDB, "id", FIELD_UINT32, true);        // Primary key
            api->registerField(pokemonDB, "name", FIELD_STRING, false);
            api->registerField(pokemonDB, "type1", FIELD_STRING, false);
            api->registerField(pokemonDB, "type2", FIELD_STRING, false);
            api->registerField(pokemonDB, "hp", FIELD_UINT16, false);
            api->registerField(pokemonDB, "attack", FIELD_UINT16, false);
            api->registerField(pokemonDB, "defense", FIELD_UINT16, false);
            api->registerField(pokemonDB, "level", FIELD_UINT8, false);
            api->registerField(pokemonDB, "shiny", FIELD_BOOL, false);
            api->registerField(pokemonDB, "experience", FIELD_FLOAT, false);
            
            api->print("Pokemon database created");
        } else {
            api->print("Failed to create Pokemon database");
            return false;
        }
        
        // Create Trainer database
        trainerDB = api->createDatabase("test_trainers.wdb");
        if (trainerDB.isValid()) {
            api->registerField(trainerDB, "id", FIELD_UINT32, true);
            api->registerField(trainerDB, "name", FIELD_STRING, false);
            api->registerField(trainerDB, "badges", FIELD_UINT8, false);
            api->registerField(trainerDB, "money", FIELD_UINT32, false);
            
            api->print("Trainer database created");
        } else {
            api->print("Failed to create Trainer database");
            return false;
        }
        
        // Create Item database
        itemDB = api->createDatabase("test_items.wdb");
        if (itemDB.isValid()) {
            api->registerField(itemDB, "id", FIELD_UINT32, true);
            api->registerField(itemDB, "name", FIELD_STRING, false);
            api->registerField(itemDB, "quantity", FIELD_UINT16, false);
            api->registerField(itemDB, "price", FIELD_UINT32, false);
            
            api->print("Item database created");
        } else {
            api->print("Failed to create Item database");
            return false;
        }
        
        // Get initial record count
        totalRecords = api->getRecordCount(pokemonDB);
        
        return true;
    }
    
    std::string getModeName(DatabaseTestMode mode) {
        const char* names[] = {"Basic CRUD", "Field Management", "Batch Operations", "Data Integrity"};
        return names[mode];
    }
    
    void update() override {
        if (!databaseInitialized) return;
        
        uint32_t currentTime = api->getTime();
        
        // Handle input
        const WispInputState& input = api->getInput();
        static WispInputState lastInput;
        
        // Mode selection
        if (input.up && !lastInput.up) {
            currentMode = (DatabaseTestMode)((currentMode + 1) % TEST_COUNT);
            api->print("Database Mode: " + getModeName(currentMode));
        }
        if (input.down && !lastInput.down) {
            currentMode = (DatabaseTestMode)((currentMode - 1 + TEST_COUNT) % TEST_COUNT);
            api->print("Database Mode: " + getModeName(currentMode));
        }
        
        // Batch size adjustment
        if (input.left && !lastInput.left && currentMode == TEST_BATCH_OPS) {
            batchSize = std::max(10u, batchSize - 10);
            api->print("Batch Size: " + std::to_string(batchSize));
        }
        if (input.right && !lastInput.right && currentMode == TEST_BATCH_OPS) {
            batchSize = std::min(1000u, batchSize + 10);
            api->print("Batch Size: " + std::to_string(batchSize));
        }
        
        // Execute current test
        if (input.buttonA && !lastInput.buttonA && !batchInProgress) {
            executeCurrentTest();
        }
        
        // Next record
        if (input.buttonB && !lastInput.buttonB) {
            currentRecordId++;
            if (currentRecordId > totalRecords) currentRecordId = 1;
            api->print("Record ID: " + std::to_string(currentRecordId));
        }
        
        // Batch test
        if (input.start && !lastInput.start && currentMode == TEST_BATCH_OPS) {
            startBatchTest();
        }
        
        lastInput = input;
        
        // Update batch progress
        if (batchInProgress) {
            updateBatchTest();
        }
    }
    
    void executeCurrentTest() {
        uint32_t startTime = api->getTime();
        
        switch (currentMode) {
            case TEST_BASIC_CRUD:
                executeCRUDTest();
                break;
            case TEST_FIELD_MGMT:
                executeFieldTest();
                break;
            case TEST_BATCH_OPS:
                executeSingleOperation();
                break;
            case TEST_DATA_INTEGRITY:
                executeIntegrityTest();
                break;
        }
        
        lastOperationTime = api->getTime() - startTime;
        totalRecords = api->getRecordCount(pokemonDB);
    }
    
    void executeCRUDTest() {
        // Create a new Pokemon record
        TestPokemon pokemon;
        pokemon.id = currentRecordId;
        pokemon.name = pokemonNames[api->randomInt(0, pokemonNames.size() - 1)];
        pokemon.type1 = pokemonTypes[api->randomInt(0, pokemonTypes.size() - 1)];
        pokemon.type2 = (api->randomInt(0, 2) == 0) ? "" : pokemonTypes[api->randomInt(0, pokemonTypes.size() - 1)];
        pokemon.hp = api->randomInt(20, 255);
        pokemon.attack = api->randomInt(10, 200);
        pokemon.defense = api->randomInt(10, 200);
        pokemon.level = api->randomInt(1, 100);
        pokemon.shiny = api->randomInt(0, 100) < 5; // 5% shiny chance
        pokemon.experience = api->random(0.0f, 1000000.0f);
        
        // CREATE operation
        DatabaseRecord record;
        record.setField("id", pokemon.id);
        record.setField("name", pokemon.name);
        record.setField("type1", pokemon.type1);
        record.setField("type2", pokemon.type2);
        record.setField("hp", pokemon.hp);
        record.setField("attack", pokemon.attack);
        record.setField("defense", pokemon.defense);
        record.setField("level", pokemon.level);
        record.setField("shiny", pokemon.shiny);
        record.setField("experience", pokemon.experience);
        
        if (api->insertRecord(pokemonDB, record)) {
            // READ operation
            auto readRecord = api->getRecord(pokemonDB, currentRecordId);
            if (readRecord.isValid()) {
                // UPDATE operation
                readRecord.setField("level", pokemon.level + 1);
                readRecord.setField("experience", pokemon.experience + 100.0f);
                
                if (api->updateRecord(pokemonDB, readRecord)) {
                    lastOperationResult = "CREATE/READ/UPDATE successful for " + pokemon.name;
                } else {
                    lastOperationResult = "UPDATE failed";
                }
            } else {
                lastOperationResult = "READ failed";
            }
        } else {
            lastOperationResult = "CREATE failed";
        }
    }
    
    void executeFieldTest() {
        // Test field validation and constraints
        DatabaseRecord testRecord;
        testRecord.setField("id", currentRecordId);
        
        // Test string field limits
        std::string longName(256, 'A'); // Very long name
        testRecord.setField("name", longName);
        
        // Test numeric field ranges
        testRecord.setField("hp", 65535); // Max uint16
        testRecord.setField("level", 255); // Max uint8
        
        // Test required field validation
        if (api->validateRecord(pokemonDB, testRecord)) {
            lastOperationResult = "Field validation passed";
        } else {
            lastOperationResult = "Field validation failed (expected)";
        }
        
        // Test field type conversion
        testRecord.setField("hp", "150"); // String to uint16
        testRecord.setField("shiny", 1);   // int to bool
        
        if (api->insertRecord(pokemonDB, testRecord)) {
            lastOperationResult = "Field type conversion successful";
        } else {
            lastOperationResult = "Field type conversion failed";
        }
    }
    
    void executeSingleOperation() {
        // Single record operation for comparison
        TestPokemon pokemon;
        pokemon.id = totalRecords + 1;
        pokemon.name = "Single_" + std::to_string(pokemon.id);
        pokemon.type1 = "Normal";
        pokemon.hp = 100;
        pokemon.attack = 100;
        pokemon.defense = 100;
        pokemon.level = 50;
        pokemon.shiny = false;
        pokemon.experience = 50000.0f;
        
        DatabaseRecord record;
        record.setField("id", pokemon.id);
        record.setField("name", pokemon.name);
        record.setField("type1", pokemon.type1);
        record.setField("hp", pokemon.hp);
        record.setField("attack", pokemon.attack);
        record.setField("defense", pokemon.defense);
        record.setField("level", pokemon.level);
        record.setField("shiny", pokemon.shiny);
        record.setField("experience", pokemon.experience);
        
        if (api->insertRecord(pokemonDB, record)) {
            lastOperationResult = "Single record inserted";
        } else {
            lastOperationResult = "Single record insert failed";
        }
    }
    
    void executeIntegrityTest() {
        // Test database integrity constraints
        
        // Test duplicate primary key
        DatabaseRecord duplicateRecord;
        duplicateRecord.setField("id", 1); // Assuming ID 1 exists
        duplicateRecord.setField("name", "Duplicate Test");
        
        if (!api->insertRecord(pokemonDB, duplicateRecord)) {
            lastOperationResult = "Duplicate key rejection: PASS";
        } else {
            lastOperationResult = "Duplicate key rejection: FAIL";
        }
        
        // Test foreign key constraints (if implemented)
        // Test data consistency
        // Test transaction rollback (if supported)
    }
    
    void startBatchTest() {
        batchInProgress = true;
        batchProgress = 0;
        batchStartTime = api->getTime();
        
        api->print("Starting batch test: " + std::to_string(batchSize) + " records");
    }
    
    void updateBatchTest() {
        if (batchProgress >= batchSize) {
            // Batch complete
            uint32_t elapsed = api->getTime() - batchStartTime;
            float recordsPerSecond = (float)batchSize / (elapsed / 1000.0f);
            
            lastOperationResult = "Batch complete: " + std::to_string(recordsPerSecond) + " records/sec";
            batchInProgress = false;
            api->print("Batch test completed");
            return;
        }
        
        // Insert a batch of records (process a few per frame)
        int recordsThisFrame = std::min(5, (int)(batchSize - batchProgress));
        
        for (int i = 0; i < recordsThisFrame; i++) {
            uint32_t id = totalRecords + batchProgress + i + 1;
            
            DatabaseRecord record;
            record.setField("id", id);
            record.setField("name", "Batch_" + std::to_string(id));
            record.setField("type1", pokemonTypes[id % pokemonTypes.size()]);
            record.setField("hp", (uint16_t)(100 + (id % 100)));
            record.setField("attack", (uint16_t)(80 + (id % 80)));
            record.setField("defense", (uint16_t)(60 + (id % 60)));
            record.setField("level", (uint8_t)(1 + (id % 99)));
            record.setField("shiny", (id % 20) == 0);
            record.setField("experience", (float)(id * 100));
            
            api->insertRecord(pokemonDB, record);
        }
        
        batchProgress += recordsThisFrame;
    }
    
    void render() override {
        // Clear with dark background
        api->drawRect(0, 0, 320, 240, WispColor(20, 10, 30), 0);
        
        // Draw title
        api->drawText("DATABASE TEST", 160, 10, WispColor(255, 255, 255), 10);
        
        // Draw current mode
        api->drawText(getModeName(currentMode), 160, 25, WispColor(200, 200, 255), 9);
        
        if (!databaseInitialized) {
            api->drawText("Database initialization failed", 160, 120, WispColor(255, 0, 0), 8);
            return;
        }
        
        // Draw database status
        renderDatabaseStatus();
        
        // Draw mode-specific content
        switch (currentMode) {
            case TEST_BASIC_CRUD:
                renderCRUDTest();
                break;
            case TEST_FIELD_MGMT:
                renderFieldTest();
                break;
            case TEST_BATCH_OPS:
                renderBatchTest();
                break;
            case TEST_DATA_INTEGRITY:
                renderIntegrityTest();
                break;
        }
        
        // Draw operation result
        renderOperationResult();
        
        // Draw controls
        api->drawText("Up/Down: Mode  A: Execute  B: Next Record", 10, 210, WispColor(180, 180, 180), 8);
        if (currentMode == TEST_BATCH_OPS) {
            api->drawText("Left/Right: Batch Size  Start: Batch Test", 10, 225, WispColor(180, 180, 180), 8);
        }
    }
    
    void renderDatabaseStatus() {
        int y = 45;
        
        api->drawText("Total Records: " + std::to_string(totalRecords), 10, y, WispColor(255, 255, 255), 8);
        api->drawText("Current ID: " + std::to_string(currentRecordId), 180, y, WispColor(200, 200, 200), 8);
        
        // Database size (simulated)
        uint32_t dbSize = totalRecords * 64; // Approximate bytes per record
        std::string sizeText = "DB Size: " + std::to_string(dbSize) + " bytes";
        api->drawText(sizeText, 10, y + 15, WispColor(200, 200, 200), 8);
        
        if (lastOperationTime > 0) {
            api->drawText("Last Op: " + std::to_string(lastOperationTime) + "ms", 180, y + 15, WispColor(200, 200, 200), 8);
        }
    }
    
    void renderCRUDTest() {
        int y = 85;
        
        api->drawText("Basic CRUD Operations", 10, y, WispColor(255, 255, 255), 8);
        api->drawText("CREATE - Insert new Pokemon record", 10, y + 15, WispColor(200, 200, 200), 8);
        api->drawText("READ   - Retrieve record by ID", 10, y + 30, WispColor(200, 200, 200), 8);
        api->drawText("UPDATE - Modify existing record", 10, y + 45, WispColor(200, 200, 200), 8);
        api->drawText("DELETE - Remove record (future)", 10, y + 60, WispColor(200, 200, 200), 8);
        
        api->drawText("Press A to test CRUD cycle", 10, y + 80, WispColor(255, 255, 0), 8);
    }
    
    void renderFieldTest() {
        int y = 85;
        
        api->drawText("Field Management & Validation", 10, y, WispColor(255, 255, 255), 8);
        api->drawText("• String length validation", 10, y + 15, WispColor(200, 200, 200), 8);
        api->drawText("• Numeric range checking", 10, y + 30, WispColor(200, 200, 200), 8);
        api->drawText("• Type conversion testing", 10, y + 45, WispColor(200, 200, 200), 8);
        api->drawText("• Required field enforcement", 10, y + 60, WispColor(200, 200, 200), 8);
        
        api->drawText("Press A to test field validation", 10, y + 80, WispColor(255, 255, 0), 8);
    }
    
    void renderBatchTest() {
        int y = 85;
        
        api->drawText("Batch Operations & Performance", 10, y, WispColor(255, 255, 255), 8);
        api->drawText("Batch Size: " + std::to_string(batchSize), 10, y + 15, WispColor(200, 200, 200), 8);
        
        if (batchInProgress) {
            float progress = (float)batchProgress / batchSize;
            api->drawText("Progress: " + std::to_string((int)(progress * 100)) + "%", 10, y + 30, WispColor(255, 255, 0), 8);
            
            // Progress bar
            int barWidth = 200;
            int barHeight = 10;
            api->drawRect(10, y + 45, barWidth, barHeight, WispColor(60, 60, 60), 3);
            api->drawRect(10, y + 45, (int)(progress * barWidth), barHeight, WispColor(0, 255, 0), 4);
            
            uint32_t elapsed = api->getTime() - batchStartTime;
            api->drawText("Elapsed: " + std::to_string(elapsed / 1000) + "s", 10, y + 60, WispColor(200, 200, 200), 8);
        } else {
            api->drawText("Press Start to begin batch test", 10, y + 30, WispColor(255, 255, 0), 8);
            api->drawText("Press A for single operation", 10, y + 45, WispColor(200, 200, 200), 8);
        }
    }
    
    void renderIntegrityTest() {
        int y = 85;
        
        api->drawText("Data Integrity & Constraints", 10, y, WispColor(255, 255, 255), 8);
        api->drawText("• Primary key uniqueness", 10, y + 15, WispColor(200, 200, 200), 8);
        api->drawText("• Foreign key constraints", 10, y + 30, WispColor(200, 200, 200), 8);
        api->drawText("• Data consistency checks", 10, y + 45, WispColor(200, 200, 200), 8);
        api->drawText("• Transaction integrity", 10, y + 60, WispColor(200, 200, 200), 8);
        
        api->drawText("Press A to test constraints", 10, y + 80, WispColor(255, 255, 0), 8);
    }
    
    void renderOperationResult() {
        int y = 175;
        
        if (!lastOperationResult.empty()) {
            api->drawText("Result:", 10, y, WispColor(255, 255, 255), 8);
            
            // Color based on success/failure keywords
            WispColor resultColor = WispColor(200, 200, 200);
            if (lastOperationResult.find("successful") != std::string::npos ||
                lastOperationResult.find("PASS") != std::string::npos) {
                resultColor = WispColor(0, 255, 0);
            } else if (lastOperationResult.find("failed") != std::string::npos ||
                      lastOperationResult.find("FAIL") != std::string::npos) {
                resultColor = WispColor(255, 100, 100);
            }
            
            api->drawText(lastOperationResult, 10, y + 15, resultColor, 8);
        }
    }
    
    void cleanup() override {
        // Close databases
        if (pokemonDB.isValid()) {
            api->closeDatabase(pokemonDB);
        }
        if (trainerDB.isValid()) {
            api->closeDatabase(trainerDB);
        }
        if (itemDB.isValid()) {
            api->closeDatabase(itemDB);
        }
        
        api->print("Database Test App cleaned up");
    }
};

// Export function for the engine
extern "C" WispAppBase* createDatabaseTestApp() {
    return new DatabaseTestApp();
}

extern "C" void destroyDatabaseTestApp(WispAppBase* app) {
    delete app;
}
