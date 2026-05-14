#include "!common/led_helper.h"

uint8_t to_blink = 0;
unsigned long last_blink;

void setupLed() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
}

void add_blinks(uint8_t to_add) {
    to_blink += 2 * to_add;
}

void blink() {
    if (to_blink && millis() - last_blink >= LED_TIMOUT_MS) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        to_blink--;
        last_blink = millis();
    }
}
