#include <Arduino.h>
#include <LoRa.h>
#include "!common/config.h"
#include "!common/lora_init.h"

void setup() {
    Serial.begin(115200);
    while (!Serial);

    Serial.println("LoRa Receiver");

    SETUP_LORA;
}

void loop() {
    // try to parse packet
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
        // received a packet
        Serial.print("Received packet '");

        // read packet
        while (LoRa.available()) {
            Serial.print((char)LoRa.read());
        }

        // print RSSI of packet
        Serial.print("' with RSSI: ");
        Serial.print(LoRa.packetRssi());
        Serial.print(", and SNR: ");
        Serial.println(LoRa.packetSnr());
    }
}
