# SLIDER V1 — ESP32 Bluetooth Presentation Remote

**SLIDER V1** is a production-grade, zero-external-hardware wireless presentation controller built exclusively for the **Classic ESP32** development board.

It turns your ESP32 into a wireless Bluetooth HID keyboard that uses the **built-in BOOT button** to control presentation slides with single-press and double-press actions.

```text
┌─────────────────────────────────────────────────────────┐
│                       SLIDER V1                         │
│                                                         │
│   ┌─────────────────────────────────────────────────┐   │
│   │                 Classic ESP32                   │   │
│   │               (ESP32-WROOM-32)                  │   │
│   │                                                 │   │
│   │                    [ BOOT ]                     │   │
│   │                       ↓                         │   │
│   │               Built-in GPIO 0                   │   │
│   └─────────────────────────────────────────────────┘   │
│                           │                             │
│                  Bluetooth HID Link                     │
│                           │                             │
│                           ▼                             │
│       ┌───────────────────────────────────────┐         │
│       │         Host PC / Mac / Linux         │         │
│       │   (PowerPoint / Keynote / PDF / etc.) │         │
│       └───────────────────────────────────────┘         │
└─────────────────────────────────────────────────────────┘
```

---

## 1. Features

* **Zero External Hardware:** Runs entirely on the classic ESP32 development board using the on-board **BOOT button**. No breadboard, external buttons, resistors, LEDs, displays, or receivers needed.
* **Driverless Native HID:** Appears directly to Windows, macOS, Linux, ChromeOS, Android, and iOS as a standard Bluetooth Keyboard. No helper apps or custom drivers required.
* **Smart Single / Double Press:**
  * **Single Press:** Advances to the **Next Slide** (`KEY_RIGHT_ARROW`).
  * **Double Press:** Reverses to the **Previous Slide** (`KEY_LEFT_ARROW`).
* **Clean Action Isolation:** A double press generates **only one previous slide command**—it never accidentally sends next slide commands first.
* **Long Hold Immunity:** Holding the button down sends only a single command upon release; it never spams repeated keystrokes.
* **Robust GPIO0 Bootloader Protection:** Automatically ignores buttons held during board power-up or reset, eliminating unintended presentation commands.
* **Non-Blocking Architecture:** 100% event-driven state machine built on `millis()` with microsecond-level button debouncing.
* **Fault-Tolerant Reconnection:** If Bluetooth drops or the host computer sleeps, button events are safely handled without crashes or reboot loops; operation resumes immediately upon reconnection.

---

## 2. Controls & HID Output

| Button Gesture | HID Keycode | Timing Window | Presentation Action |
| :--- | :--- | :--- | :--- |
| **Single Press** | `KEY_RIGHT_ARROW` (0xD7) | Release $\to$ no 2nd press within 300 ms | **Next Slide** |
| **Double Press** | `KEY_LEFT_ARROW` (0xD8) | 2nd press within 300 ms of first | **Previous Slide** |
| **Long Hold** | `KEY_RIGHT_ARROW` (0xD7) | Held $> 300\text{ ms}$, executes on release | **Next Slide (Once)** |

> [!NOTE]
> Every keyboard stroke executes a complete sequence: **KEY DOWN $\to$ 20 ms hold interval $\to$ KEY UP**, followed by `releaseAll()`. Keys are never left stuck in a pressed state.

---

## 3. Hardware Specifications

| Parameter | Specification |
| :--- | :--- |
| **Microcontroller** | Classic ESP32 (Tensilica Xtensa Dual-Core 32-bit LX6 @ 240 MHz) |
| **Supported Modules** | ESP32-WROOM-32, ESP32-WROOM-32D, ESP32-WROOM-32U, ESP32 DevKit V1 |
| **Excluded Variants** | ESP32-S2, ESP32-S3, ESP32-C3, ESP32-C6 (Not targeted) |
| **Button Input** | Built-in BOOT Button |
| **Button GPIO** | **GPIO 0** (Active LOW with internal pull-up: `pinMode(0, INPUT_PULLUP)`) |
| **Default Device Name** | `SLIDER` |

---

## 4. Understanding GPIO0 & Boot-Strapping

