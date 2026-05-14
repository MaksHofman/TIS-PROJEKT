#include <Arduino.h>
#include <LoRa.h>
#include "!common/config.h"
#include "!common/lora_init.h"
#include "!common/proto.h"
#include "!common/proto_helper.h"
#include "!common/led_helper.h"
#include "!common/lora.h"
#include "!common/state.h"

#define MY_ID W1_ID

byte rxBuff[BUFF_SIZE], txBuff[BUFF_SIZE];
int currentPacketSize = 0;

NodeState currentState = STATE_IDLE;

void setup() {
#ifdef DEBUG
    Serial.begin(115200);
    Serial.println(F("Inicjalizacja węzła router"));
#endif

    clearRx(); clearTx();
    SETUP_LORA;

    LoRa.receive(); // Zaczynamy od nasłuchu
    SETUP_LED;
}

void loop() {
    blink();
    // Serce programu - maszyna stanów
    switch (currentState) {

        case STATE_IDLE:
            if (flagPacketReceived) {
                flagPacketReceived = false;
                currentState = STATE_PROCESS_PACKET;
            }
            break;

        case STATE_PROCESS_PACKET: {
            // 1. Odczyt danych z radia po SPI
            clearRx();
            LoRa.readBytes(rxBuff, currentPacketSize);

            // 2. Walidacja z "Early Exit" (zamiast zagnieżdżania)
            int valStatus = validateMsg(rxBuff, currentPacketSize);
            if (valStatus != 0) {
#ifdef DEBUG
                BEGIN_DEBUG;
                Serial.println(valStatus == 1 ? F("Za krotka wiadomosc") : F("Nieznany protokol"));
                END_DEBUG;
#endif
                currentState = STATE_IDLE;
                break; // Kończymy ten case natychmiast!
            }

            // 3. ROUTING KIEROWANY
            if (GET_ROUTING(rxBuff)) {
                if(GET_HOP_NODE_ID(rxBuff, GET_NHOP(rxBuff)) != MY_ID) {
#ifdef DEBUG
                    BEGIN_DEBUG;
                    Serial.println(F("Kierowane: ale nie jestem nastepnym wezlem. Ignoruje."));
                    END_DEBUG;
#endif
                    currentState = STATE_IDLE;
                    break;
                }
                
                // Przepisanie pakietu, bo idzie przez nas
#ifdef DEBUG
                BEGIN_DEBUG;
                Serial.println(F("Kierowane: przez nas. Przesylam dalej."));
                END_DEBUG;
#endif
                add_blinks(1);
                memcpy(txBuff, rxBuff, currentPacketSize);
                SET_NHOP(txBuff, (GET_NHOP(txBuff) - 1));

                currentState = STATE_TRANSMIT;
                break;
            } 
            
            // 4. ROUTING ROZSIEWCZY (jeśli kod dotarł tu, to na pewno nie jest kierowany)
            if (GET_NHOP(rxBuff) >= MAX_NHOPS) {
#ifdef DEBUG
                BEGIN_DEBUG;
                Serial.println(F("Zbyt duza ilosc przeskokow (TTL). Odrzucam."));
                END_DEBUG;
#endif
                currentState = STATE_IDLE;
                break;
            }

            // Sprawdzanie pętli
            bool alreadySeen = false;
            for (int i = 1; i <= GET_NHOP(rxBuff); i++) {
                if (GET_HOP_NODE_ID(rxBuff, i) == MY_ID) {
                    alreadySeen = true;
                    break;
                }
            }

            if (alreadySeen) {
#ifdef DEBUG
                BEGIN_DEBUG;
                Serial.println(F("Rozsiewcze: Juz to przesylalem, zapobiegam petli."));
                END_DEBUG;
#endif
                currentState = STATE_IDLE;
                break;
            } 

            // Jeśli przeszedł wszystkie testy – nadajemy
            add_blinks(1);
            memcpy(txBuff, rxBuff, currentPacketSize);
            SET_NHOP(txBuff, (GET_NHOP(txBuff) + 1));
            SET_HOP(txBuff, GET_NHOP(txBuff), MY_ID);
            SET_SYS_CODE(txBuff, currentPacketSize, SYS_CODE);

            currentState = STATE_TRANSMIT;
            break;
        }

        case STATE_TRANSMIT:
#ifdef DEBUG
            BEGIN_DEBUG;
            Serial.println(F("Eter wolny. Nadawanie..."));
            END_DEBUG;
#endif
            // WARNING: nie wiem, czy przypadkiem tu nie trzeba dać LoRa.idle();
            LoRa.beginPacket();
            LoRa.write(txBuff, currentPacketSize);
            LoRa.endPacket();
            // ^ to endPacket() jest blokujące, więc teraz na spokojnie można wyczyścić bufor nadawczy
            clearTx();
            
            // Nasłuchiwanie
            LoRa.receive();
            add_blinks(2);
            currentState = STATE_IDLE;
            break;
    }
}