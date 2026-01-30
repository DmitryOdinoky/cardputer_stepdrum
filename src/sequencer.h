#ifndef SEQUENCER_H
#define SEQUENCER_H

#include <cstdint>

// Constants
constexpr uint8_t NUM_INSTRUMENTS = 4;
constexpr uint8_t MAX_STEPS = 8;
constexpr uint8_t MIN_STEPS = 1;
constexpr uint16_t DEFAULT_BPM = 120;
constexpr uint16_t MIN_BPM = 60;
constexpr uint16_t MAX_BPM = 240;

// Pattern data: each byte holds 8 steps for one instrument
struct Pattern {
    uint8_t steps[NUM_INSTRUMENTS];  // Each bit = one step (0-7)

    bool getStep(uint8_t instrument, uint8_t step) const {
        return (steps[instrument] >> step) & 0x01;
    }

    void setStep(uint8_t instrument, uint8_t step, bool value) {
        if (value) {
            steps[instrument] |= (1 << step);
        } else {
            steps[instrument] &= ~(1 << step);
        }
    }

    void toggleStep(uint8_t instrument, uint8_t step) {
        steps[instrument] ^= (1 << step);
    }

    void clear() {
        for (int i = 0; i < NUM_INSTRUMENTS; i++) {
            steps[i] = 0;
        }
    }
};

// Playback state
struct PlaybackState {
    bool isPlaying = false;
    uint8_t currentStep = 0;
    uint8_t patternLength = MAX_STEPS;  // 1-8
    uint16_t bpm = DEFAULT_BPM;
    uint32_t lastStepTimeMs = 0;
    uint32_t stepIntervalMs = 125;

    void updateInterval() {
        // 16th notes: 60000 / (bpm * 4) = 15000 / bpm
        stepIntervalMs = 15000 / bpm;
    }
};

// Cursor for editing
struct Cursor {
    uint8_t row = 0;  // 0-3 (instrument)
    uint8_t col = 0;  // 0-7 (step)

    void moveUp()    { if (row > 0) row--; }
    void moveDown()  { if (row < NUM_INSTRUMENTS - 1) row++; }
    void moveLeft()  { if (col > 0) col--; }
    void moveRight(uint8_t maxCol) { if (col < maxCol - 1) col++; }

    void clampToLength(uint8_t length) {
        if (col >= length) col = length - 1;
    }
};

// Main sequencer class
class Sequencer {
public:
    Pattern pattern;
    PlaybackState playback;
    Cursor cursor;
    uint8_t trackSamples[NUM_INSTRUMENTS] = {0, 1, 2, 3};  // Which sample each track uses

    void init() {
        pattern.clear();
        playback.isPlaying = false;
        playback.currentStep = 0;
        playback.patternLength = MAX_STEPS;
        playback.bpm = DEFAULT_BPM;
        playback.updateInterval();
        cursor.row = 0;
        cursor.col = 0;
        // Default sample assignment
        for (int i = 0; i < NUM_INSTRUMENTS; i++) {
            trackSamples[i] = i;
        }
    }

    // Returns true if step changed
    bool update(uint32_t currentTimeMs) {
        if (!playback.isPlaying) return false;

        if (currentTimeMs - playback.lastStepTimeMs >= playback.stepIntervalMs) {
            playback.lastStepTimeMs = currentTimeMs;
            playback.currentStep = (playback.currentStep + 1) % playback.patternLength;
            return true;
        }
        return false;
    }

    void togglePlay() {
        playback.isPlaying = !playback.isPlaying;
        if (playback.isPlaying) {
            playback.currentStep = 0;  // Reset to start
            playback.lastStepTimeMs = millis();
        }
    }

    void stop() {
        playback.isPlaying = false;
        playback.currentStep = 0;
    }

    void setBPM(uint16_t newBpm) {
        if (newBpm < MIN_BPM) newBpm = MIN_BPM;
        if (newBpm > MAX_BPM) newBpm = MAX_BPM;
        playback.bpm = newBpm;
        playback.updateInterval();
    }

    void adjustBPM(int16_t delta) {
        setBPM(playback.bpm + delta);
    }

    void setPatternLength(uint8_t length) {
        if (length < MIN_STEPS) length = MIN_STEPS;
        if (length > MAX_STEPS) length = MAX_STEPS;
        playback.patternLength = length;
        // Clamp cursor and playhead
        cursor.clampToLength(length);
        if (playback.currentStep >= length) {
            playback.currentStep = 0;
        }
    }

    void adjustPatternLength(int8_t delta) {
        setPatternLength(playback.patternLength + delta);
    }

    void setTrackSample(uint8_t track, uint8_t sampleIndex) {
        if (track < NUM_INSTRUMENTS) {
            trackSamples[track] = sampleIndex;
        }
    }

    uint8_t getTrackSample(uint8_t track) const {
        if (track < NUM_INSTRUMENTS) {
            return trackSamples[track];
        }
        return 0;
    }
};

#endif
