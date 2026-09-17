#pragma once

#include <Arduino.h>
#include "config.h"

/**
 * @brief Manages the built-in LED status indicator.
 *        - Rapidly blinks when Bluetooth is disconnected.
 *        - Glows constantly (solid ON) when Bluetooth is connected.
 */
class LedManager {
public:
    LedManager();

    /**
     * @brief Configures LED pin as OUTPUT and turns it off initially.
     */
    void init();

    /**
     * @brief Periodic non-blocking update routine.
     * @param now Current timestamp in milliseconds (from millis()).
     * @param isConnected Current Bluetooth connection state.
     */
    void update(uint32_t now, bool isConnected);

private:
    uint32_t _lastToggleMs;
    bool     _ledState;
};
