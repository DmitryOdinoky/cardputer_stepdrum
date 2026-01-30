#include <M5Cardputer.h>
#include "sequencer.h"
#include "audio.h"
#include "display.h"
#include "input.h"

// Global objects
Sequencer sequencer;
AudioManager audio;
DisplayManager display;
InputHandler input;

// Timing
constexpr uint32_t DISPLAY_UPDATE_MS = 50;  // 20 Hz
uint32_t lastDisplayUpdate = 0;
bool needsRedraw = true;

void handleInput(InputEvent event);
void updateDisplaySampleNames();
void cycleTrackSample(uint8_t track, int8_t direction);

void setup() {
    Serial.begin(115200);
    Serial.println("Drum Sequencer starting...");

    // Initialize M5Cardputer with speaker enabled
    auto cfg = M5.config();
    cfg.internal_spk = true;  // Enable internal speaker
    M5Cardputer.begin(cfg, true);

    // Explicitly enable and configure speaker
    M5Cardputer.Speaker.begin();
    M5Cardputer.Speaker.setVolume(255);  // Max volume

    // Display setup
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setBrightness(80);
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(TFT_WHITE);
    M5Cardputer.Display.setCursor(10, 10);
    M5Cardputer.Display.println("Initializing...");

    // Initialize audio/SD
    if (!audio.init()) {
        M5Cardputer.Display.setTextColor(TFT_RED);
        M5Cardputer.Display.println("SD Card Failed!");
        M5Cardputer.Display.println("Insert SD with WAV files");
        while (1) {
            M5Cardputer.update();
            delay(100);
        }
    }

    M5Cardputer.Display.printf("Found %d WAV files\n", audio.getWavFileCount());

    if (audio.getWavFileCount() == 0) {
        M5Cardputer.Display.setTextColor(TFT_RED);
        M5Cardputer.Display.println("No WAV files found!");
        M5Cardputer.Display.println("Add .wav files to SD root");
        while (1) {
            M5Cardputer.update();
            delay(100);
        }
    }

    // Load samples using simple hardcoded paths (like original working version)
    M5Cardputer.Display.println("Loading samples...");
    for (int i = 0; i < 4; i++) {
        char filename[16];
        sprintf(filename, "/%d.wav", i + 1);
        M5Cardputer.Display.printf("%s...", filename);
        if (audio.loadSample(i, filename)) {
            M5Cardputer.Display.println("OK");
        } else {
            M5Cardputer.Display.println("FAIL");
        }
    }

    // Scan for additional WAV files for sample switching
    audio.scanWavFiles();
    M5Cardputer.Display.printf("Total WAVs: %d\n", audio.getWavFileCount());
    delay(500);

    delay(500);

    // Test speaker
    M5Cardputer.Display.println("Speaker test...");
    M5Cardputer.Speaker.tone(1000, 200);
    delay(300);

    // Initialize components
    sequencer.init();
    display.init();

    // Set initial track sample assignments (samples 0-3 are loaded from /1.wav-/4.wav)
    for (int i = 0; i < NUM_INSTRUMENTS; i++) {
        sequencer.trackSamples[i] = i;  // Track i uses sample i
        // Set display name from the loaded sample
        if (audio.samples[i].loaded) {
            display.setSampleName(i, audio.samples[i].name);
        }
    }

    // Set a default pattern
    sequencer.pattern.setStep(0, 0, true);
    sequencer.pattern.setStep(0, 4, true);
    sequencer.pattern.setStep(1, 2, true);
    sequencer.pattern.setStep(1, 6, true);
    for (int i = 0; i < 8; i++) {
        sequencer.pattern.setStep(2, i, true);
    }

    Serial.println("Setup complete!");
    needsRedraw = true;
}

