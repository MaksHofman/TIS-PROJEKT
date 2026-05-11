#include <Arduino.h>
#include "!common/config.h"
#include "sensor/color_helper.h"
#include "sensor/rgb_led_helper.h"

uint8_t maxVals[5];

uint8_t readColor(uint8_t color) {

    switch (color) {
        case COLOR_RED:
        digitalWrite(S2, LOW);
        digitalWrite(S3, LOW);
        break;
        case COLOR_GREEN:
        digitalWrite(S2, HIGH);
        digitalWrite(S3, HIGH);
        break;
        case COLOR_BLUE:
        digitalWrite(S2, LOW);
        digitalWrite(S3, HIGH);
        break;
        case COLOR_CLEAR:
        digitalWrite(S2, HIGH);
        digitalWrite(S3, LOW);
        break;
        default: return 0;
    }

    uint8_t mapped = map(pulseIn(OUT_PIN, LOW), 0, (unsigned long) (0UL - 1UL), 0, 255);

    return maxVals[color] ? (((255 - mapped) * 255) / maxVals[color]) : (255 - mapped);
}

void calibrateColor(uint8_t color) {
    // show user the color to calibrate
    setRgbLedColor(color);

    // give user some time to react
    delay(2000);

    // read and save
    maxVals[color] = readColor(color);

    // set led back
    setRgbLedColor(COLOR_NONE);
}

void calibrateColorSensor() {
    calibrateColor(COLOR_RED);
    calibrateColor(COLOR_GREEN);
    calibrateColor(COLOR_BLUE);
    calibrateColor(COLOR_CLEAR);
}
