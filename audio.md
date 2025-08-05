🔊 AUDIO ENGINE: FULL FEATURE SET (TECHNICAL)
🔁 Runtime Architecture
Audio runs independently from the game loop

Powered by a ringbuffer feeding into an I2S or Bluetooth A2DP backend

Engine runs at a fixed sample rate (e.g., 16,000 Hz)

Game logic runs at 24–30 FPS and feeds timing-sensitive data to audio

ROMs interface with the audio system via a dedicated AudioAPI

🎛 AUDIO LAYER OVERVIEW
1. BGM (Background Music)
Can be:

PCM streams (looped or one-shot)

Tracker modules (MOD/XM)

ADPCM-encoded longer tracks

Supports:

Play / pause / resume

Looping

Soft transitions (with fade out + replacement)

Preloading or streaming from flash or SD

Can be interrupted by cries

Automatically resumes after cries if paused

2. SFX (Sound Effects)
Short bursts of sound (UI clicks, hits, menu FX)

Stored as raw PCM or ADPCM

Played via AudioAPI::playSFX(...)

Mixed into the BGM, not interrupting it

Multiple can play simultaneously (queued and mixed)

Auto-cleaned when complete

3. Cries (Procedural Cry Synthesizer)
Defined via .wcryseq: parameter timeline per channel

Synthesized at runtime by CrySynthNode

Cry playback:

Interrupts and pauses BGM

Blocks all other cries until complete

Each cry:

4-channel procedural synth

Each channel has its own waveform, envelope, and automation

Engine uses fixed-length (e.g. 3s max), fading in/out automatically

Fully real-time, low-memory

🎧 MIXING BEHAVIOR
Source	Mixed With BGM?	Interrupts BGM?	Mixing Style
BGM	N/A	❌	Base audio stream
SFX	✅	❌	Additive overlay
Cries	❌	✅	Exclusive focus

Soft clipper or limiter protects against overflow

Mixer handles buffer-per-frame (e.g. 128–256 samples per tick)

📡 AUDIOAPI INTERFACE (ROM-FACING)
The ROM calls into the audio engine through a unified API, e.g.:
```
AudioAPI::playBGM(source);
AudioAPI::transitionBGM(newSource);
AudioAPI::playSFX(sfx);
AudioAPI::playCry(cry);
AudioAPI::stopBGM();
AudioAPI::tick(); // Main loop call
AudioAPI::renderAudio(buffer, samples); // Called by ISR or audio driver
```
🧠 TIMING MODEL
Audio sample rate: 16kHz recommended

Engine mixes every 8–16ms

Game loop only updates cry/sfx triggers + handles playback state

💾 STORAGE FORMATS & MEMORY
PCM / ADPCM / XM: used for BGM and SFX

.wcryseq: used for procedural cries (no waveform data, only automation curves)

Audio buffers are ring-based (e.g., 2–4KB)

Each AudioSource type (e.g., CrySynthNode, SFXPlayer) manages its own memory and stream position

⚙️ FUTURE EXTENSIONS (SUPPORTED BY DESIGN)
Real-time EQ or filters (per-channel or master)

FM or pulse-width synthesis

Dynamic cry parameter generation (not just baked)

Stereo panning (currently mono by default)

✅ ENGINE STRENGTHS
Lightweight, modular, deterministic

Prioritized audio layer behavior

Full ROM integration for game-triggered playback

Designed for low RAM / no PSRAM

Can stream audio as needed, or generate it procedurally



