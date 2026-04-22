#pragma once
#include <Arduino.h>

// Piny LED RGB (dopasuj do swojej płytki MKR WAN 1310)
#ifndef LED_PIN_R
#define LED_PIN_R  A3
#endif
#ifndef LED_PIN_G
#define LED_PIN_G  A4
#endif
#ifndef LED_PIN_B
#define LED_PIN_B  A5
#endif

typedef enum {
    IDLE = 0,
    SLEEP,
    RECEIVING,
    TRANSMITTING,
    RETRANSMITTING,
    GENERIC_ERROR,
    NONE
} STATE;

/** Ustaw stały kolor LED (nie wraca automatycznie). */
void setupStateLed();
void setLedState(STATE state);

/** Ustaw kolor na ~500 ms, potem automatycznie wróć do IDLE. */
void triggerLedState(STATE state);

/** Wywołuj w każdej iteracji loop() aby obsługiwać powrót po triggerLedState. */
void updateLedTask();

/** Hamming checksum - zgodny interfejs z calcCRC8 */
uint8_t calcCRC8(byte* buf, uint8_t len);