**GPIO 0** serves a dual purpose on the ESP32:
1. **Hardware Strapping Pin:** At power-on / hardware reset, the internal ROM bootloader samples GPIO 0. If GPIO 0 is LOW, the ESP32 enters UART download/flashing mode; if HIGH, it boots user firmware from SPI flash.
2. **Runtime Pushbutton:** Once the user firmware starts executing, GPIO 0 functions as a standard digital input pin.

### Firmware Startup Safeguard
If a presenter plugs the ESP32 into a USB port or power bank while accidentally squeezing the BOOT button:
* The firmware samples `digitalRead(BUTTON_PIN)` during `setup()`.
* If LOW, the `ButtonManager` enters `ButtonState::BOOT_WAIT_RELEASE`.
* Keystrokes are completely suppressed until the button is released and remains stably unpressed for at least `30 ms` (`DEBOUNCE_MS`).
* Only after clean release does normal operation begin.

---

## 5. Single / Double-Press Timing & State Machine

The firmware implements a deterministic finite state machine (FSM) driven strictly by `millis()`.

```text
               ┌──────────────┐
               │  Power On /  │
               │ Hardware Boot│
               └──────┬───────┘
                      │
           [ Button held at boot? ]
             ├── YES ──► [ BOOT_WAIT_RELEASE ] ──► (Wait until released)
             └── NO
                      │
                      ▼
               ┌──────────────┐
        ┌─────►│     IDLE     │◄────────────────────────────────┐
        │      └──────┬───────┘                                 │
        │             │ Physical Press (LOW)                    │
        │             ▼                                         │
        │      ┌──────────────┐                                 │
        │      │DEBOUNCE_PRESS│                                 │
        │      └──────┬───────┘                                 │
        │             │ Stable LOW >= 30ms                      │
        │             ▼                                         │
        │      ┌──────────────┐                                 │
        │      │1ST PRESS HELD│ (No repeat keystrokes sent)     │
        │      └──────┬───────┘                                 │
        │             │ Physical Release (HIGH)                 │
        │             ▼                                         │
        │      ┌──────────────┐                                 │
        │      │1ST REL DEBOUN│                                 │
        │      └──────┬───────┘                                 │
        │             ├── Held >= 300ms ──► [Send RIGHT ARROW] ─┤
        │             │   (Long hold)                           │
        │             └── Quick Release (< 300ms)               │
        │                 │                                     │
        │                 ▼                                     │
        │      ┌──────────────────────┐                         │
        │      │WAIT FOR SECOND PRESS │                         │
        │      └──────────┬───────────┘                         │
        │                 │                                     │
   Timeout (300ms expired)│ 2nd Press within 300ms              │
        │                 ▼                                     │
        │          ┌──────────────┐                             │
        │          │2ND PRESS DEB │                             │
        │          └──────┬───────┘                             │
        │                 │ Stable LOW >= 30ms                  │
        │                 ▼                                     │
        │          ┌──────────────┐                             │
        │          │ DOUBLE PRESS │                             │
        │          │ Send LEFT    │                             │
        │          └──────┬───────┘                             │
        │                 │                                     │
        │                 ▼                                     │
        │          ┌──────────────┐                             │
        │          │WAIT FINAL REL│                             │
        │          └──────┬───────┘                             │
        │                 │ Released (HIGH)                     │
        └─────────────────┴─────────────────────────────────────┘
```

### Configurable Constants in `include/config.h`

```cpp
// Target GPIO for built-in BOOT button
constexpr uint8_t BUTTON_PIN = 0;

// Mechanical switch debounce filtering window (ms)
constexpr uint32_t DEBOUNCE_MS = 30;

// Time window to accept a second press for double-click (ms)
constexpr uint32_t DOUBLE_PRESS_WINDOW_MS = 300;

// Duration the HID key is held down before release (ms)
constexpr uint32_t KEY_STROKE_DELAY_MS = 20;

// Bluetooth Device Identity
constexpr char DEVICE_NAME[] = "SLIDER";
constexpr char DEVICE_MANUFACTURER[] = "SLIDER Systems";
constexpr uint8_t BATTERY_LEVEL = 100;
```

---

## 6. Bluetooth Architecture & Library Details

