/*
 * W0 - Agregator (węzeł nadrzędny, podłączony przez USB do PC)
 * Sprzęt: Arduino MKR WAN 1310
 *
 * Serial  → USB/PC
 * Serial1 → forwarder W3
 * Serial4 → forwarder W4
 *
 * NIE ZMIENIAMY podłączenia sprzętowego.
 */

#include <Arduino.h>
#include "led_helper.h"
#include "uart_helper.h"
#include "const_defs.h"
#include "proto_defs.h"

#define MY_ID         W0_ID
#define SERIAL1_NEI   W3_ID
#define SERIAL4_NEI   W4_ID
#define TIMEOUT_TOTAL 10000   // ms

// Menu tekstowe
#define MAIN_MENU_TEXT \
    "\r\n==== MENU ====\r\n" \
    "1 - Zapytaj S1 (R/G/B)\r\n" \
    "2 - Zapytaj S2\r\n" \
    "3 - Zapytaj S3\r\n" \
    "4 - Zapytaj S4\r\n" \
    "5 - Zapytaj S5\r\n" \
    "6 - Zapytaj S1 (kalibracja)\r\n" \
    "Wybor: "

// ===== Globalne bufory =====
byte Tx_buff[BUFF_LEN];
byte Rx_buff[BUFF_LEN];
byte ret_buf[RET_LEN];

byte choice = '0';

uint8_t  awaiting_res1 = 0;   // czy czekamy przez Serial1
uint8_t  awaiting_res4 = 0;   // czy czekamy przez Serial4
unsigned long sent1    = 0;
unsigned long sent4    = 0;

/*
 * handleLHS - odbiera wiadomości od forwarderów
 */
void handleLHS(Uart &from, uint8_t from_id, uint8_t &awaiting) {
    while (from.available() > 0) {
        uint8_t typ = (from.peek() & 0x0E) >> 1;
        uint8_t exp_len = 0;

        switch (typ) {
            case MSG_T_REQ:  exp_len = REQ_LEN;  break;
            case MSG_T_RES:  exp_len = RES_LEN;  break;
            case MSG_T_DEAD: exp_len = DEAD_LEN; break;
            case MSG_T_RET:  exp_len = RET_LEN;  break;
            default:
                from.read();
                continue;
        }

        if (from.available() < exp_len) return;

        triggerLedState(RECEIVING);
        memset(Rx_buff, 0, BUFF_LEN);
        from.readBytes(Rx_buff, exp_len);

        if (!hammingVerify(Rx_buff, exp_len)) {
            while (from.available()) from.read();
            SET_DST(ret_buf, from_id);
            SET_CRC(ret_buf, calcCRC8(ret_buf, RET_LEN - 1));
            from.write(ret_buf, RET_LEN);
            triggerLedState(GENERIC_ERROR);
            return;
        }

        uint8_t id, r, g, b, maxVal;

        switch (GET_TYP(Rx_buff)) {
            case MSG_T_RET:
                // Forwarder prosi o retransmisję zapytania
                triggerLedState(RETRANSMITTING);
                from.write(Tx_buff, REQ_LEN);
                break;

            case MSG_T_DEAD:
                triggerLedState(GENERIC_ERROR);
                awaiting = 0;
                Serial.print(F("Wykryto awarie lacza pomiedzy "));
                switch (GET_SRC(Rx_buff)) {
                    case W1_ID: Serial.print(F("W1 i ")); break;
                    case W2_ID: Serial.print(F("W2 i ")); break;
                    case W3_ID: Serial.print(F("W3 i ")); break;
                    case W4_ID: Serial.print(F("W4 i ")); break;
                    default:    Serial.print(F("N/A i ")); break;
                }
                switch (GET_DEAD(Rx_buff)) {
                    case W1_ID: Serial.println(F("W1.")); break;
                    case W2_ID: Serial.println(F("W2.")); break;
                    case S1_ID: Serial.println(F("S1.")); break;
                    case S2_ID: Serial.println(F("S2.")); break;
                    case S3_ID: Serial.println(F("S3.")); break;
                    case S4_ID: Serial.println(F("S4.")); break;
                    case S5_ID: Serial.println(F("S5.")); break;
                    default:    Serial.println(F("N/A.")); break;
                }
                break;

            case MSG_T_RES:
                awaiting = 0;
                id = GET_SRC(Rx_buff);
                r = 0; g = 0; b = 0;

                if (id == S1_ID) {
                    UNPACK_RGB(GET_READ(Rx_buff), r, g, b);
                    Serial.print(F("Odczyt S1: R="));   Serial.print(r, DEC);
                    Serial.print(F(", G="));             Serial.print(g, DEC);
                    Serial.print(F(", B="));             Serial.print(b, DEC);
                    Serial.print(F(". Dominujacy: "));
                    maxVal = max(r, max(g, b));
                    if      (maxVal == r) Serial.println(F("czerwony."));
                    else if (maxVal == g) Serial.println(F("zielony."));
                    else if (maxVal == b) Serial.println(F("niebieski."));
                    else                  Serial.println(F("N/A."));
                } else {
                    Serial.print(F("Odczyt z S"));
                    switch (id) {
                        case S2_ID: Serial.print(F("2")); break;
                        case S3_ID: Serial.print(F("3")); break;
                        case S4_ID: Serial.print(F("4")); break;
                        case S5_ID: Serial.print(F("5")); break;
                        default:    Serial.print(F("?")); break;
                    }
                    Serial.print(F(": raw=0x"));
                    Serial.println(GET_READ(Rx_buff), HEX);
                }
                break;

            default:
                break;
        }
    }
}

