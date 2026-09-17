/**
 * =============================================================================
 * SLIDER V1 — ESP32 Wireless Bluetooth Presentation Remote
 * =============================================================================
 * Target: Classic ESP32 (ESP32-WROOM-32 / DevKit V1)
 * Button: Built-in BOOT button on GPIO 0 (Active LOW with internal pull-up)
 * 
 * Controls:
 *   - Single Press : Next Slide     (Sends KEY_RIGHT_ARROW)
 *   - Double Press : Previous Slide (Sends KEY_LEFT_ARROW)
 *   - Long Hold    : Single press only upon release (No repeat spam)
 * 
 * Dependencies (Arduino IDE Library Manager):
 *   - "ESP32 BLE Keyboard" by T-vK (v0.3.2 or compatible)
 *   - Optional: "NimBLE-Arduino" by h2zero (v1.4.1+) for reduced RAM/Flash
 * =============================================================================
 */

#include <Arduino.h>
#include <BleKeyboard.h>

// =============================================================================
// 1. CONFIGURATION
// =============================================================================
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

// Target Button Hardware Configuration (Classic ESP32 built-in BOOT button)
constexpr uint8_t BUTTON_PIN = 0;

// Built-in status LED (GPIO 2 on standard Classic ESP32 DevKit boards)
#ifdef LED_BUILTIN
constexpr uint8_t LED_PIN = LED_BUILTIN;
#else
constexpr uint8_t LED_PIN = 2;
#endif

// Timing Configurations (in milliseconds)
constexpr uint32_t DEBOUNCE_MS             = 30;   // Debounce window to filter mechanical chatter
constexpr uint32_t DOUBLE_PRESS_WINDOW_MS = 300;  // Double-press detection window
constexpr uint32_t KEY_STROKE_DELAY_MS    = 20;   // Key hold duration before release
constexpr uint32_t LED_BLINK_DISCONNECTED_MS = 150; // LED rapid blink interval (150ms toggle)

// Bluetooth HID Device Profile
constexpr char DEVICE_NAME[]         = "SLIDER";
constexpr char DEVICE_MANUFACTURER[] = "SLIDER Systems";
constexpr uint8_t BATTERY_LEVEL      = 100;

// =============================================================================
// 2. BUTTON STATE MACHINE DEFINITION
// =============================================================================
enum class ButtonEvent : uint8_t {
    NONE = 0,
    SINGLE_PRESS,
    DOUBLE_PRESS
};

enum class ButtonState : uint8_t {
    BOOT_WAIT_RELEASE,
    IDLE,
    DEBOUNCE_PRESS,
    FIRST_PRESS_HELD,
    FIRST_RELEASE_DEBOUNCE,
    WAIT_FOR_SECOND_PRESS,
    SECOND_PRESS_DEBOUNCE,
    WAIT_FINAL_RELEASE
};

class ButtonManager {
public:
    ButtonManager()
        : _state(ButtonState::IDLE)
        , _stateTimerMs(0)
        , _pressStartMs(0)
        , _windowStartMs(0)
    {}

    void init() {
        DEBUG_PRINTLN("[SLIDER] Initializing button...");
        DEBUG_PRINTF("[SLIDER] Button GPIO: %d (Active LOW)\n", BUTTON_PIN);

        pinMode(BUTTON_PIN, INPUT_PULLUP);

        // Startup Safeguard: If BOOT was held down during power-on/reset, wait for release
        if (isPhysicalPressed()) {
            DEBUG_PRINTLN("[SLIDER] Boot warning: Button was held during startup. Waiting for release...");
            _state = ButtonState::BOOT_WAIT_RELEASE;
            _stateTimerMs = millis();
        } else {
            _state = ButtonState::IDLE;
            DEBUG_PRINTLN("[SLIDER] Button ready (Idle)");
        }
    }

