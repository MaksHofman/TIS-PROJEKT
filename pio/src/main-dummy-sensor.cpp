#include <Arduino.h>
#include "led_helper.h"
#include "uart_helper.h"
#include "const_defs.h"
#include "proto_defs.h"
#include "color_helper.h"

#define MY_ID S1_ID

#if MY_ID==S2_ID || MY_ID==S3_ID
#define MASTER_ID W2_ID
#elif MY_ID==S1_ID || MY_ID==S4_ID || MY_ID==S5_ID
#define MASTER_ID W1_ID
#endif

byte Tx_buff[BUFF_LEN], Rx_buff[BUFF_LEN];
byte ret_msg[RET_LEN];

uint8_t maxVals[5] = {0};

void setup() {
    Serial1.begin(UART_BAUD);
    setup_sercoms();

    memset((void*) Tx_buff, 0, BUFF_LEN);
    memset((void*) Rx_buff, 0, BUFF_LEN);

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

        if (typ == MSG_T_REQ) exp_len = REQ_LEN;
        else if (typ == MSG_T_RES) exp_len = RES_LEN;
        else if (typ == MSG_T_DEAD) exp_len = DEAD_LEN;
        else if (typ == MSG_T_RET) exp_len = RET_LEN;
        else { Serial1.read(); continue; }

        if (Serial1.available() < exp_len) break; // Zbyt mało bajtów

        triggerLedState(RECEIVING);
        memset((void*) Rx_buff, 0, BUFF_LEN);
        Serial1.readBytes(Rx_buff, exp_len);

        if (GET_CRC(Rx_buff) != calcCRC8(Rx_buff, exp_len - 1)) {
            while(Serial1.available()) Serial1.read();
            Serial1.write(ret_msg, RET_LEN);
            triggerLedState(GENERIC_ERROR);
            break;
        }

        switch(GET_TYP(Rx_buff)) {
            case MSG_T_REQ: {
                if (GET_DST(Rx_buff) != MY_ID) continue; // Ramka OK, ale nie do nas (ignoruj)

                triggerLedState(TRANSMITTING);
                memset((void*) Rx_buff, 0, BUFF_LEN);

                SET_DST(Tx_buff, W0_ID);
                SET_TYP(Tx_buff, MSG_T_RES);
                SET_SRC(Tx_buff, MY_ID);

                uint16_t payload =
                ((readColor(COLOR_RED) >> 4) << 8) |
                ((readColor(COLOR_GREEN) >> 4) << 4) |
                (readColor(COLOR_BLUE) >> 4);
                SET_READ(Tx_buff, payload);

                SET_CRC(Tx_buff, calcCRC8(Tx_buff, RES_LEN - 1));
                Serial1.write(Tx_buff, RES_LEN);
                break;
            }
            case MSG_T_RET: {
                if( GET_DST(Rx_buff) == MY_ID ) {
                    Serial1.write(Tx_buff, RES_LEN);
                    triggerLedState(RETRANSMITTING);
                }
                break;
            }
        }
    }
}
