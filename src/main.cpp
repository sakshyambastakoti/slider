#include <Arduino.h>
#include <BleKeyboard.h>

BleKeyboard bleKeyboard("SLIDER", "Espressif", 100);

void setup() {
    Serial.begin(115200);
    bleKeyboard.begin();
}

void loop() {
    if (bleKeyboard.isConnected()) {
        // ready
    }
}
