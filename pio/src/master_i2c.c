#include <Arduino.h>
#include <Wire.h>

// ================== KONFIG ==================
#define I2C_FREQ 100000

// Lista adresów urządzeń (możesz zmienić)
#define DEV1_ADDR 0x10
#define DEV2_ADDR 0x11
#define DEV3_ADDR 0x12
#define DEV4_ADDR 0x13

uint8_t devices[] = {DEV1_ADDR, DEV2_ADDR, DEV3_ADDR, DEV4_ADDR};
const int DEVICE_COUNT = sizeof(devices);

// ===========================================

void scanI2C() {
  Serial.println("Skanowanie I2C...");

  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("Znaleziono urzadzenie: 0x");
      Serial.println(addr, HEX);
    }
  }

  Serial.println("Skanowanie zakonczone\n");
}

void requestData(uint8_t addr) {
  Serial.print("Odpytuje: 0x");
  Serial.println(addr, HEX);

  Wire.requestFrom(addr, (uint8_t)4); // prosimy o 4 bajty

  while (Wire.available()) {
    uint8_t b = Wire.read();
    Serial.print("0x");
    Serial.print(b, HEX);
    Serial.print(" ");
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Wire.begin(); // MASTER
  Wire.setClock(I2C_FREQ);

  Serial.println("MASTER START");

  scanI2C(); // wykrywanie urządzeń
}

void loop() {
  for (int i = 0; i < DEVICE_COUNT; i++) {
    requestData(devices[i]);
    delay(500);
  }

  Serial.println("-----");
  delay(2000);
}