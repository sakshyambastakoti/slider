#pragma once

#include <Arduino.h>

// =============================================================================
// SLIDER V1 — SYSTEM CONFIGURATION
// =============================================================================

// -----------------------------------------------------------------------------
// 1. Serial Debugging
// -----------------------------------------------------------------------------
#define DEBUG_ENABLED true
constexpr uint32_t DEBUG_BAUD_RATE = 115200;

#if DEBUG_ENABLED
    #define DEBUG_PRINT(x)    Serial.print(x)
    #define DEBUG_PRINTLN(x)  Serial.println(x)
    #define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(...)
#endif

// -----------------------------------------------------------------------------
// 2. Hardware / GPIO Configuration
// -----------------------------------------------------------------------------
// Built-in BOOT button on standard Classic ESP32 boards (active LOW with pull-up)
constexpr uint8_t BUTTON_PIN = 0;

// -----------------------------------------------------------------------------
// 3. Timing Parameters (in milliseconds)
// -----------------------------------------------------------------------------
// Debounce delay to eliminate mechanical contact chatter
constexpr uint32_t DEBOUNCE_MS = 30;

// Maximum time window after first press release to accept a second press
constexpr uint32_t DOUBLE_PRESS_WINDOW_MS = 300;

// Duration the HID key is held down before release to ensure host OS recognition
constexpr uint32_t KEY_STROKE_DELAY_MS = 20;

// -----------------------------------------------------------------------------
// 4. Bluetooth HID Identity
// -----------------------------------------------------------------------------
constexpr char DEVICE_NAME[]         = "SLIDER";
constexpr char DEVICE_MANUFACTURER[] = "SLIDER Systems";
constexpr uint8_t BATTERY_LEVEL      = 100;
