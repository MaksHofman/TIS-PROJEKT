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
    setupColorSensor();
    calibrateColorSensor();

    // sygnalizacja
    SETUP_LED;

    // Czyścimy bufory
    clearRx(); clearTx();
    SETUP_LORA;

    // Zaczynamy od nasłuchu
    LoRa.receive();
}

void loop() {
    // Mruganko
    blink();

    switch (currentState) {
        case STATE_IDLE:
            if ((currentPacketSize = receive())) {
                // odebraliśmy wiadomość, trzeba to ogarnąć
                lastReceiveTime = millis();
                currentState = STATE_PROCESS_PACKET;
            } else if (millis() - lastReadTime >= READ_TIMEOUT) {
                // dawno nie wysyłaliśmy wiadomości, trzeba to zrobić
                prepRead();
                lastReadTime = millis();
                currentState = STATE_TRANSMIT;
            }
            break;
        case STATE_PROCESS_PACKET: {
            // Sprawdzamy co nam wpadło
            uint8_t isValid = validateMsg(rxBuff, currentPacketSize);
            if (isValid == MSG_STATE_TOO_SHORT) {
#ifdef DEBUG
                BEGIN_DEBUG;
                Serial.print(F("Otrzymano za krotka wiadomosc, dlugosc: "));
                Serial.println(currentPacketSize);
                END_DEBUG;
#endif
                currentState = STATE_IDLE;
                break;
            } else if (isValid == MSG_STATE_INVALID) {
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
            add_blinks(BLINKS_RECEIVE);
            prepMeasResp(currentPacketSize, lastReceiveTime);

            currentState = STATE_TRANSMIT;

            break;
        }

        case STATE_TRANSMIT:
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