    ButtonEvent update(uint32_t now) {
        const bool pressed = isPhysicalPressed();

        switch (_state) {
            case ButtonState::BOOT_WAIT_RELEASE:
                if (pressed) {
                    _stateTimerMs = now;
                } else if (now - _stateTimerMs >= DEBOUNCE_MS) {
                    _state = ButtonState::IDLE;
                    DEBUG_PRINTLN("[SLIDER] Button released after boot. Entering normal operation.");
                }
                break;

            case ButtonState::IDLE:
                if (pressed) {
                    _stateTimerMs = now;
                    _state = ButtonState::DEBOUNCE_PRESS;
                }
                break;

            case ButtonState::DEBOUNCE_PRESS:
                if (!pressed) {
                    _state = ButtonState::IDLE;
                } else if (now - _stateTimerMs >= DEBOUNCE_MS) {
                    _pressStartMs = now;
                    _state = ButtonState::FIRST_PRESS_HELD;
                    DEBUG_PRINTLN("[SLIDER] First press detected");
                }
                break;

            case ButtonState::FIRST_PRESS_HELD:
                if (!pressed) {
                    _stateTimerMs = now;
                    _state = ButtonState::FIRST_RELEASE_DEBOUNCE;
                }
                // No repeat actions while held
                break;

            case ButtonState::FIRST_RELEASE_DEBOUNCE:
                if (pressed) {
                    _state = ButtonState::FIRST_PRESS_HELD;
                } else if (now - _stateTimerMs >= DEBOUNCE_MS) {
                    if (now - _pressStartMs >= DOUBLE_PRESS_WINDOW_MS) {
                        _state = ButtonState::IDLE;
                        DEBUG_PRINTLN("[SLIDER] Single press confirmed (released after long hold)");
                        DEBUG_PRINTLN("[SLIDER] NEXT SLIDE");
                        return ButtonEvent::SINGLE_PRESS;
                    } else {
                        _windowStartMs = now;
                        _state = ButtonState::WAIT_FOR_SECOND_PRESS;
                        DEBUG_PRINTLN("[SLIDER] Waiting for second press...");
                    }
                }
                break;

            case ButtonState::WAIT_FOR_SECOND_PRESS:
                if (now - _windowStartMs >= DOUBLE_PRESS_WINDOW_MS) {
                    _state = ButtonState::IDLE;
                    DEBUG_PRINTLN("[SLIDER] Single press confirmed");
                    DEBUG_PRINTLN("[SLIDER] NEXT SLIDE");
                    return ButtonEvent::SINGLE_PRESS;
                } else if (pressed) {
                    _stateTimerMs = now;
                    _state = ButtonState::SECOND_PRESS_DEBOUNCE;
                }
                break;

            case ButtonState::SECOND_PRESS_DEBOUNCE:
                if (!pressed) {
                    _state = ButtonState::WAIT_FOR_SECOND_PRESS;
                } else if (now - _stateTimerMs >= DEBOUNCE_MS) {
                    _state = ButtonState::WAIT_FINAL_RELEASE;
                    DEBUG_PRINTLN("[SLIDER] Second press detected");
                    DEBUG_PRINTLN("[SLIDER] DOUBLE PRESS");
                    DEBUG_PRINTLN("[SLIDER] PREVIOUS SLIDE");
                    return ButtonEvent::DOUBLE_PRESS;
                }
                break;

            case ButtonState::WAIT_FINAL_RELEASE:
                if (!pressed) {
                    _state = ButtonState::IDLE;
                }
                break;
        }

        return ButtonEvent::NONE;
    }

private:
    ButtonState _state;
    uint32_t    _stateTimerMs;
    uint32_t    _pressStartMs;
    uint32_t    _windowStartMs;

    inline bool isPhysicalPressed() const {
        return (digitalRead(BUTTON_PIN) == LOW);
    }
};

// =============================================================================
// 3. BLUETOOTH HID MANAGER
// =============================================================================
class HidManager {
public:
    HidManager()
        : _bleKeyboard(DEVICE_NAME, DEVICE_MANUFACTURER, BATTERY_LEVEL)
        , _wasConnected(false)
    {}

    void init() {
        DEBUG_PRINTLN("[SLIDER] Initializing Bluetooth HID...");
        DEBUG_PRINTF("[SLIDER] Device name: %s\n", DEVICE_NAME);
        _bleKeyboard.begin();
        DEBUG_PRINTLN("[SLIDER] Waiting for Bluetooth connection...");
    }

