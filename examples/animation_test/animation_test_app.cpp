// examples/animation_test_app.cpp - Animation System Test
// Tests sprite animation, timing, interpolation, and animation sequencing

#include "../src/engine/app/interface.h"

class AnimationTestApp : public WispAppBase {
private:
    // Animation types to test
    enum AnimationType {
        ANIM_SPRITE_FRAMES,    // Traditional frame-based animation
        ANIM_POSITION,         // Position interpolation  
        ANIM_SCALE,            // Scale animation
        ANIM_COLOR,            // Color transitions
        ANIM_ROTATION,         // Rotation animation
        ANIM_COUNT
    };
    
    struct AnimationTest {
        AnimationType type;
        float progress;        // 0.0 to 1.0
        float speed;          // Animation speed multiplier
        bool playing;
        bool loop;
        float startValue;
        float endValue;
        float currentValue;
        
        // For sprite animation
        uint32_t frameCount;
        uint32_t currentFrame;
        uint32_t lastFrameTime;
        uint32_t frameInterval;  // milliseconds per frame
    };
    
    AnimationTest animations[ANIM_COUNT];
    uint8_t selectedAnimation = 0;
    
    // Visual elements
    float testX = 160, testY = 120;
    float testScale = 1.0f;
    WispColor testColor = WispColor(255, 255, 255);
    float testRotation = 0.0f;
    
    // Sprite frame animation
    ResourceHandle animatedSprite = 0;
    uint32_t spriteFrames = 8;

public:
    bool init() override {
        setAppInfo("Animation Test", "1.0.0", "Wisp Engine Team");
        
        // Initialize different animation types
        animations[ANIM_SPRITE_FRAMES] = {
            ANIM_SPRITE_FRAMES, 0.0f, 1.0f, true, true, 0, 0, 0,
            8, 0, 0, 125  // 8 frames, 125ms per frame (8 FPS)
        };
        
        animations[ANIM_POSITION] = {
            ANIM_POSITION, 0.0f, 2.0f, true, true, 50.0f, 270.0f, 160.0f,
            0, 0, 0, 0
        };
        
        animations[ANIM_SCALE] = {
            ANIM_SCALE, 0.0f, 1.5f, true, true, 0.5f, 2.0f, 1.0f,
            0, 0, 0, 0
        };
        
        animations[ANIM_COLOR] = {
            ANIM_COLOR, 0.0f, 0.8f, true, true, 0.0f, 360.0f, 0.0f,
            0, 0, 0, 0
        };
        
        animations[ANIM_ROTATION] = {
            ANIM_ROTATION, 0.0f, 3.0f, true, true, 0.0f, 360.0f, 0.0f,
            0, 0, 0, 0
        };
        
        // Load animated sprite from assets folder
        animatedSprite = api->loadSprite("assets/anim_frames.spr");
        
        api->print("Animation Test App initialized");
        api->print("Controls: Up/Down - Select Animation, A - Play/Pause, B - Reset");
        return true;
    }
    
    void update() override {
        uint32_t currentTime = api->getTime();
        float deltaTime = api->getDeltaTime() / 1000.0f; // Convert to seconds
        
        // Handle input
        const WispInputState& input = api->getInput();
        static WispInputState lastInput;
        
        // Animation selection
        if (input.up && !lastInput.up) {
            selectedAnimation = (selectedAnimation + 1) % ANIM_COUNT;
            api->print("Selected: " + getAnimationName(selectedAnimation));
        }
        if (input.down && !lastInput.down) {
            selectedAnimation = (selectedAnimation - 1 + ANIM_COUNT) % ANIM_COUNT;
            api->print("Selected: " + getAnimationName(selectedAnimation));
        }
        
        // Play/Pause selected animation
        if (input.buttonA && !lastInput.buttonA) {
            animations[selectedAnimation].playing = !animations[selectedAnimation].playing;
            api->print(getAnimationName(selectedAnimation) + ": " + 
                      (animations[selectedAnimation].playing ? "PLAYING" : "PAUSED"));
        }
        
        // Reset selected animation
        if (input.buttonB && !lastInput.buttonB) {
            animations[selectedAnimation].progress = 0.0f;
            animations[selectedAnimation].currentFrame = 0;
            api->print(getAnimationName(selectedAnimation) + ": RESET");
        }
        
        // Speed control with left/right
        if (input.left && !lastInput.left) {
            animations[selectedAnimation].speed = std::max(0.1f, animations[selectedAnimation].speed - 0.2f);
            api->print("Speed: " + std::to_string(animations[selectedAnimation].speed));
        }
        if (input.right && !lastInput.right) {
            animations[selectedAnimation].speed = std::min(5.0f, animations[selectedAnimation].speed + 0.2f);
            api->print("Speed: " + std::to_string(animations[selectedAnimation].speed));
        }
        
        lastInput = input;
        
        // Update all animations
        for (int i = 0; i < ANIM_COUNT; i++) {
            updateAnimation(animations[i], deltaTime, currentTime);
        }
        
        // Apply animation results to visual elements
        applyAnimations();
    }
    
