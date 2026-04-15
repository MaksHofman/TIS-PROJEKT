#include <Arduino.h>
#include <Wire.h>

// ================== KONFIG ==================
#define DEVICE_ADDR 0x10   // ZMIENIASZ dla każdego czujnika
// ===========================================

volatile bool requestFlag = false;

// Funkcja wywoływana gdy master chce dane
void onRequest() {
  // przykładowe dane
  uint8_t data[4];

  data[0] = DEVICE_ADDR;       // ID
  data[1] = random(0, 100);    // fake pomiar
  data[2] = millis() & 0xFF;   // czas (LSB)
  data[3] = 0xAA;              // marker

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

  Wire.begin(DEVICE_ADDR); // SLAVE
  Wire.onRequest(onRequest);
  Wire.onReceive(onReceive);

  Serial.print("SLAVE START addr=0x");
  Serial.println(DEVICE_ADDR, HEX);
}

void loop() {
  delay(100);
}