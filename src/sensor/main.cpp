#include <Arduino.h>
#include <LoRa.h>
#include "!common/config.h"
#include "!common/lora_init.h"
#include "!common/proto_helper.h"
#include "!common/proto.h"
#include "!common/led_helper.h"
#include "!common/lora.h"
#include "!common/state.h"
#include "sensor/identity.h"
#include "sensor/color_helper.h"
#include "sensor/msg_helper.h"

// Bufor nadawczy i odbiorczy
byte rxBuff[BUFF_SIZE], txBuff[BUFF_SIZE];
int currentPacketSize = 0;

NodeState currentState = STATE_IDLE;

// Zmienne do nieblokującego czekania
unsigned long lastReceiveTime = 0;

unsigned long lastReadTime = 0;

void setup() {
#ifdef DEBUG
    Serial.begin(115200);
#endif
    // Sensor kolorów
    pinMode(S0, OUTPUT);
    pinMode(S1, OUTPUT);
    pinMode(S2, OUTPUT);
    pinMode(S3, OUTPUT);
    pinMode(OUT_PIN, INPUT);
    digitalWrite(S0, HIGH);
    digitalWrite(S1, LOW);

    calibrateColorSensor();

    // Koniec sensora kolorów

    // Czyścimy bufory
    clearRx(); clearTx();
    SETUP_LORA;

    LoRa.receive(); // Zaczynamy od nasłuchu
    SETUP_LED;

}

void loop() {
    // Mruganko
    blink();

    switch (currentState) {
        case STATE_IDLE:
            // odebraliśmy wiadomość, trzeba to ogarnąć
            if (flagPacketReceived) {
                lastReceiveTime = millis();
                flagPacketReceived = false;
                currentState = STATE_PROCESS_PACKET;
            } else if (millis() - lastReadTime >= READ_TIMEOUT) {
                // Trzeba nadać pomiar
                prepRead();
                // Losowe opóźnienie przed CAD
                waitDuration = LoRa.random();
                waitStartTime = millis();
                currentState = STATE_WAIT_RANDOM;
            }
            break;
        case STATE_PROCESS_PACKET: {
            // 1. Odczyt danych z radia po SPI
            clearRx();
            LoRa.readBytes(rxBuff, currentPacketSize);

            // Sprawdzamy co nam wpadło
            uint8_t isValid = validateMsg(rxBuff, currentPacketSize);
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

            // ok, tu wiadomość już jest z naszego systemu, ale trzeba sprawdzić, czy musimy odpowiadać
            if(!doIHaveToRespond()) {
#ifdef DEBUG
                BEGIN_DEBUG;
                Serial.println(F("Wiadomosc nie jest dla sensora"));
                END_DEBUG;
#endif
                currentState = STATE_IDLE;
                break;
            }
            // Tutaj już wyczerpaliśmy wszystkie opcje i trzeba odpowiedzieć
            add_blinks(1);
            prepMeasResp(currentPacketSize, lastReceiveTime);

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
