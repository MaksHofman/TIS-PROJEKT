#include <Arduino.h>
#include "api/Common.h"
#include "led_helper.h"
#include "uart_helper.h"
#include "const_defs.h"
#include "proto_defs.h"

#define MY_ID W3_ID

#if MY_ID==W4_ID
#define SERIAL4_NEI W1_ID
#define SERIAL3_NEI W2_ID
#define THROUGH_SERIAL4(DST) (!((DST) & 0x1))
#define THROUGH_SERIAL3(DST) (((DST) & 0x1))
#elif MY_ID==W3_ID
#define SERIAL4_NEI W2_ID
#define SERIAL3_NEI W1_ID
#define THROUGH_SERIAL4(DST) (((DST) & 0x1))
#define THROUGH_SERIAL3(DST) (!((DST) & 0x1))
#endif

#define LHS_TIMEOUT 4000

byte Tx_buff[BUFF_LEN], Rx_buff[BUFF_LEN];
byte ret_buf[RET_LEN];

uint8_t awaiting_res = 0;
unsigned long sent = 0;

void handleLHS(Uart &from, uint8_t from_id) {
    while (from.available() > 0) {
        uint8_t typ = (from.peek() & 0x0E) >> 1;
        uint8_t exp_len = 0;

        if (typ == MSG_T_REQ) exp_len = REQ_LEN;
        else if (typ == MSG_T_RES) exp_len = RES_LEN;
        else if (typ == MSG_T_DEAD) exp_len = DEAD_LEN;
        else if (typ == MSG_T_RET) exp_len = RET_LEN;
        else { from.read(); continue; }

        if (from.available() < exp_len) return;

        triggerLedState(RECEIVING);
        memset(Rx_buff, 0, BUFF_LEN);
        from.readBytes(Rx_buff, exp_len);

        if (GET_CRC(Rx_buff) != calcCRC8(Rx_buff, exp_len - 1)) {
            while(from.available()) from.read();
            SET_DST(ret_buf, from_id);
            SET_CRC(ret_buf, calcCRC8(ret_buf, RET_LEN - 1));
            from.write(ret_buf, RET_LEN);
            triggerLedState(GENERIC_ERROR);
            return;
        }

        uint8_t msg_type = GET_TYP(Rx_buff);
        switch (msg_type) {
            case MSG_T_RET:
                from.write(Tx_buff, REQ_LEN);
                triggerLedState(RETRANSMITTING);
                break;
            case MSG_T_RES:
                if (awaiting_res) {
                    memcpy(Tx_buff, Rx_buff, RES_LEN);
                    Serial1.write(Tx_buff, RES_LEN);
                    triggerLedState(TRANSMITTING);
                    awaiting_res = 0;
                }
                break;
            case MSG_T_DEAD:
                if (awaiting_res) {
                    memcpy(Tx_buff, Rx_buff, DEAD_LEN);
                    Serial1.write(Tx_buff, DEAD_LEN);
                    triggerLedState(TRANSMITTING);
                    awaiting_res = 0;
                }
                break;
        }
    }
}

void setup() {
    Serial.begin(115200);
    Serial1.begin(UART_BAUD);
    Serial3.begin(UART_BAUD);
    Serial4.begin(UART_BAUD);

    setup_sercoms();
    memset((void*) Tx_buff, 0, BUFF_LEN);
    memset((void*) Rx_buff, 0, BUFF_LEN);
    memset((void*) ret_buf, 0, RET_LEN);
    SET_TYP(ret_buf, MSG_T_RET);
    setLedState(IDLE);
}

void loop() {
    updateLedTask();

    // Obsługa P->L (Od W0 / Prawej strony)
    while (Serial1.available() > 0) {
        uint8_t typ = (Serial1.peek() & 0x0E) >> 1;
        uint8_t exp_len = 0;

        if (typ == MSG_T_REQ) exp_len = REQ_LEN;
        else if (typ == MSG_T_RES) exp_len = RES_LEN;
        else if (typ == MSG_T_DEAD) exp_len = DEAD_LEN;
        else if (typ == MSG_T_RET) exp_len = RET_LEN;
        else { Serial1.read(); continue; }

        if (Serial1.available() < exp_len) break; // Czeka na resztę

        triggerLedState(RECEIVING);
        memset(Rx_buff, 0, BUFF_LEN);
        Serial1.readBytes(Rx_buff, exp_len);

        if (GET_CRC(Rx_buff) != calcCRC8(Rx_buff, exp_len - 1)) {
            while(Serial1.available()) Serial1.read();
            SET_DST(ret_buf, W0_ID);
            SET_CRC(ret_buf, calcCRC8(ret_buf, RET_LEN - 1));
            Serial1.write(ret_buf, RET_LEN);
            triggerLedState(GENERIC_ERROR);
            break;
        }

        uint8_t req_type = GET_TYP(Rx_buff);
        switch (req_type) {
            case MSG_T_REQ: {
                memcpy(Tx_buff, Rx_buff, REQ_LEN);
                uint8_t dst_id = GET_DST(Tx_buff);

                if (THROUGH_SERIAL4(dst_id)) {
                    Serial4.write(Tx_buff, REQ_LEN);
                    triggerLedState(TRANSMITTING);
                } else if (THROUGH_SERIAL3(dst_id)) {
                    Serial3.write(Tx_buff, REQ_LEN);
                    triggerLedState(TRANSMITTING);
                } else {
                    triggerLedState(GENERIC_ERROR);
                }
                sent = millis();
                awaiting_res = 1 | (dst_id << 1);
                break;
            }
            case MSG_T_RET: {
                uint8_t tx_type = GET_TYP(Tx_buff);
                if (tx_type == MSG_T_DEAD) {
                    Serial1.write(Tx_buff, DEAD_LEN);
                    triggerLedState(RETRANSMITTING);
                } else if (tx_type == MSG_T_RES) {
                    Serial1.write(Tx_buff, RES_LEN);
                    triggerLedState(RETRANSMITTING);
                }
                break;
            }
        }
    }

    // Obsługa L->P (Od strony Slaves)
    handleLHS(Serial4, SERIAL4_NEI);
    handleLHS(Serial3, SERIAL3_NEI);

    // Timeout
    if (awaiting_res && (millis() - sent > LHS_TIMEOUT)) {
        uint8_t dead_node = (awaiting_res >> 1);
        memset(Tx_buff, 0, BUFF_LEN);
        SET_DST(Tx_buff, W0_ID);
        SET_TYP(Tx_buff, MSG_T_DEAD);
        SET_SRC(Tx_buff, MY_ID);
        SET_DEAD(Tx_buff, dead_node);
        SET_CRC(Tx_buff, calcCRC8(Tx_buff, DEAD_LEN - 1 ));

        Serial1.write(Tx_buff, DEAD_LEN);
        triggerLedState(GENERIC_ERROR);
        awaiting_res = 0;
    }
}