    void updateAnimation(AnimationTest& anim, float deltaTime, uint32_t currentTime) {
        if (!anim.playing) return;
        
        switch (anim.type) {
            case ANIM_SPRITE_FRAMES:
                // Frame-based animation
                if (currentTime - anim.lastFrameTime >= anim.frameInterval / anim.speed) {
                    anim.currentFrame = (anim.currentFrame + 1) % anim.frameCount;
                    anim.lastFrameTime = currentTime;
                    
                    if (anim.currentFrame == 0 && !anim.loop) {
                        anim.playing = false;
                    }
                }
                break;
                
            default:
                // Progress-based animation
                anim.progress += deltaTime * anim.speed;
                
                if (anim.progress >= 1.0f) {
                    if (anim.loop) {
                        anim.progress = anim.progress - 1.0f; // Wrap around
                    } else {
                        anim.progress = 1.0f;
                        anim.playing = false;
                    }
                }
                
                // Apply easing (simple sine wave for smooth animation)
                float easedProgress = (1.0f - cos(anim.progress * 3.14159f)) * 0.5f;
                anim.currentValue = api->lerp(anim.startValue, anim.endValue, easedProgress);
                break;
        }
    }
    
    void applyAnimations() {
        // Apply position animation
        testX = animations[ANIM_POSITION].currentValue;
        
        // Apply scale animation
        testScale = animations[ANIM_SCALE].currentValue;
        
        // Apply rotation animation
        testRotation = animations[ANIM_ROTATION].currentValue;
        
        // Apply color animation (HSV color wheel)
        float hue = animations[ANIM_COLOR].currentValue;
        testColor = hsvToRgb(hue, 1.0f, 1.0f);
    }
    
    WispColor hsvToRgb(float h, float s, float v) {
        // Simple HSV to RGB conversion
        float c = v * s;
        float x = c * (1.0f - abs(fmod(h / 60.0f, 2.0f) - 1.0f));
        float m = v - c;
        
        float r, g, b;
        if (h < 60) { r = c; g = x; b = 0; }
        else if (h < 120) { r = x; g = c; b = 0; }
        else if (h < 180) { r = 0; g = c; b = x; }
        else if (h < 240) { r = 0; g = x; b = c; }
        else if (h < 300) { r = x; g = 0; b = c; }
        else { r = c; g = 0; b = x; }
        
        return WispColor((r + m) * 255, (g + m) * 255, (b + m) * 255);
    }
    
    std::string getAnimationName(uint8_t index) {
        const char* names[] = {"Sprite Frames", "Position", "Scale", "Color", "Rotation"};
        return names[index];
    }
    
    void render() override {
        // Clear with dark background
        api->drawRect(0, 0, 320, 240, WispColor(15, 15, 25), 0);
        
        // Draw title
        api->drawText("ANIMATION TEST", 160, 10, WispColor(255, 255, 255), 10);
        
        // Draw animated test object
        WispColor objColor = testColor;
        if (selectedAnimation == ANIM_COLOR) {
            // Highlight color animation
            objColor = WispColor(255, 255, 255);
        }
        
        // Draw test rectangle (representing animated sprite)
        float size = 20 * testScale;
        api->drawRect(testX - size/2, testY - size/2, size, size, objColor, 5);
        
        // Draw sprite frame animation indicator
        if (selectedAnimation == ANIM_SPRITE_FRAMES) {
            std::string frameInfo = "Frame: " + std::to_string(animations[ANIM_SPRITE_FRAMES].currentFrame + 1) + 
                                   "/" + std::to_string(animations[ANIM_SPRITE_FRAMES].frameCount);
            api->drawText(frameInfo, testX, testY + 30, WispColor(255, 255, 0), 6);
        }
        
        // Draw animation list
        for (int i = 0; i < ANIM_COUNT; i++) {
            WispColor textColor = (i == selectedAnimation) ? WispColor(255, 255, 0) : WispColor(180, 180, 180);
            std::string status = animations[i].playing ? " [PLAY]" : " [PAUSE]";
            std::string animText = getAnimationName(i) + status;
            
            api->drawText(animText, 10, 150 + i * 12, textColor, 7);
        }
        
        // Draw controls
        api->drawText("Up/Down: Select  A: Play/Pause  B: Reset", 10, 210, WispColor(200, 200, 200), 8);
        api->drawText("Left/Right: Speed", 10, 225, WispColor(200, 200, 200), 8);
        
        // Draw current animation info
        const AnimationTest& current = animations[selectedAnimation];
        std::string info = "Speed: " + std::to_string(current.speed) + 
                          "  Progress: " + std::to_string((int)(current.progress * 100)) + "%";
        api->drawText(info, 160, 30, WispColor(255, 255, 255), 8);
    }
    
    void cleanup() override {
        if (animatedSprite) {
            api->unloadSprite(animatedSprite);
        }
        api->print("Animation Test App cleaned up");
    }
};

// Export function for the engine
extern "C" WispAppBase* createAnimationTestApp() {
    return new AnimationTestApp();
}

extern "C" void destroyAnimationTestApp(WispAppBase* app) {
    delete app;
}