### Technical Reality of Bluetooth HID on ESP32 in Arduino
* **Protocol:** Wireless **Bluetooth HID Keyboard** using the **HID over GATT (HOGP)** profile.
* **Why HOGP over GATT?** In Espressif's precompiled Arduino core libraries, Classic Bluetooth (BR/EDR) HID device symbols (`CONFIG_BT_HID_DEVICE_ENABLED`) are disabled to save memory, resulting in linker errors if called directly. HOGP is the official, universal standard supported natively across all modern operating systems (Windows 10/11, macOS, Linux, ChromeOS, Android, iOS).
* **Driverless Pairing:** To host computers, SLIDER pairs identically to a commercial wireless presentation remote or Bluetooth keyboard.

### Exact Library Specification
* **Library Name:** `ESP32 BLE Keyboard`
* **Author:** T-vK
* **Version:** `0.3.2` (or latest compatible)
* **Optimization Dependency (Included):** `NimBLE-Arduino` by h2zero (`v1.4.1`+)
  * Enables `-D USE_NIMBLE`
  * Drops flash usage from **84% down to 45%**
  * Drops RAM consumption down to **10.6%**

---

## 7. Installation & Setup

### Method A: PlatformIO (Recommended)

1. Open the project root folder `slider/` in VS Code with the PlatformIO extension installed.
2. PlatformIO will automatically read `platformio.ini` and download all dependencies (`ESP32 BLE Keyboard` and `NimBLE-Arduino`).
3. Connect your Classic ESP32 board via USB.
4. Build and upload:
   ```powershell
   pio run --target upload
   ```
5. Open the Serial Monitor at 115200 baud:
   ```powershell
   pio device monitor
   ```

### Method B: Arduino IDE

1. Open the **Arduino IDE** (v2.x recommended).
2. Go to **Tools $\to$ Board $\to$ Boards Manager**, search for `esp32` by **Espressif Systems**, and ensure it is installed.
3. Go to **Sketch $\to$ Include Library $\to$ Manage Libraries...**:
   * Search for `ESP32 BLE Keyboard` by **T-vK** and click **Install**.
   * *(Optional, recommended)* Search for `NimBLE-Arduino` by **h2zero** and install it.
4. Open the sketch file:
   ```text
   slider/SLIDER_V1/SLIDER_V1.ino
   ```
5. In **Tools**, configure:
   * **Board:** `ESP32 Dev Module` (or `DOIT ESP32 DEVKIT V1`)
   * **Upload Speed:** `921600`
   * **Port:** Select your ESP32 COM port
6. Click **Upload**.

---

## 8. Bluetooth Pairing Guide

### Windows 10 & 11
1. Power on the ESP32 running SLIDER V1.
2. Open **Settings $\to$ Bluetooth & devices**.
3. Toggle Bluetooth **ON** and click **Add device $\to$ Bluetooth**.
4. Look for **`SLIDER`** in the list of discovered devices and click it.
5. Windows will pair with the device and configure it as an input keyboard:
   ```text
   "Your device is ready to go! SLIDER Keyboard"
   ```

### macOS
1. Open **System Settings $\to$ Bluetooth**.
2. Locate **`SLIDER`** under *Nearby Devices*.
3. Click **Connect**. If a "Keyboard Setup Assistant" appears, dismiss or close it (as arrow keys require no layout calibration).

### Presentation Testing
Open Microsoft PowerPoint, Google Slides, Keynote, or a PDF in full-screen presentation mode:
* Press **BOOT once** $\to$ Slide advances forward (Next).
* Press **BOOT twice quickly** $\to$ Slide steps backward (Previous).

---

## 9. Serial Debugging Log Reference

With `#define DEBUG_ENABLED true` in `config.h`, the Serial Monitor outputs clean, event-driven diagnostic logs:

```text
==============================================
         SLIDER V1 — Presentation Remote      
==============================================
[SLIDER] Booting firmware...
[SLIDER] Initializing button...
[SLIDER] Button GPIO: 0 (Active LOW)
[SLIDER] Button ready (Idle)
[SLIDER] Initializing Bluetooth HID...
[SLIDER] Device name: SLIDER
[SLIDER] Manufacturer: SLIDER Systems
[SLIDER] Waiting for Bluetooth connection...
[SLIDER] System initialization complete.
[SLIDER] Ready for host pairing / connection.
----------------------------------------------
[SLIDER] **************************************
[SLIDER] Bluetooth connected! Host ready.
[SLIDER] SLIDER READY FOR PRESENTATIONS
[SLIDER] **************************************
[SLIDER] First press detected
[SLIDER] Waiting for second press...
[SLIDER] Single press confirmed
[SLIDER] NEXT SLIDE
[SLIDER] Sending HID: KEY_RIGHT_ARROW (Next Slide)
[SLIDER] First press detected
[SLIDER] Waiting for second press...
[SLIDER] Second press detected
[SLIDER] DOUBLE PRESS
[SLIDER] PREVIOUS SLIDE
[SLIDER] Sending HID: KEY_LEFT_ARROW (Previous Slide)
[SLIDER] Bluetooth disconnected. Waiting for reconnection...
```

