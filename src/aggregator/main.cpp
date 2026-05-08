#include <Arduino.h>
#include <LoRa.h>
#include <cstdint>
#include "!common/config.h"
#include "!common/lora_init.h"
#include "!common/proto_helper.h"
#include "!common/proto.h"
#include "!common/led_helper.h"
#include "aggregator/node_activity.h"

// Bufor nadawczy i odbiorczy
byte rxBuff[BUFF_SIZE], txBuff[BUFF_SIZE];
int currentPacketSize = 0;

// Flagi dla przerwań
volatile bool flagPacketReceived = false;
volatile bool flagCadDone = false;
volatile bool flagCadSignalDetected = false;

// Definicja stanów naszej maszyny
enum AggregatorState {
    STATE_IDLE,             // Nasłuchuje (nic się nie dzieje)
    STATE_PROCESS_PACKET,   // Analizuje to, co przyszło
    STATE_DO_CAD,           // Zleca skanowanie eteru (Listen Before Talk)
    STATE_WAIT_CAD,         // Czeka na wynik skanowania
    STATE_WAIT_RANDOM,      // Eter zajęty - czeka losowy czas
    STATE_TRANSMIT          // Eter wolny - nadaje
};

AggregatorState currentState = STATE_IDLE;

// Zmienne do nieblokującego czekania
unsigned long waitStartTime = 0;
uint8_t waitDuration = 0;

// Aktywne węzły
uint8_t activeNodes = 0;

void onRxDone(int packetSize) {
    currentPacketSize = packetSize;
    flagPacketReceived = true; // Zgłoś do loop(), że jest paczka
}

void onCadDone(boolean signal) {
    flagCadSignalDetected = signal;
    flagCadDone = true; // Zgłoś do loop(), że skanowanie zakończone
}

void setup() {
    Serial.begin(115200);

    clearRx(); clearTx();
    SETUP_LORA;

    LoRa.onCadDone(onCadDone);
    LoRa.onReceive(onRxDone);
    LoRa.receive(); // Zaczynamy od nasłuchu
    SETUP_LED;

}

void loop() {
    // Mruganko
    blink();

    switch (currentState) {
        case STATE_IDLE: {

            break;
        }
        case STATE_PROCESS_PACKET: {

            break;
        }
        case STATE_DO_CAD: {

            break;
        }
        case STATE_WAIT_CAD: {

            break;
        }
        case STATE_WAIT_RANDOM: {

            break;
        }
        case STATE_TRANSMIT: {

            break;
        }
    }
}