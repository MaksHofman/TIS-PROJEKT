#pragma once

#include <Arduino.h>

typedef enum {
    IDLE,
    SLEEP,
    RECEIVING,
    RETRANSMITTING,
    TRANSMITTING,
    GENERIC_ERROR
} STATE;

#define LED_PIN_R 8
#define LED_PIN_G 7
#define LED_PIN_B 6

void setupStateLed();

void setLedColor(uint8_t red, uint8_t green, uint8_t blue);

void setLedState(STATE state);
