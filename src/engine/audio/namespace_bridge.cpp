// engine/audio/namespace_bridge.cpp - Audio namespace implementation  
#include "../../wisp_engine.h"
#include "engine.h" // Local audio engine

namespace WispEngine {
namespace Audio {

// Audio Engine bridge
class Engine {
private:
    static AudioEngine* instance;
    
public:
    static bool initialize(uint8_t outputs = AUDIO_ALL, uint32_t sampleRate = AUDIO_SAMPLE_RATE) {
        if (!instance) {
            instance = new AudioEngine();
            instance->init(outputs, sampleRate);
            return true;
        }
        return true;
    }
    
    static void shutdown() {
        if (instance) {
            instance->cleanup();
            delete instance;
            instance = nullptr;
        }
    }
    
    static AudioEngine* getInstance() {
        return instance;
    }
    
    static bool playTone(uint16_t frequency, uint16_t duration, uint8_t volume = 128) {
        if (instance) {
            return instance->playTone(frequency, duration, volume);
        }
        return false;
    }
    
    static void update() {
        if (instance) {
            instance->update();
        }
    }
    
    static bool isInitialized() {
        return instance != nullptr;
    }
};

// Static member definition
AudioEngine* Engine::instance = nullptr;

} // namespace Audio
} // namespace WispEngine
