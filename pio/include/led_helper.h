#pragma once

#include <Arduino.h>
#include <cstdint>

typedef enum {
    IDLE, SLEEP, RECEIVING, RETRANSMITTING, TRANSMITTING, GENERIC_ERROR, NONE
} STATE;

#define LED_PIN_R 7
#define LED_PIN_G 8
#define LED_PIN_B 9

void setupStateLed();
void setLedColor(uint8_t red, uint8_t green, uint8_t blue);
void setLedState(STATE state);
uint8_t calcCRC8(byte* buff, uint8_t len);

/* --- DODANE DLA TIMER'A DIOD --- */
extern unsigned long _led_timer;
extern unsigned long _led_duration;
extern STATE _current_temp_state;

inline void triggerLedState(STATE state, unsigned long duration = 200) {
    setLedState(state);
    _current_temp_state = state;
    _led_timer = millis();
    _led_duration = duration;
}

inline void updateLedTask() {
    if (_current_temp_state != IDLE && _current_temp_state != SLEEP && _current_temp_state != NONE) {
        if (millis() - _led_timer > _led_duration) {
            setLedState(IDLE);
            _current_temp_state = IDLE;
        }
    }
}
