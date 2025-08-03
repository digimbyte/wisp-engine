// examples/snake_game_native.cpp
// Example of a complete native C++ app for WispEngine
// This shows how simple and performant native apps can be

#include "../src/engine/wisp_app_interface.h"
#include <vector>

class SnakeGame : public WispAppBase {
private:
    // Game constants
    static const uint8_t GRID_SIZE = 8;
    static const uint8_t GRID_WIDTH = 20;
    static const uint8_t GRID_HEIGHT = 15;
    static const uint8_t MAX_SNAKE_LENGTH = 300; // Max possible snake length
    
    // Game state
    struct Position { uint8_t x, y; };
    Position snake[MAX_SNAKE_LENGTH];
    uint16_t snakeLength;
    Position food;
    Position direction;
    uint32_t score;
    uint32_t gameSpeed;
    uint32_t lastMoveTime;
    bool gameOver;
    
    // Sprites (would be loaded from assets)
    uint16_t snakeHeadSprite;
    uint16_t snakeBodySprite;
    uint16_t foodSprite;
    uint16_t wallSprite;
    
    // Input state
    Position nextDirection;
    bool inputReceived;
    
public:
    // App identification
    const char* getAppName() const override { return "Snake Game"; }
    const char* getAppVersion() const override { return "1.0.0"; }
    const char* getAppAuthor() const override { return "WispEngine Team"; }
    
    // Performance settings
    uint8_t getTargetFPS() const override { return 60; } // Smooth input
    uint8_t getMinimumFPS() const override { return 30; }
    bool allowAdaptiveFrameRate() const override { return true; }
    
protected:
    bool initializeApp() override {
        // Initialize game state
        resetGame();
        
        // Load sprites (placeholder - would load from SD card)
        snakeHeadSprite = 0; // TODO: Load actual sprites
        snakeBodySprite = 1;
        foodSprite = 2;
        wallSprite = 3;
        
        Serial.println("Snake Game: Initialized");
        Serial.println("Controls: Arrow keys to move");
        Serial.println("Goal: Eat food to grow and increase score");
        
        return true;
    }
    
    void updateApp(float deltaTime) override {
        if (gameOver) {
            // Check for restart input
            return;
        }
        
        // Update game speed based on score
        gameSpeed = 300 - (score * 10); // Get faster as score increases
        if (gameSpeed < 100) gameSpeed = 100; // Cap minimum speed
        
        // Check if it's time to move
        uint32_t currentTime = millis();
        if (currentTime - lastMoveTime < gameSpeed) {
            return;
        }
        
        // Update direction from input
        if (inputReceived) {
            // Prevent reversing into self
            if (!(nextDirection.x == -direction.x && nextDirection.y == -direction.y)) {
                direction = nextDirection;
            }
            inputReceived = false;
        }
        
        // Move snake
        moveSnake();
        
        // Check collisions
        if (checkCollisions()) {
            gameOver = true;
            Serial.print("Game Over! Final Score: ");
            Serial.println(score);
            return;
        }
        
        // Check food collision
        if (checkFoodCollision()) {
            eatFood();
            spawnFood();
        }
        
        lastMoveTime = currentTime;
    }
    
    void renderApp(GraphicsEngine* gfx) override {
        // Clear screen
        gfx->clearBuffers(0x0000); // Black background
        
        // Draw walls
        drawWalls(gfx);
        
        // Draw food
        drawFood(gfx);
        
        // Draw snake
        drawSnake(gfx);
        
        // Draw UI
        drawUI(gfx);
        
        if (gameOver) {
            drawGameOver(gfx);
        }
    }
    
    void onButtonPress(uint8_t button) override {
        if (gameOver) {
            if (button == 4) { // Center button - restart
                resetGame();
            }
            return;
        }
        
        // Handle direction input
        switch (button) {
            case 0: // Up
                nextDirection = {0, -1};
                inputReceived = true;
                break;
            case 1: // Down
                nextDirection = {0, 1};
                inputReceived = true;
                break;
            case 2: // Left
                nextDirection = {-1, 0};
                inputReceived = true;
                break;
            case 3: // Right
                nextDirection = {1, 0};
                inputReceived = true;
                break;
        }
    }
    
    void cleanupApp() override {
        snakeLength = 0;
        Serial.println("Snake Game: Cleaned up");
    }

private:
    void resetGame() {
        snakeLength = 2;
        snake[0] = {GRID_WIDTH / 2, GRID_HEIGHT / 2}; // Start in center
        snake[1] = {GRID_WIDTH / 2 - 1, GRID_HEIGHT / 2}; // Initial tail
        
        direction = {1, 0}; // Moving right
        nextDirection = direction;
        inputReceived = false;
        
        spawnFood();
        
        score = 0;
        gameSpeed = 300;
        lastMoveTime = 0;
        gameOver = false;
    }
    
    void moveSnake() {
        Position newHead = snake[0];
        newHead.x += direction.x;
        newHead.y += direction.y;
        
        // Shift all segments down and add new head
        for (int i = snakeLength; i > 0; i--) {
            snake[i] = snake[i - 1];
        }
        snake[0] = newHead;
    }
    
