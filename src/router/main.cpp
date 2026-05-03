#include <Arduino.h>
#include <LoRa.h>
#include "!common/config.h"
#include "!common/lora_init.h"
#include "router/main.h"
#include "!common/proto.h"
#include "router/helper.h"
#include "api/Common.h"

#define MY_ID W1_ID

byte rxBuff[BUFF_SIZE], txBuff[BUFF_SIZE];

void setup() {
    Serial.begin(115200);

#ifdef DEBUG
    while(!Serial);
    Serial.println(F("Inicjalizacja węzła router"));
#endif

    clearTx();
    clearRx();

    SETUP_LORA;
    LoRa.onCadDone(onCadDone);
    LoRa.onReceive(onRxDone);
    LoRa.receive();
}

void loop() {
    
}

void onCadDone(boolean signal) {
    // Jest sygnał, więc musimy czekać na czyste powietrze
    if (signal) {
        // losowe opóźnienie (w ISR xD)
        delay(random(10, 100));
        LoRa.channelActivityDetection();
        return;
    }

    // wysyłamy co mamy
    LoRa.beginPacket();
    LoRa.write(txBuff, GET_LEN(txBuff));
    LoRa.endPacket();

}

void onRxDone(int packetSize) {
#ifdef DEBUG
    Serial.print(F("Otrzymano pakiet o rozmiarze: "));
    Serial.println(packetSize, DEC);
#endif
// Tutaj można dać pojedyncze mrugnięcie LEDem, że coś dostaliśmy, tylko proszę użyć Timer1, żeby nie robić bezsensownego delay'a
    // Czyścimy bufor
    clearRx();

    // Odczytujemy co dostaliśmy
    LoRa.readBytes(rxBuff, packetSize);

    // Za krótka wiadomość lub nie zgadzają się pola zarezerwowane + SYS_ID, nie nasza
    // Tu też można jakieś statusy dawać LEDem
    switch (validateMsg(rxBuff, packetSize)) {
        case 1:
#ifdef DEBUG
        Serial.println(F("Za krótka wiadomość"));
#endif
        break;
        case 2:
#ifdef DEBUG
        Serial.println(F("Nieznany protokół"));
#endif
        break;
        case 0: goto processReceived; // Tutaj jesteśmy jak wszystko jest git do tej pory
        default: break; // Tu się nigdy nie znajdziemy...
    }

    clearAndReturn:
    clearRx();
    return;

    processReceived:
    if(GET_ROUTING(rxBuff)) {
        // Najpierw routing kierowany:
        if(GET_HOP_NODE_ID(rxBuff, GET_NHOP(rxBuff)) != MY_ID) {
#ifdef DEBUG
            Serial.print(F("Otrzymano wiadomość z routingiem kierowanym, ale nie jestem następnym węzłem"));
            clearRx();
            return;
#endif
        } else {
#ifdef DEBUG
            Serial.println(F("Otrzymano wiadomość z routingiem kierowanym przez nas"));
#endif
            // Routing kierowany przez nas, więc zmniejszamy o 1 nhop i przesyłamy dalej
            memcpy(txBuff, rxBuff, packetSize);
            SET_NHOP(txBuff, (GET_NHOP(txBuff) - 1));
            goto beginCAD;
        }

    } else {
        // routing rozsiewczy
        // sprawdzenie, czy już nie byliśmy na drodze tego pakietu i czy przypadkiem nie krąży on już zbyt długo
        if (GET_NHOP(rxBuff) >= MAX_NHOPS) {
#ifdef DEBUG
            Serial.println(F("Zbyt duża ilość przeskoków, nie przesyłam dalej"));
#endif
            goto clearAndReturn;
        }
        for (int i = 1; i <= GET_NHOP(rxBuff); i++) {
            if (GET_HOP_NODE_ID(rxBuff, i) == MY_ID) {
#ifdef DEBUG
                Serial.print(F("Już to przesyłałem, zapobiegamy pętli"));
#endif
                goto clearAndReturn;
            }
        }

        memcpy(txBuff, rxBuff, packetSize);
        // jesteśmy kolejnym przeskokiem
        SET_NHOP(txBuff, (GET_NHOP(txBuff) + 1));
        // dodanie swojego ID do listy
        SET_HOP(txBuff, GET_NHOP(txBuff), MY_ID);
        // poprawienie SYS_CODE
        SET_SYS_CODE(txBuff, SYS_CODE);
    }


    // Tu będzie kopiowanie bufora rx do tx i początek cad
    beginCAD:
    LoRa.channelActivityDetection();

}