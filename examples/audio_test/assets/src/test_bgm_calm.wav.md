# test_bgm_calm.wav - Source Background Music

## Purpose
Source WAV file for calm background music testing.

## WAV Requirements
- **Filename**: `test_bgm_calm.wav`
- **Format**: WAV (uncompressed)
- **Sample Rate**: 44100 Hz (will be converted to 22050 Hz)
- **Channels**: Stereo
- **Bit Depth**: 16-bit
- **Duration**: 60-90 seconds

## Musical Content
Calm, ambient background track:
- **Tempo**: Slow (60-80 BPM)
- **Key**: Major key for positive feeling
- **Instruments**: Soft synthesizers, light percussion
- **Dynamics**: Consistent volume, minimal sudden changes
- **Mood**: Peaceful, contemplative, non-intrusive

## Loop Requirements
- **CRITICAL**: Must loop seamlessly without gap or click
- Design with loop point in mind
- Fade in/out at edges for smooth transition
- Test loop point before final export

## Build Process
This WAV file will be converted to `test_bgm_calm.wbgm` using:
```
tools/wav_to_wisp.exe assets/src/test_bgm_calm.wav assets/test_bgm_calm.wbgm --format bgm --loop
```

## Technical Guidelines
1. Keep volume consistent throughout
2. Avoid sudden volume changes or harsh attacks
3. Test at low volume to ensure it doesn't overpower SFX
4. Compress dynamics lightly for consistent playback
5. High-pass filter below 60Hz to reduce low-end buildup

## Creation Tips
- Use royalty-free samples or original composition
- Test loop point by playing the end and beginning together
- Keep musical complexity moderate to avoid listener fatigue
- Consider the track will play repeatedly during testing
