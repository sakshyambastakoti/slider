#pragma once

#include <Arduino.h>
#include "config.h"

/**
 * @brief Events emitted by the ButtonManager state machine.
 */
enum class ButtonEvent : uint8_t {
    NONE = 0,
    SINGLE_PRESS,   // Maps to Next Slide (Right Arrow)
    DOUBLE_PRESS    // Maps to Previous Slide (Left Arrow)
};

/**
 * @brief Internal states for the non-blocking button state machine.
 */
enum class ButtonState : uint8_t {
    BOOT_WAIT_RELEASE,       // Safety guard: if BOOT was held down during power-on
    IDLE,                    // Awaiting physical button press
    DEBOUNCE_PRESS,          // Validating button press is held stable >= DEBOUNCE_MS
    FIRST_PRESS_HELD,        // Button is physically held down during the 1st press
    FIRST_RELEASE_DEBOUNCE,  // Validating button release is held stable >= DEBOUNCE_MS
    WAIT_FOR_SECOND_PRESS,   // Button released; waiting up to DOUBLE_PRESS_WINDOW_MS
    SECOND_PRESS_DEBOUNCE,   // Validating 2nd press is held stable >= DEBOUNCE_MS
    WAIT_FINAL_RELEASE       // Awaiting button release following a confirmed double press
};

/**
 * @brief Manages button debouncing, single-press, and double-press state machine.
 */
class ButtonManager {
public:
    ButtonManager();

    /**
     * @brief Configures GPIO, reads initial state, and establishes startup safety.
     */
    void init();

    /**
     * @brief Periodic non-blocking update routine.
     * @param now Current timestamp in milliseconds (from millis()).
     * @return ButtonEvent Detected logical event (NONE, SINGLE_PRESS, or DOUBLE_PRESS).
     */
    ButtonEvent update(uint32_t now);

    /**
     * @brief Returns current internal state (useful for diagnostics).
     */
    ButtonState getState() const { return _state; }

private:
    ButtonState _state;
    uint32_t    _stateTimerMs;
    uint32_t    _pressStartMs;
    uint32_t    _windowStartMs;

    bool isPhysicalPressed() const;
};
