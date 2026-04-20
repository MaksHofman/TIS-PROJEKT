#include <Arduino.h>
#include <CRC.h>
#include <cstdint>
#include <sys/_intsup.h>
#include "api/Common.h"
#include "led_helper.h"
#include "uart_helper.h"
#include "const_defs.h"
#include "proto_defs.h"

#define MY_ID W0_ID

#define SERIAL1_NEI W3_ID
#define SERIAL4_NEI W4_ID

#define TIMEOUT_TOTAL 10000

byte Tx_buff[BUFF_LEN], Rx_buff[BUFF_LEN];
byte ret_buf[RET_LEN];
byte choice = '0';

uint8_t awaiting_res1, awaiting_res4 = 0;
unsigned long sent1, sent4 = 0;

void handleLHS(Uart &from, uint8_t from_id) {
    uint8_t recv = from.available();
    uint8_t crc_valid = 0;
    uint8_t id = 0;
    uint8_t r, g, b, maxVal;
    r = g = b = maxVal = 0;

    if (!(recv >= RET_LEN))
        return;

    setLedState(RECEIVING);

    from.readBytes(Rx_buff, min(BUFF_LEN, recv));

    crc_valid = GET_CRC(Rx_buff) == calcCRC8(Rx_buff, recv - 1);

    if (!crc_valid) {
        SET_DST(ret_buf, from_id);
        SET_CRC(ret_buf, calcCRC8(ret_buf, RET_LEN - 1));
        from.write(ret_buf, RET_LEN);
        return;
    }

    // Spodziewamy się RET, DEAD, RES
    switch (GET_TYP(Rx_buff)) {
        case MSG_T_RET:
        setLedState(RETRANSMITTING);
        from.write(Tx_buff, REQ_LEN);
        break;
        case MSG_T_DEAD:
        setLedState(GENERIC_ERROR);
        Serial.print(F("Wykryto awarię łącza pomiędzy "));
        switch (GET_SRC(Rx_buff)) {
            case W1_ID: Serial.print(F("W1 i ")); break;
            case W2_ID: Serial.print(F("W2 i ")); break;
            case W3_ID: Serial.print(F("W3 i ")); break;
            case W4_ID: Serial.print(F("W4 i ")); break;
            default: Serial.print(F("N/A"));
        }
        switch (GET_DEAD(Rx_buff)) {
            case W1_ID: Serial.print(F("W1.")); break;
            case W2_ID: Serial.print(F("W2.")); break;
            case S1_ID: Serial.print(F("S1.")); break;
            case S2_ID: Serial.print(F("S2.")); break;
            case S3_ID: Serial.print(F("S3.")); break;
            case S4_ID: Serial.print(F("S4.")); break;
            case S5_ID: Serial.print(F("S5.")); break;
            default: Serial.print(F("N/A."));
        }

        break;
        case MSG_T_RES:
        setLedState(RECEIVING);
        if (from_id == SERIAL1_NEI)
            awaiting_res1 = 0;
        else if (from_id == SERIAL4_NEI)
            awaiting_res4 = 0;
        // else
        //     Serial.println(F("Coś jest grubo nie tak"));

        id = GET_SRC(Rx_buff);
        switch (id) {
            case S1_ID:
            UNPACK_RGB(GET_READ(Rx_buff), r, g, b);
            Serial.print(F("Otrzymano odczyt z S1: R="));
            Serial.print(r, DEC);
            Serial.print(F(", G="));
            Serial.print(g, DEC);
            Serial.print(F(", B="));
            Serial.print(b, DEC);
            Serial.print(F(". Dominujący kolor: "));
            maxVal = max(r, max(g, b));
            if (maxVal == r)
                Serial.println(F("czerwony."));
            else if (maxVal == g)
                Serial.println(F("zielony."));
            else if (maxVal == b)
                Serial.println(F("niebieski."));
            else
                Serial.println(F("N/A."));

            break;
            default:
            Serial.print(F("Otrzymano ID "));
            Serial.print(id, DEC);
            Serial.print(F(" od węzła "));
            switch (id) {
                case S2_ID: Serial.print(F("S2.")); break;
                case S3_ID: Serial.print(F("S3.")); break;
                case S4_ID: Serial.print(F("S4.")); break;
                case S5_ID: Serial.print(F("S5.")); break;
                default: Serial.print(F("N/A."));
            }
        }

        break;
        default:
        Serial.print(F("Otrzymano nieznany typ wiadomości: 0x"));
        Serial.println(GET_TYP(Rx_buff), HEX);
    }
}

