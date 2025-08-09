#pragma once
// ASH (App Script Hybrid) to WASH (Wisp ASH) Compiler
// Compiles human-readable ASH scripts into executable WASH code

#include "engine_common.h"
#include "../scene/scene_system.h"
#include "../app/curated_api_extended.h"

namespace WispEngine {
namespace Script {

// === ASH LANGUAGE SPECIFICATION ===

/*
ASH Language Features:
- Entity manipulation with UUID tracking
- Panel content management (tiles, backgrounds, camera)
- Event-driven execution (input, collision, timers, animations)
- Simple data types: int, float, string, bool, vector2, color
- Built-in functions for engine interaction
- C-style syntax with simplified constructs

Example ASH Script (Entity AI):
```ash
// Entity behavior script - gets compiled to WASH
entity_script "goblin_ai" {
    var health = 100;
    var speed = 2.0;
    var target_uuid = null;
    
    function onUpdate() {
        // Find nearest player
        var players = findEntitiesByType("player", getCurrentPanel());
        if (length(players) > 0) {
            target_uuid = players[0];
            var my_pos = getPosition(self);
            var target_pos = getPosition(target_uuid);
            
            // Move towards player
            var dx = target_pos.x - my_pos.x;
            var dy = target_pos.y - my_pos.y;
            var distance = sqrt(dx*dx + dy*dy);
            
            if (distance > 32) {
                moveEntity(self, dx/distance * speed, dy/distance * speed);
                setAnimation(self, "walk");
            } else {
                setVelocity(self, 0, 0);
                setAnimation(self, "idle");
            }
        }
    }
    
    function onCollision(other_uuid) {
        if (getEntityType(other_uuid) == "player") {
            health -= 10;
            playSound("hit");
            if (health <= 0) {
                destroyEntity(self);
            }
        }
    }
}
```

Example ASH Script (Panel Camera):
```ash
// Panel control script
panel_script "camera_controller" {
    var smooth_factor = 0.1;
    var boundary_margin = 60;
    
    function onUpdate() {
        var player = findEntitiesByType("player", self)[0];
        if (player != null) {
            var pos = getPosition(player);
            var cam_pos = getCameraPosition(self);
            var panel_size = getPanelSize(self);
            
            // Calculate target camera position with boundaries
            var target_x = clamp(pos.x, boundary_margin, panel_size.x - boundary_margin);
            var target_y = clamp(pos.y, boundary_margin, panel_size.y - boundary_margin);
            
            // Smooth interpolation
            var new_x = lerp(cam_pos.x, target_x, smooth_factor);
            var new_y = lerp(cam_pos.y, target_y, smooth_factor);
            
            setCameraPosition(self, new_x, new_y);
        }
    }
    
    function onInput(input_type) {
        if (input_type == INPUT_MENU) {
            // Toggle debug camera mode
            toggleFreeCameraMode(self);
        }
    }
}
```
*/

// === ASH TOKEN TYPES ===

enum ASHTokenType : uint8_t {
    TOKEN_UNKNOWN,
    
    // Literals
    TOKEN_NUMBER,       TOKEN_STRING,       TOKEN_BOOL,         TOKEN_NULL,
    
    // Identifiers and Keywords
    TOKEN_IDENTIFIER,   TOKEN_ENTITY_SCRIPT, TOKEN_PANEL_SCRIPT, TOKEN_GLOBAL_SCRIPT,
    TOKEN_FUNCTION,     TOKEN_VAR,          TOKEN_IF,           TOKEN_ELSE,
    TOKEN_WHILE,        TOKEN_FOR,          TOKEN_RETURN,       TOKEN_BREAK,
    TOKEN_CONTINUE,     TOKEN_TRUE,         TOKEN_FALSE,
    
    // Operators
    TOKEN_PLUS,         TOKEN_MINUS,        TOKEN_MULTIPLY,     TOKEN_DIVIDE,
    TOKEN_MODULO,       TOKEN_ASSIGN,       TOKEN_PLUS_ASSIGN,  TOKEN_MINUS_ASSIGN,
    TOKEN_EQUALS,       TOKEN_NOT_EQUALS,   TOKEN_LESS,         TOKEN_LESS_EQUAL,
    TOKEN_GREATER,      TOKEN_GREATER_EQUAL, TOKEN_AND,         TOKEN_OR,
    TOKEN_NOT,
    
    // Delimiters
    TOKEN_SEMICOLON,    TOKEN_COMMA,        TOKEN_DOT,          TOKEN_COLON,
    TOKEN_LEFT_PAREN,   TOKEN_RIGHT_PAREN,  TOKEN_LEFT_BRACE,   TOKEN_RIGHT_BRACE,
    TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET,
    
    // Special
    TOKEN_NEWLINE,      TOKEN_EOF,          TOKEN_ERROR
};

struct ASHToken {
    ASHTokenType type;
    String value;
    uint16_t line;
    uint16_t column;
    
