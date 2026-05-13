#include <Arduino.h>
#include <LoRa.h>
#include "!common/config.h"
#include "!common/lora_init.h"
#include "!common/proto_helper.h"
#include "!common/proto.h"
#include "!common/led_helper.h"
#include "aggregator/node_activity.h"

#include "aggregator/identity.h"
#include "api/Common.h"

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
unsigned long lastReceiveTime = 0;

// Aktywne węzły
unsigned long lastActivityPrint = 0;
uint8_t activeNodes = 0;
unsigned long lastSeen[8];

// A przechowajmy sobie timestamp1 z 10 wiadomości,
// żeby printować tylko tą pierwszą i sprawdzać czas propagacji tylko tej pierwszej
unsigned long seenMsgs[MSGS_TO_REMEMBER];

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
            // odebraliśmy wiadomość, trzeba to ogarnąć
            if (flagPacketReceived) {
                lastReceiveTime = millis();
                flagPacketReceived = false;
                currentState = STATE_PROCESS_PACKET;
            }
            break;
        }
        case STATE_PROCESS_PACKET: {
            // 1. Odczyt danych z radia po SPI
            clearRx();
            LoRa.readBytes(rxBuff, currentPacketSize);

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
            add_blinks(1);

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
                END_DEBUG;
#endif
                currentState = STATE_IDLE;
                break;
            }
            seeItNow();

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

            // i teraz otrzymaliśmy READ, więc trzeba odesłać do pomiaru czasu tą samą ścieżką
            prepMeasResp(currentPacketSize);
#ifdef IGNORE_MEAS
            currentState = STATE_IDLE;
            break;
#endif
            waitDuration = LoRa.random();
            waitStartTime = millis();
            currentState = STATE_WAIT_RANDOM;
            break;
        }
        case STATE_DO_CAD: {
            // Zlecamy zbadanie kanału i natychmiast wychodzimy ze stanu
            flagCadDone = false;
            currentState = STATE_WAIT_CAD;
            LoRa.channelActivityDetection();
            break;
        }
        case STATE_WAIT_CAD: {
            // Jeszcze się nie skończyło CAD
            if (!flagCadDone)
                break;

            // Skończyło się CAD
            flagCadDone = false;

            // coś akurat nadaje
            if (flagCadSignalDetected) {
#ifdef DEBUG
                BEGIN_DEBUG;
                Serial.println(F("Eter zajety. Czekam losowy czas..."));
                END_DEBUG;
#endif
                waitDuration = LoRa.random();
                waitStartTime = millis();
                currentState = STATE_WAIT_RANDOM;
                break;
            }

            // Nic nie nadaje, możemy nadawać
            currentState = STATE_TRANSMIT;
            break;
        }
        case STATE_WAIT_RANDOM: {
            if (millis() - waitStartTime >= waitDuration) {
#ifndef SKIP_CAD
                currentState = STATE_DO_CAD; // Po odczekaniu, spróbuj ponownie zbadać eter
#endif
#ifdef SKIP_CAD
                currentState = STATE_TRANSMIT;
#endif
            }
            break;
        }
        case STATE_TRANSMIT: {
#ifdef DEBUG
            BEGIN_DEBUG;
            Serial.println(F("Eter wolny. Nadawanie..."));
            END_DEBUG;
#endif
            // WARNING: nie wiem, czy przypadkiem tu nie trzeba dać LoRa.idle();
            // dodajemy przed samym nadaniem pomiaru timestamp1
            if (GET_TYPE(txBuff) == MSG_T_READ)
                SET_TIMESTAMP1(txBuff, millis());

            LoRa.beginPacket();
            LoRa.write(txBuff, GET_LEN(txBuff));    // tutaj długość powinna być poprawnie wyliczona niezależnie od typu
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
}