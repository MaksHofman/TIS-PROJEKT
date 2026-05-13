#include <Arduino.h>
#include <LoRa.h>
#include "!common/config.h"
#include "!common/lora_init.h"

int counter = 0;

void setup() {
    
#ifdef DEBUG
    Serial.begin(115200);
    while (!Serial);
    Serial.println("LoRa Sender");
#endif

    SETUP_LORA;
}

void loop() {
    // wait until the radio is ready to send a packet
    while (LoRa.beginPacket() == 0) {
#ifdef DEBUG
        Serial.print("Waiting for radio ... ");
#endif
        delay(100);
    }
#ifdef DEBUG
    Serial.print("Sending packet: ");
    Serial.println(counter);
#endif

    // send packet
    LoRa.beginPacket();
    LoRa.print("hello ");
    LoRa.print(counter);
    // Można zrobić nieblokujące i sprawdzać czy skończyło wysyłać: LoRa.isTransmitting()
    LoRa.endPacket();

    counter++;

    delay(5000);
}