    ASHToken() : type(TOKEN_UNKNOWN), line(0), column(0) {}
    ASHToken(ASHTokenType t, const String& v, uint16_t l, uint16_t c) 
        : type(t), value(v), line(l), column(c) {}
};

// === ASH AST (Abstract Syntax Tree) ===

enum ASHNodeType : uint8_t {
    NODE_SCRIPT,        // Root script node
    NODE_FUNCTION,      // Function declaration
    NODE_VARIABLE,      // Variable declaration
    NODE_BLOCK,         // Code block
    NODE_IF,            // If statement
    NODE_WHILE,         // While loop
    NODE_FOR,           // For loop
    NODE_RETURN,        // Return statement
    NODE_CALL,          // Function call
    NODE_ASSIGN,        // Assignment
    NODE_BINARY_OP,     // Binary operation
    NODE_UNARY_OP,      // Unary operation
    NODE_LITERAL,       // Literal value
    NODE_IDENTIFIER,    // Variable/function name
    NODE_MEMBER_ACCESS  // Object.member access
};

struct ASHNode {
    ASHNodeType type;
    String value;           // For literals and identifiers
    uint16_t line;          // Source line number
    
    // Child nodes
    static const uint8_t MAX_CHILDREN = 8;
    ASHNode* children[MAX_CHILDREN];
    uint8_t childCount;
    
    // Node-specific data
    union {
        struct { ASHTokenType operator_type; } binaryOp;
        struct { String function_name; uint8_t param_count; } call;
        struct { String var_name; bool is_global; } variable;
        struct { String func_name; uint8_t param_count; } function;
    };
    
    ASHNode() : type(NODE_LITERAL), line(0), childCount(0) {
        memset(children, 0, sizeof(children));
        memset(&binaryOp, 0, sizeof(binaryOp));
    }
    
    ~ASHNode() {
        for (uint8_t i = 0; i < childCount; i++) {
            delete children[i];
        }
    }
    
    void addChild(ASHNode* child) {
        if (childCount < MAX_CHILDREN) {
            children[childCount++] = child;
        }
    }
};

// === ASH COMPILER ===

enum CompilerOutputType : uint8_t {
    OUTPUT_CPP,         // Generate C++ code that links with curated API
    OUTPUT_BYTECODE,    // Generate custom bytecode for VM execution
    OUTPUT_ASSEMBLY     // Generate ESP32 assembly (advanced)
};

struct CompilerConfig {
    CompilerOutputType outputType;
    bool optimizations;
    bool debugInfo;
    bool safetyChecks;
    String targetPlatform;  // "esp32c6", "esp32s3", etc.
    
    CompilerConfig() : outputType(OUTPUT_CPP), optimizations(true), 
                      debugInfo(true), safetyChecks(true), targetPlatform("esp32c6") {}
};

class ASHCompiler {
private:
    // Lexical analysis
    const char* source;
    uint16_t sourceLength;
    uint16_t currentPos;
    uint16_t currentLine;
    uint16_t currentColumn;
    
    // Token stream
    static const uint16_t MAX_TOKENS = 1024;
    ASHToken tokens[MAX_TOKENS];
    uint16_t tokenCount;
    uint16_t currentToken;
    
    // Parser state
    ASHNode* rootNode;
    String currentScriptType;  // "entity_script", "panel_script", "global_script"
    String currentScriptName;
    
    // Symbol table
    struct Symbol {
        String name;
        String type;
        bool isGlobal;
        uint16_t line;
    };
    static const uint16_t MAX_SYMBOLS = 128;
    Symbol symbolTable[MAX_SYMBOLS];
    uint16_t symbolCount;
    
    // Error handling
    struct CompilerError {
        String message;
        uint16_t line;
        uint16_t column;
    };
    static const uint8_t MAX_ERRORS = 32;
    CompilerError errors[MAX_ERRORS];
    uint8_t errorCount;
    
    // Code generation
    String generatedCode;
    CompilerConfig config;
    
public:
    ASHCompiler();
    ~ASHCompiler();
    
    // === MAIN COMPILATION INTERFACE ===
    
    bool compile(const String& ashSource, const String& outputPath, 
                const CompilerConfig& cfg = CompilerConfig());
    bool compileToString(const String& ashSource, String& output, 
                        const CompilerConfig& cfg = CompilerConfig());
    
    // === ERROR REPORTING ===
    
    bool hasErrors() const { return errorCount > 0; }
    uint8_t getErrorCount() const { return errorCount; }
    String getError(uint8_t index) const;
    void dumpErrors() const;
    
    // === LEXER ===
    
    bool tokenize();
    void dumpTokens() const;
    
private:
    // Lexical analysis
    char peekChar(int offset = 0);
    char nextChar();
    void skipWhitespace();
    void skipComment();
    ASHToken readNumber();
    ASHToken readString();
    ASHToken readIdentifier();
    ASHToken readOperator();
    
    // Parser
    bool parse();
    ASHNode* parseScript();
    ASHNode* parseFunction();
    ASHNode* parseStatement();
    ASHNode* parseExpression();
    ASHNode* parsePrimary();
    ASHNode* parseCall();
    ASHNode* parseBinaryOp(int precedence);
    ASHNode* parseAssignment();
    
