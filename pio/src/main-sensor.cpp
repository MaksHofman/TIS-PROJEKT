/*
  Projekt TIS E1
  Czujnik koloru TCS3200
  Platforma: Arduino MKR WAN 1310
*/
#include <Arduino.h>
#include "color_helper.h"
#include "const_defs.h"

// Read values
uint8_t vals[4] = { };

// calibration values
uint8_t maxVals[5] = { };

inline void calibrate() {
    Serial.println("cal red:");
    delay(CAL_DELAY);
    maxVals[RED] = readColor(RED);

    Serial.println("cal green:");
    delay(CAL_DELAY);
    maxVals[GREEN] = readColor(GREEN);

    Serial.println("cal blue:");
    delay(CAL_DELAY);
    maxVals[BLUE] = readColor(BLUE);

    Serial.println("cal white:");
    delay(CAL_DELAY);
    maxVals[CLEAR] = readColor(CLEAR);
}

inline void printColor() {
    if (vals[RED] == maxVals[TOTAL])
        Serial.println("RED");
    else if (vals[GREEN] == maxVals[TOTAL])
        Serial.println("GREEN");
    else if (vals[BLUE] == maxVals[TOTAL])
        Serial.println("BLUE");
    else
        Serial.println("WHITE");
}

void setup()
{
    Serial.begin(115200);

    pinMode(S0, OUTPUT);
    pinMode(S1, OUTPUT);
    pinMode(S2, OUTPUT);
    pinMode(S3, OUTPUT);
    pinMode(OUT_PIN, INPUT);    

    // ustawienie skalowania częstotliwości: L L - Power down, L H - 2%, H L - 20%, H H - 100%
    digitalWrite(S0, HIGH);
    digitalWrite(S1, LOW);

    calibrate();
}

void loop() {
    vals[RED] = readColor(RED);
    vals[GREEN] = readColor(GREEN);
    vals[BLUE] = readColor(BLUE);
    vals[CLEAR] = readColor(CLEAR);

    maxVals[TOTAL] = max(max(max(vals[RED], vals[GREEN]), vals[BLUE]), vals[CLEAR]);

    Serial.print("DBG: (");
    Serial.print(vals[RED]/maxVals[TOTAL]);

    Serial.print(", ");
    Serial.print(vals[GREEN]/maxVals[TOTAL]);

    Serial.print(", ");
    Serial.print(vals[BLUE]/maxVals[TOTAL]);

    Serial.print(", ");
    Serial.print(vals[CLEAR]/maxVals[TOTAL]);
    Serial.println(")");

    printColor();
    delay(1000);
}
