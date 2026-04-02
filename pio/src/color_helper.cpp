#include <Arduino.h>
#include "const_defs.h"

extern uint8_t maxVals[5];

uint8_t readColor(uint8_t color) {

    switch (color) {
        case RED:
        digitalWrite(S2, LOW);
        digitalWrite(S3, LOW);
        break;
        case GREEN:
        digitalWrite(S2, HIGH);
        digitalWrite(S3, HIGH);
        break;
        case BLUE:
        digitalWrite(S2, LOW);
        digitalWrite(S3, HIGH);
        break;
        case CLEAR:
        digitalWrite(S2, HIGH);
        digitalWrite(S3, LOW);
        break;
        default: return 0;
    }

    return maxVals[color] ? (255 - map(pulseIn(OUT_PIN, LOW), 0, (unsigned long) (0UL - 1UL), 0, 255)) / maxVals[color] : 
    (255 - map(pulseIn(OUT_PIN, LOW), 0, (unsigned long) (0UL - 1UL), 0, 255));
}
