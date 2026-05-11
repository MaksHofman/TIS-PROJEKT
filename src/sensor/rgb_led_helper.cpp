#include <Arduino.h>
#include "sensor/rgb_led_helper.h"
#include "sensor/color_helper.h"

// it's not the best solution, but should make it easier
// to use led, since setup will be run on demand
bool rgbSetupDone = false;

void setupRgbLed() {
    // run setup conditionally
    if (rgbSetupDone)
        return;

    pinMode(LED_PIN_R, OUTPUT);
    pinMode(LED_PIN_G, OUTPUT);
    pinMode(LED_PIN_B, OUTPUT);

    // mark setup done
    rgbSetupDone = true;
}

void setRgbLedColor(uint8_t color) {
    setupRgbLed();

    switch (color) {
        case COLOR_RED:
        digitalWrite(LED_PIN_R, HIGH);
        digitalWrite(LED_PIN_G, LOW);
        digitalWrite(LED_PIN_B, LOW);
        break;
        case COLOR_GREEN:
        digitalWrite(LED_PIN_R, LOW);
        digitalWrite(LED_PIN_G, HIGH);
        digitalWrite(LED_PIN_B, LOW);
        break;
        case COLOR_BLUE:
        digitalWrite(LED_PIN_R, LOW);
        digitalWrite(LED_PIN_G, LOW);
        digitalWrite(LED_PIN_B, HIGH);
        break;
        case COLOR_CLEAR:
        digitalWrite(LED_PIN_R, HIGH);
        digitalWrite(LED_PIN_G, HIGH);
        digitalWrite(LED_PIN_B, HIGH);
        break;
        case COLOR_NONE:
        digitalWrite(LED_PIN_R, LOW);
        digitalWrite(LED_PIN_G, LOW);
        digitalWrite(LED_PIN_B, LOW);
        break;
    }
}