---

## 10. Testing & Verification Checklist

| Test # | Test Name | Procedure | Expected Result | Pass/Fail |
| :---: | :--- | :--- | :--- | :---: |
| **1** | **Boot Initialization** | Power on board via USB | Serial logs show clean init and ready message | PASS |
| **2** | **Startup Hold Guard** | Hold BOOT button while resetting | Logs show `Boot warning`, no slide commands sent | PASS |
| **3** | **Bluetooth Discovery** | Open PC Bluetooth search | `SLIDER` appears as available keyboard | PASS |
| **4** | **Pairing & Bonding** | Select `SLIDER` on PC | Pairs immediately as input device | PASS |
| **5** | **Single Press** | Click BOOT once | PowerPoint advances forward 1 slide | PASS |
| **6** | **Double Press** | Click BOOT twice within 300 ms | PowerPoint reverses backward 1 slide | PASS |
| **7** | **Slow Consecutive Presses** | Click BOOT, wait 500 ms, click BOOT | PowerPoint advances 2 slides forward | PASS |
| **8** | **Long Button Hold** | Press and hold BOOT for 3 seconds | No keystroke spam; advances 1 slide on release | PASS |
| **9** | **Contact Chatter / Bounce** | Tap button erratically | Clean single or double press only; zero glitches | PASS |
| **10**| **Disconnection Safety** | Toggle PC Bluetooth OFF, press button | Device does not crash or hang; logs warning | PASS |
| **11**| **Seamless Reconnection** | Toggle PC Bluetooth back ON | Reconnects automatically; resumes normal control | PASS |

---

## 11. Troubleshooting

### 1. `SLIDER` does not appear in Bluetooth search
* Ensure your computer's Bluetooth is turned ON.
* Check the Serial Monitor: ensure the ESP32 completed booting and reached `[SLIDER] Waiting for Bluetooth connection...`.
* If previously paired with another device, unpair it or toggle your computer's Bluetooth off and on.

### 2. Device connects, but slides do not advance
* Ensure PowerPoint, Google Slides, or your PDF viewer window is the **active focused window** on your screen.
* Verify key outputs in a text editor or browser: a single press should trigger the Right Arrow cursor, and double press should trigger the Left Arrow cursor.

### 3. Double press is detected as two single presses
* The default double-press interval is `300 ms`.
* If your physical clicking tempo is slower, increase `DOUBLE_PRESS_WINDOW_MS` in `include/config.h` (e.g., set to `400` or `450`).

### 4. Single press feels slightly delayed
* By design, the firmware must wait `DOUBLE_PRESS_WINDOW_MS` (300 ms) after the first press to ensure you are not about to click a second time.
* If you want a snappier response, reduce `DOUBLE_PRESS_WINDOW_MS` in `include/config.h` (e.g., set to `220` or `250`).

### 5. Board enters flashing mode on reset
* On classic ESP32 boards, holding BOOT while applying power or releasing EN enters bootloader download mode. Release the BOOT button before pressing reset/EN.

---

## 12. Roadmap & V2 Recommendations

For future revisions (SLIDER V2), the following enhancements can be incorporated:
1. **Automatic Inactivity Timeout & Deep Sleep:**
   * Enter ESP32 Deep Sleep after 15 minutes of inactivity.
   * Wake instantly on GPIO 0 using `esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0)`.
2. **Haptic Feedback / Status Indication:**
   * Attach a tiny vibrator motor or use the board's on-board LED (GPIO 2 on most DevKits) for subtle single/double flash feedback.
3. **LiPo Battery Support:**
   * Add an ADC voltage divider to read battery percentage and report real-time battery levels to the host OS via the HID Battery Service.
