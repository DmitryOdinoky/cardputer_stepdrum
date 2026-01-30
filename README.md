# Cardputer StepDrum

A 4x8 drum step sequencer for M5Stack Cardputer ADV, playing WAV samples from SD card.

## Features

- **4 instrument tracks x 8 steps** - Classic drum machine grid
- **WAV sample playback** - Load any WAV files from SD card
- **Sample switching per track** - Cycle through available samples for each track
- **Variable pattern length** - 1 to 8 steps
- **Adjustable BPM** - 60 to 240 BPM
- **Direct track triggering** - Play samples instantly with number keys

## Hardware

- **M5Stack Cardputer ADV** (ESP32-S3 based)
- **SD Card** with WAV samples

## Controls

| Key | Action |
|-----|--------|
| `;` or `w`/`e` | Cursor up |
| `.` or `s` | Cursor down |
| `,` or `a` | Cursor left |
| `/` or `d` | Cursor right |
| Space / Enter | Toggle step on/off |
| `p` | Play / Pause |
| `+` / `-` | BPM +5 / -5 |
| `[` / `]` | Pattern length -1 / +1 |
| `z` | Previous sample for selected track |
| `x` | Next sample for selected track |
| `1` `2` `3` `4` | Trigger tracks 1-4 directly |
| `c` | Clear pattern |

## SD Card Setup

Place WAV files in the root of the SD card:
- `/1.wav`, `/2.wav`, `/3.wav`, `/4.wav` - Loaded on startup for tracks 1-4
- Any additional `.wav` files can be selected using z/x keys

**WAV format:** 16-bit or 8-bit PCM, mono, 22050Hz recommended

## Building

Requires [PlatformIO](https://platformio.org/).

```bash
# Build
pio run

# Upload (hold G0 button while plugging USB for download mode)
pio run -t upload

# Serial monitor
pio device monitor --baud 115200
```

## Project Structure

```
├── platformio.ini      # PlatformIO configuration
└── src/
    ├── main.cpp        # Main loop, input handling, sample triggering
    ├── sequencer.h     # Pattern storage, playback state, cursor
    ├── audio.h         # WAV loading, SD card, sample playback
    ├── display.h       # Grid rendering with M5Canvas
    └── input.h         # Keyboard input handling
```

## Implementation Details

### Audio
- Samples loaded into PSRAM at startup
- Uses `M5Cardputer.Speaker.playRaw()` for playback
- 4 audio channels (one per track) for polyphonic playback
- ES8311 codec handled by M5Unified library

### Display
- 240x135 LCD with ST7789V2 controller
- Double-buffered rendering using M5Canvas
- 20Hz refresh rate (50ms)

### Sequencer
- Pattern stored as 4 bytes (1 bit per step, 8 steps per track)
- 16th note timing: `stepInterval = 15000 / BPM` ms

### SD Card Pins (Cardputer ADV)
- SCK: 40
- MISO: 39
- MOSI: 14
- CS: 12

## License

MIT