void loop() {
    uint32_t now = millis();

    // Always update keyboard state first
    M5Cardputer.update();

    // Handle input
    InputEvent event = input.poll();
    if (event != InputEvent::None) {
        handleInput(event);
        needsRedraw = true;
    }

    // Update sequencer
    if (sequencer.playback.isPlaying) {
        bool stepChanged = sequencer.update(now);

        if (stepChanged) {
            needsRedraw = true;

            // Trigger samples for current step
            uint8_t step = sequencer.playback.currentStep;
            for (uint8_t inst = 0; inst < NUM_INSTRUMENTS; inst++) {
                if (sequencer.pattern.getStep(inst, step)) {
                    audio.playSample(inst, inst);  // Track N uses sample slot N
                }
            }
        }
    }

    // Update display
    if (needsRedraw && (now - lastDisplayUpdate >= DISPLAY_UPDATE_MS)) {
        lastDisplayUpdate = now;
        needsRedraw = false;
        display.drawAll(sequencer.pattern, sequencer.cursor, sequencer.playback);
    }
}

void handleInput(InputEvent event) {
    switch (event) {
        case InputEvent::Up:
            sequencer.cursor.moveUp();
            break;

        case InputEvent::Down:
            sequencer.cursor.moveDown();
            break;

        case InputEvent::Left:
            sequencer.cursor.moveLeft();
            break;

        case InputEvent::Right:
            sequencer.cursor.moveRight(sequencer.playback.patternLength);
            break;

        case InputEvent::Toggle:
            sequencer.pattern.toggleStep(sequencer.cursor.row, sequencer.cursor.col);
            break;

        case InputEvent::PlayPause:
            sequencer.togglePlay();
            if (!sequencer.playback.isPlaying) {
                audio.stopAll();
            }
            break;

        case InputEvent::BPMUp:
            sequencer.adjustBPM(5);
            break;

        case InputEvent::BPMDown:
            sequencer.adjustBPM(-5);
            break;

        case InputEvent::LengthUp:
            sequencer.adjustPatternLength(1);
            break;

        case InputEvent::LengthDown:
            sequencer.adjustPatternLength(-1);
            break;

        case InputEvent::SampleNext:
            Serial.println("Event: SampleNext (x key)");
            cycleTrackSample(sequencer.cursor.row, 1);
            break;

        case InputEvent::SamplePrev:
            Serial.println("Event: SamplePrev (z key)");
            cycleTrackSample(sequencer.cursor.row, -1);
            break;

        case InputEvent::Clear:
            sequencer.pattern.clear();
            break;

        case InputEvent::TriggerTrack1:
            Serial.println("Event: TriggerTrack1 (1 key)");
            audio.playSample(0, 0);
            break;

        case InputEvent::TriggerTrack2:
            Serial.println("Event: TriggerTrack2 (2 key)");
            audio.playSample(1, 1);
            break;

        case InputEvent::TriggerTrack3:
            Serial.println("Event: TriggerTrack3 (3 key)");
            audio.playSample(2, 2);
            break;

        case InputEvent::TriggerTrack4:
            Serial.println("Event: TriggerTrack4 (4 key)");
            audio.playSample(3, 3);
            break;

        default:
            break;
    }
}

// Track which wavFile index each track is using
int trackWavIndex[NUM_INSTRUMENTS] = {0, 1, 2, 3};

void cycleTrackSample(uint8_t track, int8_t direction) {
    Serial.printf("cycleTrackSample: track=%d dir=%d wavCount=%d\n",
                  track, direction, audio.getWavFileCount());

    if (audio.getWavFileCount() == 0) {
        Serial.println("No WAV files found for cycling!");
        return;
    }

    // Cycle through wavFiles for this track
    int newWavIdx = trackWavIndex[track] + direction;

    // Wrap around
    if (newWavIdx < 0) {
        newWavIdx = audio.getWavFileCount() - 1;
    } else if (newWavIdx >= audio.getWavFileCount()) {
        newWavIdx = 0;
    }

    trackWavIndex[track] = newWavIdx;

    // Load the new sample into this track's slot
    const char* filename = audio.getWavFileName(newWavIdx);
    audio.loadSample(track, filename);  // Load into slot 'track'

    // Update display name
    display.setSampleName(track, audio.samples[track].name);

    // Play preview
    audio.playSample(track, track);
}

void updateDisplaySampleNames() {
    for (int i = 0; i < NUM_INSTRUMENTS; i++) {
        uint8_t sampleIdx = sequencer.trackSamples[i];
        display.setSampleName(i, audio.getShortName(sampleIdx));
    }
}
