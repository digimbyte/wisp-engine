# Audio Test Application

## Purpose
Comprehensive testing of the Wisp Engine audio system including BGM playback, SFX mixing, cry synthesis, and multi-channel audio.

## Files
- `audio_test_app.cpp` - Main test application
- `assets/` - Audio assets and documentation

## Asset Requirements
The application expects the following audio files in the `assets/` folder:

### Background Music (.wbgm)
- `test_bgm_calm.wbgm` - Peaceful background track
- `test_bgm_action.wbgm` - Energetic action music
- `test_bgm_ambient.wbgm` - Atmospheric ambient sounds
- `test_bgm_battle.wbgm` - Intense battle music

### Sound Effects (.wsfx)
- `test_sfx_beep.wsfx` - UI feedback sound
- `test_sfx_explosion.wsfx` - Impact sound effect
- `test_sfx_pickup.wsfx` - Item collection sound
- `test_sfx_jump.wsfx` - Character action sound
- `test_sfx_hit.wsfx` - Combat sound
- `test_sfx_powerup.wsfx` - Enhancement sound
- `test_sfx_menu.wsfx` - Menu navigation sound
- `test_sfx_error.wsfx` - Error notification sound

### Creature Cries (.wcry)
- `test_cry_pikachu.wcry` - Pikachu vocalization
- `test_cry_charizard.wcry` - Charizard vocalization
- `test_cry_blastoise.wcry` - Blastoise vocalization
- `test_cry_venusaur.wcry` - Venusaur vocalization
- `test_cry_mewtwo.wcry` - Mewtwo vocalization
- `test_cry_mew.wcry` - Mew vocalization

## Controls
- **Up/Down**: Switch audio test modes (BGM, SFX, Cry, Mixing)
- **A Button**: Play/Stop current audio
- **B Button**: Next track/auto-toggle
- **Left/Right**: Volume control
- **Start**: Auto SFX toggle (for SFX mode)

## Test Modes

### 1. BGM Test
- Tests background music playback and looping
- Volume control and track switching
- Loop point accuracy

### 2. SFX Test
- Individual sound effect playback
- Auto-play mode for stress testing
- Multiple simultaneous SFX

### 3. Cry Test
- Creature cry synthesis and playback
- Procedural audio generation testing

### 4. Mixing Test
- Multi-channel audio mixing
- Performance with simultaneous audio streams
- Channel management

## Features Tested
- ✅ Audio file loading (BGM, SFX, Cry formats)
- ✅ Background music playback and looping
- ✅ Sound effect triggering and mixing
- ✅ Volume control (master and per-type)
- ✅ Multi-channel audio management
- ✅ Audio visualization (level meters)
- ✅ Cry synthesis system
- ✅ Auto-play and timing systems

## Asset Creation Guide
See the `.md` files in the `assets/` folder for detailed specifications of what each audio file should contain, including format requirements, duration, and content guidelines.
