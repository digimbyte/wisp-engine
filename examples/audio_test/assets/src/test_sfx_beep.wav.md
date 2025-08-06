# test_sfx_beep.wav - Source Sound Effect

## Purpose
Source WAV file for simple beep UI feedback sound.

## WAV Requirements
- **Filename**: `test_sfx_beep.wav`
- **Format**: WAV (uncompressed)
- **Sample Rate**: 44100 Hz (will be converted to 22050 Hz)
- **Channels**: Mono
- **Bit Depth**: 16-bit
- **Duration**: 0.2-0.5 seconds

## Sound Design
Simple, clear beep tone:
- **Frequency**: 800-1200 Hz (pleasant, audible range)
- **Waveform**: Sine or triangle wave for smooth tone
- **Envelope**: Quick attack (5ms), brief sustain, smooth decay (100ms)
- **Volume**: Moderate level for clear audibility
- **Character**: Clean, electronic, non-harsh

## Build Process
This WAV file will be converted to `test_sfx_beep.wsfx` using:
```
tools/wav_to_wisp.exe assets/src/test_sfx_beep.wav assets/test_sfx_beep.wsfx --format sfx
```

## Technical Guidelines
1. Keep attack very fast for immediate recognition
2. Avoid harmonics or overtones that might sound harsh
3. Test at various volumes to ensure pleasant at all levels
4. No background noise or artifacts
5. Normalize to appropriate level (around -6dB peak)

## Creation Tips
- Generate using tone generator or synthesizer
- Apply gentle low-pass filter to soften harsh edges
- Test by playing repeatedly to ensure it's not annoying
- Consider this will be triggered frequently during UI testing
- Keep it short to avoid overlapping with rapid button presses
