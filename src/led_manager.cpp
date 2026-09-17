#include "led_manager.h"

LedManager::LedManager()
    : _lastToggleMs(0)
    , _ledState(false)
{
}

void LedManager::init() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    _ledState = false;
    DEBUG_PRINTF("[SLIDER] Initialized built-in LED on GPIO %d\n", LED_PIN);
}

void LedManager::update(uint32_t now, bool isConnected) {
    if (isConnected) {
        // Bluetooth Connected: Solid constant glow
        if (!_ledState) {
            digitalWrite(LED_PIN, HIGH);
            _ledState = true;
        }
    } else {
        // Bluetooth Disconnected: Rapid blink (150ms toggle)
        if (now - _lastToggleMs >= LED_BLINK_DISCONNECTED_MS) {
            _lastToggleMs = now;
            _ledState = !_ledState;
            digitalWrite(LED_PIN, _ledState ? HIGH : LOW);
        }
    }
}
