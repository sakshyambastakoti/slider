#include "hid_manager.h"

HidManager::HidManager()
    : _bleKeyboard(DEVICE_NAME, DEVICE_MANUFACTURER, BATTERY_LEVEL)
    , _wasConnected(false)
{
}

void HidManager::init() {
    DEBUG_PRINTLN("[SLIDER] Initializing Bluetooth HID...");
    DEBUG_PRINTF("[SLIDER] Device name: %s\n", DEVICE_NAME);
    DEBUG_PRINTF("[SLIDER] Manufacturer: %s\n", DEVICE_MANUFACTURER);

    _bleKeyboard.begin();

    DEBUG_PRINTLN("[SLIDER] Waiting for Bluetooth connection...");
}

bool HidManager::isConnected() const {
    return const_cast<BleKeyboard&>(_bleKeyboard).isConnected();
}

void HidManager::update() {
    const bool connected = isConnected();

    // Log connection state transitions
    if (connected && !_wasConnected) {
        DEBUG_PRINTLN("[SLIDER] **************************************");
        DEBUG_PRINTLN("[SLIDER] Bluetooth connected! Host ready.");
        DEBUG_PRINTLN("[SLIDER] SLIDER READY FOR PRESENTATIONS");
        DEBUG_PRINTLN("[SLIDER] **************************************");
        _wasConnected = true;
    } else if (!connected && _wasConnected) {
        DEBUG_PRINTLN("[SLIDER] Bluetooth disconnected. Waiting for reconnection...");
        _bleKeyboard.releaseAll(); // Clean up any active modifiers or keys
        _wasConnected = false;
    }
}

void HidManager::executeKeyPress(uint8_t keycode, const char* actionLabel) {
    if (!isConnected()) {
        DEBUG_PRINTF("[SLIDER] Action '%s' suppressed: Bluetooth host not connected.\n", actionLabel);
        return;
    }

    // Full press-hold-release cycle as required by Section 12
    // KEY DOWN -> short interval -> KEY UP
    _bleKeyboard.press(keycode);
    delay(KEY_STROKE_DELAY_MS);
    _bleKeyboard.release(keycode);
    _bleKeyboard.releaseAll(); // Extra guard: ensure no keys remain pressed
}

bool HidManager::sendNextSlide() {
    if (!isConnected()) {
        DEBUG_PRINTLN("[SLIDER] Warning: NEXT SLIDE dropped (Bluetooth not connected).");
        return false;
    }

    DEBUG_PRINTLN("[SLIDER] Sending HID: KEY_RIGHT_ARROW (Next Slide)");
    executeKeyPress(KEY_RIGHT_ARROW, "NEXT_SLIDE");
    return true;
}

bool HidManager::sendPrevSlide() {
    if (!isConnected()) {
        DEBUG_PRINTLN("[SLIDER] Warning: PREVIOUS SLIDE dropped (Bluetooth not connected).");
        return false;
    }

    DEBUG_PRINTLN("[SLIDER] Sending HID: KEY_LEFT_ARROW (Previous Slide)");
    executeKeyPress(KEY_LEFT_ARROW, "PREV_SLIDE");
    return true;
}