    bool checkCollisions() {
        const Position& head = snake[0];
        
        // Wall collision
        if (head.x >= GRID_WIDTH || head.y >= GRID_HEIGHT) {
            return true;
        }
        
        // Self collision
        for (uint16_t i = 1; i < snakeLength; i++) {
            if (head.x == snake[i].x && head.y == snake[i].y) {
                return true;
            }
        }
        
        return false;
    }
    
    bool checkFoodCollision() {
        const Position& head = snake[0];
        return head.x == food.x && head.y == food.y;
    }
    
    void eatFood() {
        score++;
        snakeLength++; // Snake grows by keeping the tail segment
        // Make sure we don't exceed maximum
        if (snakeLength > MAX_SNAKE_LENGTH) {
            snakeLength = MAX_SNAKE_LENGTH;
        }
        spawnFood();
    }
    
    void spawnFood() {
        // Simple food spawning - avoid snake body
        do {
            food.x = random(GRID_WIDTH);
            food.y = random(GRID_HEIGHT);
        } while (isFoodOnSnake());
    }
    
    bool isFoodOnSnake() {
        for (uint16_t i = 0; i < snakeLength; i++) {
            if (food.x == snake[i].x && food.y == snake[i].y) {
                return true;
            }
        }
        return false;
    }
    
    void drawWalls(GraphicsEngine* gfx) {
        // Draw border
        uint16_t wallColor = 0xFFFF; // White
        
        // Top and bottom walls
        for (uint8_t x = 0; x < GRID_WIDTH + 2; x++) {
            gfx->drawRect(x * GRID_SIZE, 0, GRID_SIZE, GRID_SIZE, wallColor);
            gfx->drawRect(x * GRID_SIZE, (GRID_HEIGHT + 1) * GRID_SIZE, GRID_SIZE, GRID_SIZE, wallColor);
        }
        
        // Left and right walls
        for (uint8_t y = 0; y < GRID_HEIGHT + 2; y++) {
            gfx->drawRect(0, y * GRID_SIZE, GRID_SIZE, GRID_SIZE, wallColor);
            gfx->drawRect((GRID_WIDTH + 1) * GRID_SIZE, y * GRID_SIZE, GRID_SIZE, GRID_SIZE, wallColor);
        }
    }
    
    void drawSnake(GraphicsEngine* gfx) {
        uint16_t headColor = 0x07E0; // Green
        uint16_t bodyColor = 0x0400; // Dark green
        
        for (uint16_t i = 0; i < snakeLength; i++) {
            const Position& segment = snake[i];
            uint16_t color = (i == 0) ? headColor : bodyColor;
            
            int16_t screenX = (segment.x + 1) * GRID_SIZE;
            int16_t screenY = (segment.y + 1) * GRID_SIZE;
            
            gfx->drawRect(screenX, screenY, GRID_SIZE - 1, GRID_SIZE - 1, color);
        }
    }
    
    void drawFood(GraphicsEngine* gfx) {
        uint16_t foodColor = 0xF800; // Red
        
        int16_t screenX = (food.x + 1) * GRID_SIZE;
        int16_t screenY = (food.y + 1) * GRID_SIZE;
        
        gfx->drawRect(screenX + 1, screenY + 1, GRID_SIZE - 3, GRID_SIZE - 3, foodColor);
    }
    
    void drawUI(GraphicsEngine* gfx) {
        // Draw score (simple pixel-based numbers)
        // This would typically use a font system
        uint16_t textColor = 0xFFFF;
        
        // For now, just draw score as rectangles
        int16_t scoreX = 10;
        int16_t scoreY = (GRID_HEIGHT + 2) * GRID_SIZE + 10;
        
        // Draw "SCORE: XXX" using simple rectangles
        for (uint32_t i = 0; i < score && i < 20; i++) {
            gfx->drawRect(scoreX + i * 4, scoreY, 2, 2, textColor);
        }
    }
    
    void drawGameOver(GraphicsEngine* gfx) {
        // Draw game over screen
        uint16_t overlayColor = 0x7800; // Dark red overlay
        
        // Semi-transparent overlay (simplified)
        for (int y = 50; y < 150; y++) {
            for (int x = 50; x < 200; x++) {
                if ((x + y) % 2 == 0) { // Dithering pattern
                    gfx->drawPixel(x, y, overlayColor);
                }
            }
        }
        
        // "GAME OVER" text (simplified)
        uint16_t textColor = 0xFFFF;
        gfx->drawRect(90, 90, 60, 8, textColor); // Simplified text
        gfx->drawRect(80, 110, 80, 6, textColor); // "Press center to restart"
    }
};

// Export the app - this is all that's needed to make it loadable
WISP_APP_EXPORT(SnakeGame);

/*
Compilation instructions:
1. Include this file in your PlatformIO project
2. Compile with: pio run
3. The resulting binary contains the complete game
4. Engine loads and runs it with zero interpretation overhead

Performance characteristics:
- 100% native speed
- ~2-4KB RAM usage
- No garbage collection
- Predictable frame timing
- Direct hardware access

Development workflow:
1. Write game in C++ using WispApp interface
2. Test in simulator or on device
3. Compile to binary
4. Distribute binary file
5. Engine loads and runs instantly
*/