Code examples for audio.md
🔧 AudioMixer.cpp
```
#include "esp_audio_engine.h"
#include <string.h> // for memset
#include <algorithm>

void AudioMixer::mix(int16_t* buffer, size_t samples) {
    memset(buffer, 0, samples * sizeof(int16_t));

    if (cry) {
        cry->render(buffer, samples);
        if (cry->isFinished()) {
            cry = nullptr;
            if (bgmPaused && bgm) bgm->resume();
            bgmPaused = false;
        }
    } else if (bgm && !bgmPaused) {
        bgm->render(buffer, samples);
    }

    for (auto it = sfxQueue.begin(); it != sfxQueue.end(); ) {
        AudioSource* sfx = *it;
        int16_t tempBuf[AUDIO_BUFFER_SIZE] = {0};
        sfx->render(tempBuf, samples);

        // Mix
        for (size_t i = 0; i < samples; ++i) {
            int32_t mixed = buffer[i] + tempBuf[i];
            buffer[i] = std::clamp(mixed, -32768, 32767);
        }

        if (sfx->isFinished()) {
            it = sfxQueue.erase(it);
        } else {
            ++it;
        }
    }
}

void AudioMixer::update() {
    // Handle cry priority
    if (cry && bgm && !bgmPaused) {
        bgm->pause();
        bgmPaused = true;
    }

    // Handle BGM transition
    if (pendingBGM && !cry) {
        if (fadeCounter > 0 && fadingOutBGM) {
            fadeCounter--;
            // Could scale volume here during fade (TODO)
        } else {
            if (bgm) bgm->pause(); // soft stop
            bgm = pendingBGM;
            bgm->reset();
            bgm->resume();
            pendingBGM = nullptr;
            fadingOutBGM = false;
            fadeCounter = 0;
        }
    }
}

void AudioMixer::setBGM(AudioSource* source) {
    bgm = source;
    if (bgm) {
        bgm->reset();
        bgm->resume();
    }
}

void AudioMixer::transitionToBGM(AudioSource* newBGM, bool fadeOut) {
    pendingBGM = newBGM;
    fadingOutBGM = fadeOut;
    fadeCounter = fadeOut ? 16 : 0;
}

void AudioMixer::setCry(AudioSource* source) {
    if (cry) cry->pause(); // optional
    cry = source;
    if (cry) cry->reset();
}

void AudioMixer::addSFX(AudioSource* sfx) {
    sfx->reset();
    sfxQueue.push_back(sfx);
}
```
🧩 AudioAPI.cpp
```
#include "esp_audio_engine.h"

static AudioMixer mixer;

namespace AudioAPI {
    void playBGM(AudioSource* track) {
        mixer.setBGM(track);
    }

    void transitionBGM(AudioSource* track) {
        mixer.transitionToBGM(track);
    }

    void stopBGM() {
        mixer.setBGM(nullptr);
    }

    void playSFX(AudioSource* sfx) {
        mixer.addSFX(sfx);
    }

    void playCry(AudioSource* cry) {
        mixer.setCry(cry);
    }

    void tick() {
        mixer.update();
    }

    void renderAudio(int16_t* out, size_t count) {
        mixer.mix(out, count);
    }
}
🔌 Example SFXPlayer (PCM/ADPCM stub)
```
class SFXPlayer : public AudioSource {
public:
    SFXPlayer(const int16_t* data, size_t len) : samples(data), total(len), index(0) {}

    void render(int16_t* buffer, size_t count) override {
        for (size_t i = 0; i < count; ++i) {
            if (index < total) buffer[i] = samples[index++];
            else buffer[i] = 0;
        }
    }

    bool isFinished() const override {
        return index >= total;
    }

    void reset() override {
        index = 0;
    }

private:
    const int16_t* samples;
    size_t total;
    size_t index;
};
```
📀 BGMLooper (Looping PCM or tracker file)
```
class BGMLooper : public AudioSource {
public:
    BGMLooper(const int16_t* data, size_t len) : samples(data), total(len), index(0), paused(false) {}

    void render(int16_t* buffer, size_t count) override {
        if (paused) {
            memset(buffer, 0, count * sizeof(int16_t));
            return;
        }
        for (size_t i = 0; i < count; ++i) {
            buffer[i] = samples[index++];
            if (index >= total) index = 0; // loop
        }
    }

    bool isFinished() const override {
        return false;
    }

    void reset() override {
        index = 0;
    }

    void pause() override {
        paused = true;
    }

