#include <Arduino.h>
#include "config.h"
#include "button_manager.h"
#include "hid_manager.h"
#include "led_manager.h"

// =============================================================================
// SLIDER V1 — ESP32 WIRELESS PRESENTATION REMOTE
// =============================================================================
// Target: Classic ESP32 (ESP32-WROOM-32 / DevKit V1)
// Hardware: 1 x ESP32 + 1 x Built-in BOOT Button (GPIO 0)
// Status LED: Built-in LED (GPIO 2) — Blinks rapidly when disconnected, solid when connected
// Bluetooth: Wireless Bluetooth HID Keyboard
// Controls: Single Press -> Next Slide (Right Arrow)
//           Double Press -> Previous Slide (Left Arrow)
// =============================================================================

static ButtonManager g_buttonManager;
static HidManager    g_hidManager;
static LedManager    g_ledManager;

void setup() {
#if DEBUG_ENABLED
    Serial.begin(DEBUG_BAUD_RATE);
    delay(500); // Allow UART bridge to settle
    Serial.println();
    Serial.println("==============================================");
    Serial.println("         SLIDER V1 — Presentation Remote      ");
    Serial.println("==============================================");
    Serial.println("[SLIDER] Booting firmware...");
#endif

    // 1. Initialize built-in status LED indicator
    g_ledManager.init();

    // 2. Initialize built-in BOOT button with startup hold safety
    g_buttonManager.init();

    // 3. Initialize Bluetooth HID keyboard subsystem
    g_hidManager.init();

#if DEBUG_ENABLED
    Serial.println("[SLIDER] System initialization complete.");
    Serial.println("[SLIDER] Ready for host pairing / connection.");
    Serial.println("----------------------------------------------");
#endif
}

void loop() {
    const uint32_t now = millis();

    // 1. Monitor Bluetooth host connection lifecycle
    g_hidManager.update();

    // 2. Update status LED (rapid blink when disconnected, solid glow when connected)
    g_ledManager.update(now, g_hidManager.isConnected());

    // 3. Process non-blocking button state machine
    const ButtonEvent event = g_buttonManager.update(now);

    // 3. Dispatch presentation commands based on detected event
    switch (event) {
        case ButtonEvent::SINGLE_PRESS:
            g_hidManager.sendNextSlide();
            break;

        case ButtonEvent::DOUBLE_PRESS:
            g_hidManager.sendPrevSlide();
            break;

        case ButtonEvent::NONE:
        default:
            break;
    }
}
