#include <Arduino.h>
#include <LoRa.h>
#include "!common/config.h"
#include "!common/lora_init.h"
#include "!common/proto_helper.h"
#include "!common/proto.h"
#include "!common/led_helper.h"
#include "!common/lora.h"
#include "!common/state.h"
#include "aggregator/node_activity.h"
#include "aggregator/identity.h"

#define DEBUG

// Bufor nadawczy i odbiorczy
byte rxBuff[BUFF_SIZE], txBuff[BUFF_SIZE];
int currentPacketSize = 0;

NodeState currentState = STATE_IDLE;

// Aktywne węzły
unsigned long lastActivityPrint = 0;
uint8_t activeNodes = 0;
unsigned long lastSeen[8];

// time slot sync
unsigned long lastTimeSlotSync = 0;

// A przechowajmy sobie timestamp1 z 10 wiadomości,
// żeby printować tylko tą pierwszą i sprawdzać czas propagacji tylko tej pierwszej
unsigned long seenMsgs[MSGS_TO_REMEMBER];

void setup() {
    Serial.begin(115200);

    clearRx(); clearTx();
    SETUP_LORA;

    LoRa.receive(); // Zaczynamy od nasłuchu
    SETUP_LED;

    memset(lastSeen, 0, sizeof(unsigned long) * 8);
    memset(seenMsgs, 0, sizeof(unsigned long) * 10);
}

