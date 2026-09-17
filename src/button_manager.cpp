#include "button_manager.h"

ButtonManager::ButtonManager()
    : _state(ButtonState::IDLE)
    , _stateTimerMs(0)
    , _pressStartMs(0)
    , _windowStartMs(0)
{
}

bool ButtonManager::isPhysicalPressed() const {
    // GPIO 0 BOOT button has external/internal pull-up; active LOW when pressed
    return (digitalRead(BUTTON_PIN) == LOW);
}

void ButtonManager::init() {
    DEBUG_PRINTLN("[SLIDER] Initializing button...");
    DEBUG_PRINTF("[SLIDER] Button GPIO: %d (Active LOW)\n", BUTTON_PIN);

    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // Section 4 & 15: GPIO0 Bootloader / Startup Safeguard
    // If user held BOOT button while powering on or resetting, do NOT fire a presentation slide!
    if (isPhysicalPressed()) {
        DEBUG_PRINTLN("[SLIDER] Boot warning: Button was held during startup. Waiting for release...");
        _state = ButtonState::BOOT_WAIT_RELEASE;
        _stateTimerMs = millis();
    } else {
        _state = ButtonState::IDLE;
        DEBUG_PRINTLN("[SLIDER] Button ready (Idle)");
    }
}

ButtonEvent ButtonManager::update(uint32_t now) {
    const bool pressed = isPhysicalPressed();

    switch (_state) {
        // ---------------------------------------------------------------------
        // Startup Safety: Wait until the held boot button is cleanly released
        // ---------------------------------------------------------------------
        case ButtonState::BOOT_WAIT_RELEASE:
            if (pressed) {
                _stateTimerMs = now; // Reset timer while still held
            } else if (now - _stateTimerMs >= DEBOUNCE_MS) {
                // Stably released
                _state = ButtonState::IDLE;
                DEBUG_PRINTLN("[SLIDER] Button released after boot. Entering normal operation.");
            }
            break;

        // ---------------------------------------------------------------------
        // Idle: Awaiting first intentional press
        // ---------------------------------------------------------------------
        case ButtonState::IDLE:
            if (pressed) {
                _stateTimerMs = now;
                _state = ButtonState::DEBOUNCE_PRESS;
            }
            break;

        // ---------------------------------------------------------------------
        // Debouncing initial press: Filter electrical switch bounce
        // ---------------------------------------------------------------------
        case ButtonState::DEBOUNCE_PRESS:
            if (!pressed) {
                // False trigger / glitch shorter than DEBOUNCE_MS
                _state = ButtonState::IDLE;
            } else if (now - _stateTimerMs >= DEBOUNCE_MS) {
                // Valid intentional press detected
                _pressStartMs = now;
                _state = ButtonState::FIRST_PRESS_HELD;
                DEBUG_PRINTLN("[SLIDER] First press detected");
            }
            break;

        // ---------------------------------------------------------------------
        // First press held: Wait for physical release (implements hold immunity)
        // ---------------------------------------------------------------------
        case ButtonState::FIRST_PRESS_HELD:
            if (!pressed) {
                _stateTimerMs = now;
                _state = ButtonState::FIRST_RELEASE_DEBOUNCE;
            }
            // If the user continues to hold the button, stay in this state.
            // NO continuous or repeated keystrokes are sent while held.
            break;

        // ---------------------------------------------------------------------
        // Debouncing first release: Ensure physical contacts have settled
        // ---------------------------------------------------------------------
        case ButtonState::FIRST_RELEASE_DEBOUNCE:
            if (pressed) {
                // Contact bounce during release; back to held
                _state = ButtonState::FIRST_PRESS_HELD;
            } else if (now - _stateTimerMs >= DEBOUNCE_MS) {
                // Clean release confirmed!
                // Scenario D: Check if user held the button past the double-press window
                if (now - _pressStartMs >= DOUBLE_PRESS_WINDOW_MS) {
                    // Long hold completed: Treat as single press immediately upon release
                    _state = ButtonState::IDLE;
                    DEBUG_PRINTLN("[SLIDER] Single press confirmed (released after long hold)");
                    DEBUG_PRINTLN("[SLIDER] NEXT SLIDE");
                    return ButtonEvent::SINGLE_PRESS;
                } else {
                    // Quick press: Start double-press window countdown
                    _windowStartMs = now;
                    _state = ButtonState::WAIT_FOR_SECOND_PRESS;
                    DEBUG_PRINTLN("[SLIDER] Waiting for second press...");
                }
            }
            break;

        // ---------------------------------------------------------------------
        // Waiting for potential second press within the configured window
        // ---------------------------------------------------------------------
        case ButtonState::WAIT_FOR_SECOND_PRESS:
            if (now - _windowStartMs >= DOUBLE_PRESS_WINDOW_MS) {
                // Window expired: No second press detected -> Single Press confirmed!
                _state = ButtonState::IDLE;
                DEBUG_PRINTLN("[SLIDER] Single press confirmed");
                DEBUG_PRINTLN("[SLIDER] NEXT SLIDE");
                return ButtonEvent::SINGLE_PRESS;
            } else if (pressed) {
                // Potential second press initiated within window
                _stateTimerMs = now;
                _state = ButtonState::SECOND_PRESS_DEBOUNCE;
            }
            break;

        // ---------------------------------------------------------------------
        // Debouncing second press
        // ---------------------------------------------------------------------
        case ButtonState::SECOND_PRESS_DEBOUNCE:
            if (!pressed) {
                // Bounce/glitch during second press attempt
                _state = ButtonState::WAIT_FOR_SECOND_PRESS;
            } else if (now - _stateTimerMs >= DEBOUNCE_MS) {
                // Valid second press confirmed within window -> Double Press!
                _state = ButtonState::WAIT_FINAL_RELEASE;
                DEBUG_PRINTLN("[SLIDER] Second press detected");
                DEBUG_PRINTLN("[SLIDER] DOUBLE PRESS");
                DEBUG_PRINTLN("[SLIDER] PREVIOUS SLIDE");
                return ButtonEvent::DOUBLE_PRESS;
            }
            break;

        // ---------------------------------------------------------------------
        // Awaiting final release after double press before returning to IDLE
        // ---------------------------------------------------------------------
        case ButtonState::WAIT_FINAL_RELEASE:
            if (!pressed) {
                _stateTimerMs = now;
                // Wait for debounce confirmation of release
                _state = ButtonState::IDLE;
            }
            break;
    }

    return ButtonEvent::NONE;
}
