#include "led_helper.h"

void setupStateLed() {
    pinMode(LED_PIN_R, OUTPUT);
    pinMode(LED_PIN_G, OUTPUT);
    pinMode(LED_PIN_B, OUTPUT);
}

void setLedColor(uint8_t red, uint8_t green, uint8_t blue) {
    analogWrite(LED_PIN_R, red);
    analogWrite(LED_PIN_G, green);
    analogWrite(LED_PIN_B, blue);
}

void setLedState(STATE state) {
    switch (state) {
        case IDLE: {
            // white
            setLedColor(255, 255, 255);
        }
        case SLEEP: {
            // dim white - call it gray
            setLedColor(10, 10, 10);
        }
        case RECEIVING: {
            // blue
            setLedColor(0, 0, 255);
        }
        case RETRANSMITTING: {
            // green (cause blue+yellow)
            setLedColor(0, 255, 0);
        }
        case TRANSMITTING: {
            // yellow
            setLedColor(255, 0, 255);
        }
        case GENERIC_ERROR: {
            // red
            setLedColor(255, 0, 0);
        }

        break;
    }
}