void setup() {
    Serial.begin(115200);
    Serial1.begin(UART_BAUD);
    Serial4.begin(UART_BAUD);

    setup_sercoms();

    memset(Tx_buff, 0, BUFF_LEN);
    memset(Rx_buff, 0, BUFF_LEN);
    memset(ret_buf, 0, RET_LEN);

    SET_TYP(ret_buf, MSG_T_RET);
    setLedState(IDLE);
    Serial.println(F("W0 Agregator gotowy."));
}

void loop() {
    updateLedTask();

    unsigned long now   = millis();
    unsigned long time1 = now - sent1;
    unsigned long time4 = now - sent4;

    // ---- Jeśli czekamy na odpowiedź ----
    if (awaiting_res1 || awaiting_res4) {
        handleLHS(Serial1, SERIAL1_NEI, awaiting_res1);
        handleLHS(Serial4, SERIAL4_NEI, awaiting_res4);

        // Timeout per-linia
        if (awaiting_res1 && time1 > TIMEOUT_TOTAL) {
            Serial.print(F("Timeout W3 ("));
            Serial.print(TIMEOUT_TOTAL / 1000, DEC);
            Serial.println(F("s)"));
            awaiting_res1 = 0;
        }
        if (awaiting_res4 && time4 > TIMEOUT_TOTAL) {
            Serial.print(F("Timeout W4 ("));
            Serial.print(TIMEOUT_TOTAL / 1000, DEC);
            Serial.println(F("s)"));
            awaiting_res4 = 0;
        }

        // Czekamy dalej - nie wchodzimy w menu
        if (awaiting_res1 || awaiting_res4) return;
    }

    // ---- Menu ----
    // Wyczyść bufor Serial przed wyświetleniem menu
    while (Serial.available()) Serial.read();

    Serial.print(F(MAIN_MENU_TEXT));
    while (!Serial.available()) {
        updateLedTask();
        // Obsługuj zaległe pakiety (np. dead messages)
        handleLHS(Serial1, SERIAL1_NEI, awaiting_res1);
        handleLHS(Serial4, SERIAL4_NEI, awaiting_res4);
    }

    choice = Serial.read();
    triggerLedState(TRANSMITTING);

    memset(Tx_buff, 0, BUFF_LEN);
    SET_TYP(Tx_buff, MSG_T_REQ);
    SET_SRC(Tx_buff, W0_ID);

    switch (choice) {
        case '6': SET_CMD(Tx_buff, 1); // kalibracja - celowy fallthrough
        /* falls through */
        case '1': SET_DST(Tx_buff, S1_ID); break;
        case '2': SET_DST(Tx_buff, S2_ID); break;
        case '3': SET_DST(Tx_buff, S3_ID); break;
        case '4': SET_DST(Tx_buff, S4_ID); break;
        case '5': SET_DST(Tx_buff, S5_ID); break;
        default:
            Serial.print(F("Nieprawidlowy wybor: "));
            Serial.println(choice);
            return;
    }

    SET_CRC(Tx_buff, calcCRC8(Tx_buff, REQ_LEN - 1));

    Serial1.write(Tx_buff, REQ_LEN);
    sent1 = millis();
    awaiting_res1 = 1;

    Serial4.write(Tx_buff, REQ_LEN);
    sent4 = millis();
    awaiting_res4 = 1;

    Serial.print(F("Wyslano, oczekiwanie maks "));
    Serial.print(TIMEOUT_TOTAL / 1000, DEC);
    Serial.println(F("s..."));
}
