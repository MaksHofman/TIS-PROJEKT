#include <Arduino.h>
#include <LoRa.h>
#include "!common/config.h"
#include "sensor/color_helper.h"

// Coś do kolorów
uint8_t maxVals[5] = {0};

void setup() {
    // Sensor kolorów
    pinMode(S0, OUTPUT);
    pinMode(S1, OUTPUT);
    pinMode(S2, OUTPUT);
    pinMode(S3, OUTPUT);
    pinMode(OUT_PIN, INPUT);
    digitalWrite(S0, HIGH);
    digitalWrite(S1, LOW);
    // TODO: Dodać kalibrację, najlepiej z LED'em RGB

    // Koniec sensora kolorów

}

void loop() {
    
}