    void resume() override {
        paused = false;
    }

private:
    const int16_t* samples;
    size_t total;
    size_t index;
    bool paused;
};
```
These components are lightweight, modular, and designed for your ROM-driven model. Next suggestions:

Build a CrySynthNode that renders .wcryseq on the fly.

🔁 Audio Playback Priority Rules
🎵 Background Music (BGM)
Runs continuously unless a cry is playing

When a cry starts:

Pause BGM

Resume BGM after cry completes

Optional: fade out/fade in to avoid audio pops

🔊 Sound Effects (SFX)
Must be mixed concurrently with BGM

Should not interrupt or override BGM

Mixed into final output using additive blending

🔔 Cries
Interrupts BGM and SFX
exclusive focus until complete
is synthesized in real-time
uses a fixed-length buffer (e.g., 3 seconds max)



How Cry SynthNode Works
🧬 What Is a “Cry” in This Engine?
A cry is a short, expressive audio event representing a creature, state, or behavior — generated procedurally using a fixed synthesizer layout.

The cry is not stored as waveform data

It is not compressed audio like ADPCM or PCM

Instead, it is a compact control stream that drives a hardcoded synthesizer

Conceptually, this works like a simplified MIDI or DAW automation track

You store "what the sound does", not the sound itself.

🎛 Architecture Overview
1. Cry Has 4 Hardcoded Synth Channels
Each channel has:

A fixed waveform behavior (predefined in engine)

A defined effect style, hardwired into the engine per channel

Example:

Channel 0 = Sine wave with pitch slide

Channel 1 = Square wave with vibrato

Channel 2 = White noise with tremble

Channel 3 = Sine + bass sweep

The cry system does not encode effects or wave types in .wcryseq. Instead, the cry just tells the engine:

“At step 10, increase pitch of channel 2 to 200.”

2. .wcryseq = MIDI-like Meta Format
The .wcryseq file contains only:

When each parameter changes

What value to apply

How many steps the cry plays for

This control stream is parsed and rendered in real time by a class like CrySynthNode.

📄 .wcryseq Format Structure
Fixed-size file format — ultra-compact.

c
Copy
Edit
struct CryChannelTrack {
    uint8_t pitch[64];     // 64 steps = ~3s
    uint8_t speed[64];
    uint8_t bass[64];
    uint8_t volume[64];
};

struct CrySequenceData {
    uint8_t stepCount;     // e.g. 64
    uint8_t sampleRateDiv; // e.g. 2 = 22050, 3 = 16000
    CryChannelTrack channels[4]; // Fixed synth layout
};
Each parameter is 0–255, mapped internally to freq, amp, etc.

This data is parsed once at cry start, then stepped through each tick

Cry playback is fully deterministic

🔊 Playback Process (ESP-Side)
When a cry is triggered:
AudioAPI::playCry() is called

BGM is paused

A new CrySynthNode is created

CrySynthNode steps through .wcryseq data frame by frame

Each channel:

Interpolates its pitch, volume, speed, bass for current step

Applies its fixed waveform and modulation logic

Produces a sample

Output is summed into the audio ringbuffer

After final step, cry ends and BGM resumes

🎧 Example Channel Logic (Hardcoded, Not From File)
Channel	Waveform	FX Style	Notes
Ch 0	Sine	Pitch slide	Main tone curve
Ch 1	Square	Vibrato	Robotic pulse
Ch 2	WhiteNoise	Tremble / burst	Guttural / texture
Ch 3	Sine	Bass swell	Reinforcement

These behaviors are coded in the engine, not in .wcryseq.

✅ Key Benefits of This Model
Feature	Result
No waveform data	Saves 10–50KB per cry
Fixed synth layout	Minimal RAM, no parsing logic
MIDI-style meta	Tiny files (under 200B typical)
Fade in/out baked	No abrupt cuts, no glitch pops
ROM-friendly	Easy to preload or stream
Deterministic	No buffering errors or drift

📦 Storage Cost Example
For 64 steps × 4 channels × 4 params =
1024 bytes per cry (worst case, uncompressed)

Can be compressed further using RLE or delta encoding

Real-world cries often < 300–400 bytes

🎮 Unity Side
Unity simulates this exact behavior:

Hardcoded channel synth

.wcryseq editor generates parameter values only

Preview lets user test combinations in real time

Exported .wcryseq = drop-in ROM file for ESP playback

✅ TL;DR
Cries are not audio clips.
Cries are compact control tracks that animate 4 fixed synth channels over time.
ESP32 renders them in real time, using less RAM than even short ADPCM clips.