void loop() {
    // Mruganko
    blink();

    // Wyświetlenie aktywnych węzłów
    if (millis() - lastActivityPrint < ACTIVITY_TIMEOUT)
        goto skipActivity;

    lastActivityPrint = millis();
    Serial.println(F("########## POCZATEK AKTYWNE WEZLY ##########"));
    if (activeNodes == 0)
        Serial.println(F("Brak aktywnych wezlow, czy w sieci dziala co najmniej 1 sensor?"));
    else {
        for (int i = 1; i <= 8; i++) {
            // Nieaktywne same się nie odświerzą, trzeba to zrobić gdzieś tutaj
            if (millis() - lastSeen[i - 1] >= ACTIVITY_TIMEOUT)
                SET_ACTIVE(activeNodes, i, 0);

            Serial.print(F("Wezel W"));
            Serial.print(i, DEC);
            if (GET_ACTIVE(activeNodes, i))
                Serial.println(F(" jest aktywny."));
            else
                Serial.println(F(" jest nieaktywny."));
        }
    }
    Serial.println(F("########## KONIEC AKTYWNE WEZLY ##########"));

    skipActivity:
    switch (currentState) {
        case STATE_IDLE: {
            if (millis() - lastTimeSlotSync > EPOCH_TIME) {
                clearTx();
                SET_SRC(txBuff, W0_ID);
                SET_TYPE(txBuff, MSG_T_READ);
                SET_ROUTING(txBuff, 0);
                LoRa.beginPacket();
                LoRa.write(txBuff, 1);
                LoRa.endPacket();
                LoRa.receive();
                lastTimeSlotSync = millis();
            }

            // odebraliśmy wiadomość, trzeba to ogarnąć
            if ((currentPacketSize = receive()))
                currentState = STATE_PROCESS_PACKET;

            // Serial.print(F("\b\b\b\b\b"));
            break;
        }
        case STATE_PROCESS_PACKET: {
            // receive() już odczytał dane do rxBuff i wyczyścił bufor

            uint8_t isValid = validateMsg(rxBuff, currentPacketSize);
            // Standardowa walidacja, czy to w ogóle nasze
            if (isValid == 1) {
#ifdef DEBUG
                BEGIN_DEBUG;
                Serial.print(F("Otrzymano za krotka wiadomosc, dlugosc: "));
                Serial.println(currentPacketSize);
                END_DEBUG;
#endif
                currentState = STATE_IDLE;
                break;
            } else if (isValid == 2) {
#ifdef DEBUG
                BEGIN_DEBUG;
                Serial.println(F("Otrzymano wiadomosc o nieznanym formacie"));
                END_DEBUG;
#endif
                currentState = STATE_IDLE;
                break;
            }

            uint8_t src = GET_SRC(rxBuff);
            // Podsluchalismy komunikacje od nas
            if (!IS_SENSOR(src)) {
                currentState = STATE_IDLE;
#ifdef DEBUG
                BEGIN_DEBUG;
                Serial.print(F("Otrzymano wiadomosc, ale nie od sensora"));
                printPath(currentPacketSize);
                END_DEBUG;
#endif
                break;
            }
            // Koniec standardowej walidacji
            // Na tym etapie można zamrugać diodą
            add_blinks(BLINKS_RECEIVE);

            // Wiadomość to pomiar, więc wyświetlamy wszystko
            if (GET_TYPE(rxBuff) == MSG_T_MEAS_RTT) {
                Serial.println(F("########## POCZATEK PELNEGO POMIARU ##########"));
                Serial.print(F("Otrzymano pomiar z S"));
                Serial.print(SENSOR_ID(src), DEC);
                printPath(currentPacketSize);
                Serial.println(F(":"));

                Serial.print(F("R = "));
                Serial.print(GET_R(rxBuff), DEC);
                Serial.print(F(", G = "));
                Serial.print(GET_G(rxBuff), DEC);
                Serial.print(F(", B = "));
                Serial.println(GET_B(rxBuff), DEC);

                Serial.print(F("Czas propagacji (S"));
                Serial.print(SENSOR_ID(src), DEC);
                Serial.print(F(" - W"));
                Serial.print(MY_ID, DEC);
                Serial.print(F("): "));
                Serial.print((GET_TIMESTAMP2(rxBuff) - GET_TIMESTAMP1(rxBuff)) >> 1, DEC);
                Serial.println(F(" ms."));
                Serial.println(F("########## KONIEC PELNEGO POMIARU ##########"));
                currentState = STATE_IDLE;
                break;
            }

            // Jeśli to read to:
            // 1. zapisujemy timestamp1, żeby olać pozostałe
            // 2. robimy DEBUG PRINT
            if (haveISeenIt()) {
#ifdef DEBUG
                BEGIN_DEBUG;
                Serial.print(F("Juz widzielismy: "));
                Serial.print(GET_TIMESTAMP1(rxBuff));
                printPath(currentPacketSize);
                Serial.println();
                END_DEBUG;
#endif
                currentState = STATE_IDLE;
                break;
            }


#ifdef DEBUG
            BEGIN_DEBUG;
            Serial.print(F("Otrzymano pomiar z S"));
            Serial.print(SENSOR_ID(src), DEC);
            printPath(currentPacketSize);
            Serial.println(F(":"));

            Serial.print(F("R = "));
            Serial.print(GET_R(rxBuff), DEC);
            Serial.print(F(", G = "));
            Serial.print(GET_G(rxBuff), DEC);
            Serial.print(F(", B = "));
            Serial.println(GET_B(rxBuff), DEC);
            END_DEBUG;
#endif
            if(!GET_NHOP(rxBuff)) {
                // i teraz otrzymaliśmy READ, więc trzeba odesłać do pomiaru czasu tą samą ścieżką
                currentState = STATE_IDLE;
                break;
            }
        prepMeasResp(currentPacketSize);
#ifdef IGNORE_MEAS
            currentState = STATE_IDLE;
            break;
#endif
            seeItNow();
            currentState = STATE_TRANSMIT;
            break;
        }
        case STATE_TRANSMIT: {
#ifdef DEBUG
            BEGIN_DEBUG;
            Serial.println(F("Nadawanie..."));
            END_DEBUG;
#endif
            // send() zajmuje się LoRa.idle(), time-slotem, nadaniem i LoRa.receive()
            // dodajemy przed samym nadaniem pomiaru timestamp1
            if (GET_TYPE(txBuff) == MSG_T_READ)
                SET_TIMESTAMP1(txBuff, millis());

            send(txBuff, GET_LEN(txBuff), MY_ID);
            clearTx();

            currentState = STATE_IDLE;
            break;
        }
    }
}
