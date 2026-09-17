#pragma once

#include <Arduino.h>
#include <BleKeyboard.h>
#include "config.h"

/**
 * @brief Manages Bluetooth HID keyboard lifecycle, connection states, and slide navigation.
 */
class HidManager {
public:
    HidManager();

    /**
     * @brief Initializes the Bluetooth HID subsystem and starts advertising.
     */
    void init();

    /**
     * @brief Periodic update to monitor host connection state transitions.
     */
    void update();

    /**
     * @brief Checks whether the Bluetooth HID device is currently connected to a host computer.
     */
    bool isConnected() const;

    /**
     * @brief Emits a NEXT SLIDE action (Right Arrow) with full down/delay/up sequence.
     * @return true if command was transmitted, false if suppressed (disconnected).
     */
    bool sendNextSlide();

    /**
     * @brief Emits a PREVIOUS SLIDE action (Left Arrow) with full down/delay/up sequence.
     * @return true if command was transmitted, false if suppressed (disconnected).
     */
    bool sendPrevSlide();

private:
    BleKeyboard _bleKeyboard;
    bool        _wasConnected;

    void executeKeyPress(uint8_t keycode, const char* actionLabel);
};
