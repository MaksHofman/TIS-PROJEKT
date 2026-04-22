/*
 * S1 - Smart Sensor (prawdziwy czujnik TCS3200 + protokół UART)
 * Sprzęt: Arduino MKR WAN 1310 + czujnik TCS3200
 *
 * Serial1 → magistrala BUS do mastera W1
 *
 * NIE ZMIENIAMY podłączenia sprzętowego.
 *
 * Różnica od dummy-sensor: dane są PRAWDZIWYMI odczytami z czujnika TCS3200
 * z kalibracją (białe tło). Kalibracja uruchamia się automatycznie przy starcie.
 */

#include <Arduino.h>
#include "led_helper.h"
#include "uart_helper.h"
#include "const_defs.h"
#include "proto_defs.h"
#include "color_helper.h"

// ===== KONFIGURACJA =====
#define MY_ID     S1_ID
#define MASTER_ID W1_ID

#define CAL_DELAY_MS 3000   // ile ms czekamy na ustawienie białego wzorca

// ===== Wartości kalibracji =====
uint8_t maxVals[5] = {0};   // używane przez color_helper.cpp

// ===== Globalne bufory =====
byte Tx_buff[BUFF_LEN];
byte Rx_buff[BUFF_LEN];
byte ret_msg[RET_LEN];

bool calibrated = false;

// ===== Kalibracja =====
void calibrate() {
    Serial.println(F("Kalibracja - poloz BIALY obiekt przed czujnikiem..."));
    delay(CAL_DELAY_MS);

    maxVals[COLOR_RED]   = readColor(COLOR_RED);
    maxVals[COLOR_GREEN] = readColor(COLOR_GREEN);
    maxVals[COLOR_BLUE]  = readColor(COLOR_BLUE);
    maxVals[COLOR_CLEAR] = readColor(COLOR_CLEAR);

    Serial.print(F("Kalibracja: R="));  Serial.print(maxVals[COLOR_RED]);
    Serial.print(F(" G="));             Serial.print(maxVals[COLOR_GREEN]);
    Serial.print(F(" B="));             Serial.print(maxVals[COLOR_BLUE]);
    Serial.print(F(" C="));             Serial.println(maxVals[COLOR_CLEAR]);
    calibrated = true;
}

void setup() {
    Serial.begin(115200);

    // TCS3200 piny
    pinMode(S0, OUTPUT);
    pinMode(S1_PIN, OUTPUT);
    pinMode(S2, OUTPUT);
    pinMode(S3, OUTPUT);
    pinMode(OUT_PIN, INPUT);

    // Skalowanie częstotliwości 20%
    digitalWrite(S0, HIGH);
    digitalWrite(S1_PIN, LOW);

    Serial1.begin(UART_BAUD);
    setup_sercoms();

    memset(Tx_buff, 0, BUFF_LEN);
    memset(Rx_buff, 0, BUFF_LEN);
    memset(ret_msg, 0, RET_LEN);

    SET_DST(ret_msg, MASTER_ID);
    SET_TYP(ret_msg, MSG_T_RET);
    SET_CRC(ret_msg, calcCRC8(ret_msg, RET_LEN - 1));

    setLedState(IDLE);
    Serial.println(F("Smart Sensor S1 gotowy."));

    // Kalibracja przy starcie - opcjonalna
    // Odkomentuj jeśli chcesz kalibrację:
    // calibrate();
}

void loop() {
    updateLedTask();

    while (Serial1.available() > 0) {
        uint8_t typ = (Serial1.peek() & 0x0E) >> 1;
        uint8_t exp_len = 0;

        switch (typ) {
            case MSG_T_REQ:  exp_len = REQ_LEN;  break;
            case MSG_T_RES:  exp_len = RES_LEN;  break;
            case MSG_T_DEAD: exp_len = DEAD_LEN; break;
            case MSG_T_RET:  exp_len = RET_LEN;  break;
            default:
                Serial1.read();
                continue;
        }

        if (Serial1.available() < exp_len) break;

        triggerLedState(RECEIVING);
        memset(Rx_buff, 0, BUFF_LEN);
        Serial1.readBytes(Rx_buff, exp_len);

        if (!hammingVerify(Rx_buff, exp_len)) {
            while (Serial1.available()) Serial1.read();
            Serial1.write(ret_msg, RET_LEN);
            triggerLedState(GENERIC_ERROR);
            break;
        }

        switch (GET_TYP(Rx_buff)) {
            case MSG_T_REQ: {
                if (GET_DST(Rx_buff) != MY_ID) break; // nie do nas

                // Sprawdź czy to zapytanie kalibracyjne
                if (GET_CMD(Rx_buff) == 1) {
                    calibrate();
                }

                triggerLedState(TRANSMITTING);

                memset(Tx_buff, 0, BUFF_LEN);
                SET_DST(Tx_buff, W0_ID);
                SET_TYP(Tx_buff, MSG_T_RES);
                SET_SRC(Tx_buff, MY_ID);

                // Odczyt koloru - pakuj R/G/B w 12 bitów (4 bity na kanał)
                uint8_t r = readColor(COLOR_RED);
                uint8_t g = readColor(COLOR_GREEN);
                uint8_t b = readColor(COLOR_BLUE);

                uint16_t payload =
                    ((r >> 4) << 8) |
                    ((g >> 4) << 4) |
                     (b >> 4);
                SET_READ(Tx_buff, payload);

                SET_CRC(Tx_buff, calcCRC8(Tx_buff, RES_LEN - 1));
                Serial1.write(Tx_buff, RES_LEN);

                Serial.print(F("Wyslan odczyt: R="));
                Serial.print(r); Serial.print(F(" G="));
                Serial.print(g); Serial.print(F(" B="));
                Serial.println(b);
                break;
            }

            case MSG_T_RET: {
                if (GET_DST(Rx_buff) == MY_ID) {
                    Serial1.write(Tx_buff, RES_LEN);
                    triggerLedState(RETRANSMITTING);
                }
                break;
            }

            default:
                break;
        }
    }
}
