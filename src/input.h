#ifndef INPUT_H
#define INPUT_H

#include <M5Cardputer.h>

enum class InputEvent {
    None,
    Up,
    Down,
    Left,
    Right,
    Toggle,
    PlayPause,
    BPMUp,
    BPMDown,
    Clear,
    LengthUp,
    LengthDown,
    SampleNext,
    SamplePrev,
    TriggerTrack1,
    TriggerTrack2,
    TriggerTrack3,
    TriggerTrack4
};

class InputHandler {
public:
    InputEvent poll() {
        if (!M5Cardputer.Keyboard.isChange()) {
            return InputEvent::None;
        }

        if (!M5Cardputer.Keyboard.isPressed()) {
            return InputEvent::None;
        }

        // Rate limiting
        uint32_t now = millis();
        if (now - lastKeyTime < KEY_REPEAT_DELAY_MS) {
            return InputEvent::None;
        }
        lastKeyTime = now;

        // Arrow keys on Cardputer: ; = up, . = down, , = left, / = right
        // Also keep WASD/ESAD as alternatives
        if (M5Cardputer.Keyboard.isKeyPressed(';') ||
            M5Cardputer.Keyboard.isKeyPressed('e') ||
            M5Cardputer.Keyboard.isKeyPressed('w'))
            return InputEvent::Up;

        if (M5Cardputer.Keyboard.isKeyPressed('.') ||
            M5Cardputer.Keyboard.isKeyPressed('s'))
            return InputEvent::Down;

        if (M5Cardputer.Keyboard.isKeyPressed(',') ||
            M5Cardputer.Keyboard.isKeyPressed('a'))
            return InputEvent::Left;

        if (M5Cardputer.Keyboard.isKeyPressed('/') ||
            M5Cardputer.Keyboard.isKeyPressed('d'))
            return InputEvent::Right;

        // Toggle with space
        if (M5Cardputer.Keyboard.isKeyPressed(' '))
            return InputEvent::Toggle;

        // Check Enter via keysState
        Keyboard_Class::KeysState state = M5Cardputer.Keyboard.keysState();
        if (state.enter)
            return InputEvent::Toggle;

        // Play/Pause
        if (M5Cardputer.Keyboard.isKeyPressed('p'))
            return InputEvent::PlayPause;

        // BPM control: + / -
        if (M5Cardputer.Keyboard.isKeyPressed('+') || M5Cardputer.Keyboard.isKeyPressed('='))
            return InputEvent::BPMUp;
        if (M5Cardputer.Keyboard.isKeyPressed('-') || M5Cardputer.Keyboard.isKeyPressed('_'))
            return InputEvent::BPMDown;

        // Pattern length: [ / ]
        if (M5Cardputer.Keyboard.isKeyPressed('['))
            return InputEvent::LengthDown;
        if (M5Cardputer.Keyboard.isKeyPressed(']'))
            return InputEvent::LengthUp;

        // Sample selection for current track: z/x
        if (M5Cardputer.Keyboard.isKeyPressed('z'))
            return InputEvent::SamplePrev;
        if (M5Cardputer.Keyboard.isKeyPressed('x'))
            return InputEvent::SampleNext;

        // Trigger tracks 1-4 directly
        if (M5Cardputer.Keyboard.isKeyPressed('1'))
            return InputEvent::TriggerTrack1;
        if (M5Cardputer.Keyboard.isKeyPressed('2'))
            return InputEvent::TriggerTrack2;
        if (M5Cardputer.Keyboard.isKeyPressed('3'))
            return InputEvent::TriggerTrack3;
        if (M5Cardputer.Keyboard.isKeyPressed('4'))
            return InputEvent::TriggerTrack4;

        // Clear
        if (M5Cardputer.Keyboard.isKeyPressed('c'))
            return InputEvent::Clear;

        return InputEvent::None;
    }

private:
    uint32_t lastKeyTime = 0;
    static constexpr uint32_t KEY_REPEAT_DELAY_MS = 150;
};

#endif