    void update() {
        const bool connected = isConnected();

        if (connected && !_wasConnected) {
            DEBUG_PRINTLN("[SLIDER] **************************************");
            DEBUG_PRINTLN("[SLIDER] Bluetooth connected! Host ready.");
            DEBUG_PRINTLN("[SLIDER] SLIDER READY FOR PRESENTATIONS");
            DEBUG_PRINTLN("[SLIDER] **************************************");
            _wasConnected = true;
        } else if (!connected && _wasConnected) {
            DEBUG_PRINTLN("[SLIDER] Bluetooth disconnected. Waiting for reconnection...");
            _bleKeyboard.releaseAll();
            _wasConnected = false;
        }
    }

    bool isConnected() const {
        return const_cast<BleKeyboard&>(_bleKeyboard).isConnected();
    }

    bool sendNextSlide() {
        if (!isConnected()) {
            DEBUG_PRINTLN("[SLIDER] Warning: NEXT SLIDE dropped (Bluetooth not connected).");
            return false;
        }

        DEBUG_PRINTLN("[SLIDER] Sending HID: KEY_RIGHT_ARROW (Next Slide)");
        _bleKeyboard.press(KEY_RIGHT_ARROW);
        delay(KEY_STROKE_DELAY_MS);
        _bleKeyboard.release(KEY_RIGHT_ARROW);
        _bleKeyboard.releaseAll();
        return true;
    }

    bool sendPrevSlide() {
        if (!isConnected()) {
            DEBUG_PRINTLN("[SLIDER] Warning: PREVIOUS SLIDE dropped (Bluetooth not connected).");
            return false;
        }

        DEBUG_PRINTLN("[SLIDER] Sending HID: KEY_LEFT_ARROW (Previous Slide)");
        _bleKeyboard.press(KEY_LEFT_ARROW);
        delay(KEY_STROKE_DELAY_MS);
        _bleKeyboard.release(KEY_LEFT_ARROW);
        _bleKeyboard.releaseAll();
        return true;
    }

private:
    BleKeyboard _bleKeyboard;
    bool        _wasConnected;
};

// =============================================================================
// 4. LED STATUS MANAGER
// =============================================================================
class LedManager {
public:
    LedManager()
        : _lastToggleMs(0)
        , _ledState(false)
    {}

    void init() {
        pinMode(LED_PIN, OUTPUT);
        digitalWrite(LED_PIN, LOW);
        _ledState = false;
        DEBUG_PRINTF("[SLIDER] Initialized built-in LED on GPIO %d\n", LED_PIN);
    }

    void update(uint32_t now, bool isConnected) {
        if (isConnected) {
            // Solid constant glow when connected
            if (!_ledState) {
                digitalWrite(LED_PIN, HIGH);
                _ledState = true;
            }
        } else {
            // Rapid blink when disconnected
            if (now - _lastToggleMs >= LED_BLINK_DISCONNECTED_MS) {
                _lastToggleMs = now;
                _ledState = !_ledState;
                digitalWrite(LED_PIN, _ledState ? HIGH : LOW);
            }
        }
    }

private:
    uint32_t _lastToggleMs;
    bool     _ledState;
};

// =============================================================================
// 5. MAIN APPLICATION
// =============================================================================
static ButtonManager g_buttonManager;
static HidManager    g_hidManager;
static LedManager    g_ledManager;

void setup() {
#if DEBUG_ENABLED
    Serial.begin(DEBUG_BAUD_RATE);
    delay(500);
    Serial.println();
    Serial.println("==============================================");
    Serial.println("         SLIDER V1 — Presentation Remote      ");
    Serial.println("==============================================");
    Serial.println("[SLIDER] Booting firmware...");
#endif

    // 1. Initialize status LED
    g_ledManager.init();

    // 2. Initialize button
    g_buttonManager.init();

    // 3. Initialize Bluetooth HID
    g_hidManager.init();

#if DEBUG_ENABLED
    Serial.println("[SLIDER] System initialization complete.");
    Serial.println("[SLIDER] Ready for host pairing / connection.");
    Serial.println("----------------------------------------------");
#endif
}

void loop() {
    const uint32_t now = millis();

    // 1. Monitor Bluetooth lifecycle
    g_hidManager.update();

    // 2. Update status LED (rapid blink if disconnected, solid if connected)
    g_ledManager.update(now, g_hidManager.isConnected());

    // 3. Process non-blocking button state machine
    const ButtonEvent event = g_buttonManager.update(now);

    // 4. Dispatch presentation actions
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