void setup() {
    Serial.begin(115200);

    Serial1.begin(UART_BAUD);
    Serial4.begin(UART_BAUD);

    setup_sercoms();
    memset((void*) Tx_buff, 0, BUFF_LEN);
    memset((void*) Rx_buff, 0, BUFF_LEN);
    memset((void*) ret_buf, 0, RET_LEN);
    SET_TYP(ret_buf, MSG_T_RET);
}

void loop() {
    setLedState(IDLE);

    // Dla pewności wyzerowanie wyboru
    choice = '0';
    unsigned long time1 = millis() - sent1,
    time4 = millis() - sent4;

    if ((awaiting_res1 || awaiting_res4) &&
        (time1 <= TIMEOUT_TOTAL ||
         time4 <= TIMEOUT_TOTAL ))
            goto await;

    // Upłynął timeout i nie mamy odpowiedzi
    if (awaiting_res1 && time1 > TIMEOUT_TOTAL) {
        Serial.print(F("Nie otrzymano odpowiedzi od W3 w ciągu "));
        Serial.print(TIMEOUT_TOTAL/1000, DEC);
        Serial.println(F(" sekund"));
        awaiting_res1 = 0;
        return;
    }

    if (awaiting_res4 && time4 > TIMEOUT_TOTAL) {
        Serial.print(F("Nie otrzymano odpowiedzi od W4 w ciągu "));
        Serial.print(TIMEOUT_TOTAL/1000, DEC);
        Serial.println(F(" sekund"));
        awaiting_res4 = 0;
        return;
    }

    // Wyczyszczenie bufora Rx
    while (Serial.available())
        Serial.read();

    Serial.print(F(MAIN_MENU_TEXT));

    if (!Serial.available())
        return;

    // Odczytanie odpowiedzi użytkownika
    choice = Serial.read();

    // zmiana wyświetlanego stanu
    setLedState(TRANSMITTING);

    // Przygotowanie bazowego REQ
    memset(Tx_buff, 0, BUFF_LEN);
    SET_TYP(Tx_buff, MSG_T_REQ);

    // Ustawienie DST i oczekiwania na odpowiedź
    switch (choice) {
        case '6': SET_CMD(Tx_buff, 1);
        case '1': SET_DST(Tx_buff, S1_ID); awaiting_res1 = awaiting_res4 = 1 | (S1_ID << 1); break;
        case '2': SET_DST(Tx_buff, S2_ID); awaiting_res1 = awaiting_res4 = 1 | (S2_ID << 1); break;
        case '3': SET_DST(Tx_buff, S3_ID); awaiting_res1 = awaiting_res4 = 1 | (S3_ID << 1); break;
        case '4': SET_DST(Tx_buff, S4_ID); awaiting_res1 = awaiting_res4 = 1 | (S4_ID << 1); break;
        case '5': SET_DST(Tx_buff, S5_ID); awaiting_res1 = awaiting_res4 = 1 | (S5_ID << 1); break;
        default: Serial.print(F("Nieprawidłowy wybór: ")); Serial.println(choice); return;
    }

    SET_CRC(Tx_buff, calcCRC8(Tx_buff, REQ_LEN - 1));

    // Wysłanie REQ i zapisanie czasu wysłania
    Serial1.write(Tx_buff, REQ_LEN);
    sent1 = millis();
    Serial4.write(Tx_buff, REQ_LEN);
    sent4 = millis();

    // Zapewnienie użytkownika
    Serial.print(F("Wysłano zapytanie, oczekiwanie na odpowiedź przez "));
    Serial.print(TIMEOUT_TOTAL/1000, DEC);
    Serial.println(F(" sekund"));

    // zmiana wyświetlanego ponownie na IDLE, zaraz konkretny da handleLHS
    setLedState(IDLE);

    // Oczekiwanie na to co wróci
    await:
    handleLHS(Serial1, SERIAL1_NEI);
    handleLHS(Serial4, SERIAL4_NEI);

}
