#ifndef DISPLAY_H
#define DISPLAY_H

#include <M5Cardputer.h>
#include "sequencer.h"

// Layout constants
constexpr int16_t GRID_ORIGIN_X = 50;  // More space for sample names
constexpr int16_t GRID_ORIGIN_Y = 35;
constexpr int16_t CELL_WIDTH = 22;
constexpr int16_t CELL_HEIGHT = 20;
constexpr int16_t CELL_PADDING = 2;

// Colors (RGB565)
constexpr uint16_t COLOR_BG = 0x0000;           // Black
constexpr uint16_t COLOR_GRID = 0x4208;         // Dark gray
constexpr uint16_t COLOR_ACTIVE = 0x07E0;       // Green
constexpr uint16_t COLOR_INACTIVE = 0x2104;     // Very dark gray
constexpr uint16_t COLOR_OUTSIDE = 0x1082;      // Even darker - outside pattern
constexpr uint16_t COLOR_CURSOR = 0xFFE0;       // Yellow
constexpr uint16_t COLOR_PLAYHEAD = 0xF800;     // Red
constexpr uint16_t COLOR_TEXT = 0xFFFF;         // White
constexpr uint16_t COLOR_TEXT_DIM = 0x8410;     // Gray
constexpr uint16_t COLOR_HIGHLIGHT = 0x001F;    // Blue for selected track

class DisplayManager {
public:
    M5Canvas canvas;
    String sampleNames[NUM_INSTRUMENTS] = {"1", "2", "3", "4"};

    void init() {
        canvas.setColorDepth(16);
        canvas.createSprite(240, 135);
        canvas.setTextDatum(MC_DATUM);
    }

    void setSampleName(uint8_t track, const String& name) {
        if (track < NUM_INSTRUMENTS) {
            sampleNames[track] = name;
        }
    }

    void drawAll(const Pattern& pattern, const Cursor& cursor,
                 const PlaybackState& playback) {
        // Clear
        canvas.fillSprite(COLOR_BG);

        // Title and status bar
        canvas.setTextColor(COLOR_TEXT);
        canvas.setTextSize(1);
        canvas.setTextDatum(ML_DATUM);
        canvas.drawString("SEQ", 2, 10);

        // BPM
        char bpmStr[16];
        sprintf(bpmStr, "%d", playback.bpm);
        canvas.setTextDatum(MC_DATUM);
        canvas.drawString("BPM", 95, 6);
        canvas.drawString(bpmStr, 95, 16);

        // Length
        char lenStr[8];
        sprintf(lenStr, "%d", playback.patternLength);
        canvas.drawString("LEN", 140, 6);
        canvas.drawString(lenStr, 140, 16);

        // Play/Stop status
        canvas.setTextDatum(MC_DATUM);
        if (playback.isPlaying) {
            canvas.setTextColor(COLOR_ACTIVE);
            canvas.fillRoundRect(170, 2, 40, 18, 3, 0x0300);
            canvas.drawString("PLAY", 190, 11);
        } else {
            canvas.setTextColor(COLOR_TEXT_DIM);
            canvas.drawString("STOP", 190, 11);
        }

        // Step numbers (only up to pattern length)
        canvas.setTextColor(COLOR_TEXT_DIM);
        canvas.setTextDatum(MC_DATUM);
        for (int col = 0; col < MAX_STEPS; col++) {
            int16_t x = GRID_ORIGIN_X + col * CELL_WIDTH + CELL_WIDTH / 2;
            if (col < playback.patternLength) {
                canvas.setTextColor(COLOR_TEXT_DIM);
            } else {
                canvas.setTextColor(0x2104);  // Very dim for outside steps
            }
            canvas.drawString(String(col + 1), x, 27);
        }

        // Grid rows
        for (int row = 0; row < NUM_INSTRUMENTS; row++) {
            int16_t y = GRID_ORIGIN_Y + row * CELL_HEIGHT + CELL_HEIGHT / 2;

            // Sample name (highlighted if cursor is on this row)
            canvas.setTextDatum(MR_DATUM);
            if (cursor.row == row) {
                canvas.setTextColor(COLOR_CURSOR);
            } else {
                canvas.setTextColor(COLOR_TEXT);
            }

            // Truncate name to fit
            String dispName = sampleNames[row];
            if (dispName.length() > 6) dispName = dispName.substring(0, 6);
            canvas.drawString(dispName, GRID_ORIGIN_X - 4, y);

            // Cells
            for (int col = 0; col < MAX_STEPS; col++) {
                bool active = pattern.getStep(row, col);
                bool isCursor = (cursor.row == row && cursor.col == col);
                bool isPlayhead = playback.isPlaying && (col == playback.currentStep);
                bool inPattern = (col < playback.patternLength);
                drawCell(row, col, active, isCursor, isPlayhead, inPattern);
            }
        }

        // Playhead indicator at bottom
        if (playback.isPlaying) {
            int16_t x = GRID_ORIGIN_X + playback.currentStep * CELL_WIDTH + CELL_WIDTH / 2;
            canvas.fillTriangle(x - 4, 128, x + 4, 128, x, 120, COLOR_PLAYHEAD);
        }

        // Help text at very bottom
        canvas.setTextDatum(MC_DATUM);
        canvas.setTextColor(0x4208);
        canvas.drawString("z/x:sample []:len p:play", 120, 133);

        // Push to display
        canvas.pushSprite(&M5Cardputer.Display, 0, 0);
    }

    void drawCell(uint8_t row, uint8_t col, bool active, bool isCursor,
                  bool isPlayhead, bool inPattern) {
        int16_t x = GRID_ORIGIN_X + col * CELL_WIDTH + CELL_PADDING;
        int16_t y = GRID_ORIGIN_Y + row * CELL_HEIGHT + CELL_PADDING;
        int16_t w = CELL_WIDTH - CELL_PADDING * 2;
        int16_t h = CELL_HEIGHT - CELL_PADDING * 2;

        // Determine fill color
        uint16_t fillColor;
        if (!inPattern) {
            fillColor = COLOR_OUTSIDE;  // Outside pattern length
        } else if (isPlayhead && active) {
            fillColor = 0x07FF;  // Cyan for active + playhead
        } else if (isPlayhead) {
            fillColor = 0x4010;  // Dim green for playhead
        } else if (active) {
            fillColor = COLOR_ACTIVE;
        } else {
            fillColor = COLOR_INACTIVE;
        }

        canvas.fillRoundRect(x, y, w, h, 2, fillColor);

        // Cursor border
        if (isCursor) {
            canvas.drawRoundRect(x - 1, y - 1, w + 2, h + 2, 3, COLOR_CURSOR);
        }
    }
};

#endif
