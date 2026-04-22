/*
 * S1 / S2 / S3 / S4 / S5 - Dummy Sensor (czujnik z symulowanymi danymi)
 * Sprzęt: Arduino MKR WAN 1310
 *
 * Serial1 → magistrala BUS do mastera (W1 lub W2)
 *
 * NIE ZMIENIAMY podłączenia sprzętowego.
 */

#include <Arduino.h>
#include "led_helper.h"
#include "uart_helper.h"
#include "const_defs.h"
#include "proto_defs.h"
#include "color_helper.h"

// ===== KONFIGURACJA - zmień MY_ID dla każdego sensora =====
#define MY_ID S1_ID

#if MY_ID == S2_ID || MY_ID == S3_ID
  #define MASTER_ID W2_ID
#elif MY_ID == S1_ID || MY_ID == S4_ID || MY_ID == S5_ID
  #define MASTER_ID W1_ID
#endif

// ===== Globalne bufory =====
byte Tx_buff[BUFF_LEN];
byte Rx_buff[BUFF_LEN];
byte ret_msg[RET_LEN];

void setup() {
    Serial1.begin(UART_BAUD);
    setup_sercoms();

    memset(Tx_buff, 0, BUFF_LEN);
    memset(Rx_buff, 0, BUFF_LEN);
    memset(ret_msg, 0, RET_LEN);

    SET_DST(ret_msg, MASTER_ID);
    SET_TYP(ret_msg, MSG_T_RET);
    SET_CRC(ret_msg, calcCRC8(ret_msg, RET_LEN - 1));

    setLedState(IDLE);
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

                triggerLedState(TRANSMITTING);

                memset(Tx_buff, 0, BUFF_LEN);
                SET_DST(Tx_buff, W0_ID);
                SET_TYP(Tx_buff, MSG_T_RES);
                SET_SRC(Tx_buff, MY_ID);

                // Odczyt koloru - pakuj R/G/B w 12 bitów (4 bity na kanał)
                uint16_t payload =
                    ((readColor(COLOR_RED)   >> 4) << 8) |
                    ((readColor(COLOR_GREEN) >> 4) << 4) |
                     (readColor(COLOR_BLUE)  >> 4);
                SET_READ(Tx_buff, payload);

                SET_CRC(Tx_buff, calcCRC8(Tx_buff, RES_LEN - 1));
                Serial1.write(Tx_buff, RES_LEN);
                break;
            }

            case MSG_T_RET: {
                // Master prosi o retransmisję
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
