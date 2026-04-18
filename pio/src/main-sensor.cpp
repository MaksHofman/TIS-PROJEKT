#include <Arduino.h>
#include <Wire.h>
#include "color_helper.h"
#include "const_defs.h"
/* #TODO: Przerobić to na coś co korzysta z protokołu i UART'a (trzeba jeszcze przekonwertować odczyty z 8 na 4 bity) */
// ================== KONFIG ==================
#define DEVICE_ADDR 0x10   // ZMIENIASZ dla każdego czujnika
// ===========================================

uint8_t maxVals[5] = {0};

volatile bool requestFlag = false;

// Funkcja wywoływana gdy master chce dane
void onRequest() {
  uint8_t data[4];

  data[0] = DEVICE_ADDR;
  data[1] = readColor(COLOR_RED);   // TODO: take more then RED
  data[2] = millis() & 0xFF;
  data[3] = 0xAA;

  Wire.write(data, 4);
}

// Opcjonalnie: odbiór danych od mastera
void onReceive(int len) {
  while (Wire.available()) {
    uint8_t cmd = Wire.read();
    Serial.print("CMD: ");
    Serial.println(cmd);
  }
}

void setup() {
  Serial.begin(115200);

  // TODO: move to color sensor setup function
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(OUT_PIN, INPUT);
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);  // skala 20%

  Wire.begin(DEVICE_ADDR); // SLAVE
  Wire.onRequest(onRequest);
  Wire.onReceive(onReceive);

  Serial.print("SLAVE START addr=0x");
  Serial.println(DEVICE_ADDR, HEX);
}

void loop() {
  delay(100);
}