    // Semantic analysis
    bool analyze();
    bool checkTypes(ASHNode* node);
    bool resolveSymbols(ASHNode* node);
    void addSymbol(const String& name, const String& type, bool isGlobal = false);
    Symbol* findSymbol(const String& name);
    
    // Code generation
    bool generateCode();
    bool generateCPP();
    bool generateBytecode();
    bool generateAssembly();
    
    // C++ code generation helpers
    String generateCPPHeader();
    String generateCPPFunction(ASHNode* funcNode);
    String generateCPPStatement(ASHNode* stmtNode);
    String generateCPPExpression(ASHNode* exprNode);
    String generateCPPCall(ASHNode* callNode);
    
    // Built-in function mapping (ASH → C++ API)
    String mapBuiltinFunction(const String& ashFunction);
    bool isBuiltinFunction(const String& name);
    
    // Error handling
    void addError(const String& message, uint16_t line = 0, uint16_t column = 0);
    void addError(const String& message, const ASHToken& token);
    
    // Utility
    bool match(ASHTokenType type);
    bool check(ASHTokenType type);
    ASHToken advance();
    ASHToken peek();
    ASHToken previous();
    bool isAtEnd();
};

// === ASH TO WASH RUNTIME ===

struct WASHExecutable {
    String name;
    String sourceHash;      // Hash of original ASH source
    uint32_t compileTime;   // Compilation timestamp
    
    // Script metadata
    String scriptType;      // "entity", "panel", "global"
    String scriptName;
    
    // Generated code (one of these will be populated)
    String cppCode;         // Generated C++ code
    uint8_t* bytecode;      // Generated bytecode
    uint16_t bytecodeSize;
    
    // Function entry points
    struct EntryPoint {
        String functionName;
        uint32_t offset;    // Offset in code/bytecode
    };
    static const uint8_t MAX_ENTRY_POINTS = 16;
    EntryPoint entryPoints[MAX_ENTRY_POINTS];
    uint8_t entryPointCount;
    
    WASHExecutable() : compileTime(0), bytecode(nullptr), bytecodeSize(0), entryPointCount(0) {}
    ~WASHExecutable() { if (bytecode) delete[] bytecode; }
};

// === ASH BUILT-IN FUNCTIONS (Available in ASH scripts) ===

/*
These functions are automatically mapped to C++ API calls during compilation:

Entity Manipulation:
- moveEntity(uuid, dx, dy) → api->moveEntity(uuid, dx, dy)
- setPosition(uuid, x, y) → api->setEntityPosition(uuid, x, y)
- getPosition(uuid) → api->getEntityPosition(uuid)
- setVelocity(uuid, vx, vy) → api->setEntityVelocity(uuid, vx, vy)
- getVelocity(uuid) → api->getEntityVelocity(uuid)
- setSprite(uuid, spriteId) → api->setEntitySprite(uuid, spriteId)
- setAnimation(uuid, name) → api->setEntityAnimation(uuid, name)
- setLayer(uuid, layer) → api->setEntityLayer(uuid, layer)
- setVisible(uuid, visible) → api->setEntityVisible(uuid, visible)
- destroyEntity(uuid) → api->destroyEntity(uuid)

Panel Manipulation:
- setCameraPosition(panelId, x, y) → api->setPanelCamera(panelId, x, y)
- getCameraPosition(panelId) → api->getPanelCamera(panelId)
- addTile(panelId, spriteId, x, y, layer) → api->addPanelTile(panelId, spriteId, x, y, layer)
- removeTile(panelId, x, y) → api->removePanelTile(panelId, x, y)
- setBackground(panelId, spriteId) → api->setPanelBackground(panelId, spriteId)
- focusOnEntity(panelId, uuid, speed) → api->focusPanelOnEntity(panelId, uuid, speed)

Entity Search/Creation:
- spawnEntity(panelId, spriteId, x, y, scriptName) → api->spawnEntity(panelId, spriteId, x, y, scriptName)
- findEntitiesByType(type, panelId) → api->findEntitiesByType(type, panelId)
- findEntitiesInRadius(x, y, radius, panelId) → api->findEntitiesInRadius(x, y, radius, panelId)

System Functions:
- playSound(soundId, volume) → api->playSound(soundId, volume)
- saveData(key, value) → api->saveData(key, value)
- loadData(key) → api->loadData(key)
- setTimer(timerId, delayMs, repeat) → api->setTimer(timerId, delayMs, repeat)
- log(message) → api->logMessage(message)

Math/Utility:
- sqrt(x), sin(x), cos(x), abs(x), clamp(x, min, max), lerp(a, b, t)
- length(array), random(), randomRange(min, max)

Special Variables:
- self → Current entity/panel UUID
- currentPanel → Current panel ID
- deltaTime → Frame delta time
- INPUT_UP, INPUT_DOWN, etc. → Input constants
*/

} // namespace Script
} // namespace WispEngine
