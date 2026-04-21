#pragma once

#include <Arduino.h>
#include <cstdint>

typedef enum {
    IDLE,
    SLEEP,
    RECEIVING,
    RETRANSMITTING,
    TRANSMITTING,
    GENERIC_ERROR,
    NONE
} STATE;

#define LED_PIN_R 7
#define LED_PIN_G 8
#define LED_PIN_B 9

void setupStateLed();

void setLedColor(uint8_t red, uint8_t green, uint8_t blue);

void setLedState(STATE state);

uint8_t calcCRC8(byte* buff, uint8_t len);
