// engine/audio/namespace_bridge.cpp - Audio namespace implementation  
#include "../engine_common.h"
#include "audio_engine.h"

namespace WispEngine {
namespace Audio {

// Audio Engine bridge - delegates to WispEngine::Audio functions
class Engine {
public:
    static bool initialize() {
        init();
        return true;
    }
    
    static void shutdown() {
        WispEngine::Audio::shutdown();
    }
    
    static void update() {
        WispEngine::Audio::update();
    }
    
    static bool playBGM(const WBGMHeader* header, const uint8_t* data, uint8_t volume = 255) {
        WispEngine::Audio::playBGM(header, data, volume);
        return true;
    }
    
    static void stopBGM() {
        WispEngine::Audio::stopBGM();
    }
    
    static bool playSFX(const WSFXHeader* header, const uint8_t* data) {
        WispEngine::Audio::playSFX(header, data);
        return true;
    }
    
    static bool playCry(const WCrySequenceData* sequence) {
        WispEngine::Audio::playCry(sequence);
        return true;
    }
    
    static void setMasterVolume(uint8_t volume) {
        WispEngine::Audio::setMasterVolume(volume);
    }
    
    static bool isInitialized() {
        return true; // Simple check
    }
};

} // namespace Audio
} // namespace WispEngine
