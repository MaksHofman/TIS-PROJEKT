#include <Arduino.h>
#include "api/Common.h"
#include "led_helper.h"
#include "uart_helper.h"
#include "const_defs.h"
#include "proto_defs.h"

#define MY_ID W1_ID

#if MY_ID==W2_ID
#define MY_SLAVE1 S2_ID
#define MY_SLAVE2 S3_ID
#define SERIAL3_ID W4_ID
#define SERIAL4_ID W3_ID
#elif MY_ID==W1_ID
#define MY_SLAVE1 S1_ID
#define MY_SLAVE2 S4_ID
#define MY_SLAVE3 S5_ID
#define SERIAL3_ID W3_ID
#define SERIAL4_ID W4_ID
#endif

#define SLAVE_TIMEOUT 2000

byte Tx_buff[BUFF_LEN], Rx_buff[BUFF_LEN];
byte ret_buf[RET_LEN];
uint8_t awaiting_res = 0;
unsigned long sent = 0;

void handleRHS(Uart &from, uint8_t from_id) {
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

        switch (GET_TYP(Rx_buff)) {
            case MSG_T_REQ: {
                uint8_t dst = GET_DST(Rx_buff);
                if( dst != MY_SLAVE1 && dst != MY_SLAVE2
                    #if MY_ID==W1_ID
                    && dst != MY_SLAVE3
                    #endif
                ) continue; // Puszczamy mimo, nie do nas

                triggerLedState(TRANSMITTING);
                awaiting_res = 1 | (dst << 1);
                sent = millis();
                memcpy(Tx_buff, Rx_buff, REQ_LEN);
                Serial1.write(Tx_buff, REQ_LEN);
                break;
            }
            case MSG_T_RET: {
                if (GET_DST(Rx_buff) == MY_ID) {
                    from.write(Tx_buff, RES_LEN);
                    triggerLedState(RETRANSMITTING);
                }
                break;
            }
        }
    }
}

void setup() {
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

    handleRHS(Serial4, SERIAL4_ID);
    handleRHS(Serial3, SERIAL3_ID);

    // Timeout
    if (awaiting_res && (millis() - sent) > SLAVE_TIMEOUT) {
        triggerLedState(GENERIC_ERROR);
        memset((void*) Tx_buff, 0, BUFF_LEN);
        SET_DST(Tx_buff, W0_ID);
        SET_TYP(Tx_buff, MSG_T_DEAD);
        SET_SRC(Tx_buff, MY_ID);
        SET_DEAD(Tx_buff, (awaiting_res >> 1));
        SET_CRC(Tx_buff, calcCRC8(Tx_buff, DEAD_LEN - 1));
        Serial3.write(Tx_buff, DEAD_LEN);
        Serial4.write(Tx_buff, DEAD_LEN);
        awaiting_res = 0;
    }

    // Odczyt z lewej strony (Slave)
    while (Serial1.available() > 0) {
        // Jeśli nie oczekujemy na odpowiedź, po co nam to, czyscimy
        if (!(awaiting_res && (millis() - sent) <= SLAVE_TIMEOUT)) {
            Serial1.read();
            continue;
        }

        uint8_t typ = (Serial1.peek() & 0x0E) >> 1;
        uint8_t exp_len = 0;

        if (typ == MSG_T_REQ) exp_len = REQ_LEN;
        else if (typ == MSG_T_RES) exp_len = RES_LEN;
        else if (typ == MSG_T_DEAD) exp_len = DEAD_LEN;
        else if (typ == MSG_T_RET) exp_len = RET_LEN;
        else { Serial1.read(); continue; }

        if (Serial1.available() < exp_len) break;

        triggerLedState(RECEIVING);
        memset(Rx_buff, 0, BUFF_LEN);
        Serial1.readBytes(Rx_buff, exp_len);

        if (GET_CRC(Rx_buff) != calcCRC8(Rx_buff, exp_len - 1)) {
            while(Serial1.available()) Serial1.read();
            SET_DST(ret_buf, (awaiting_res >> 1));
            SET_CRC(ret_buf, calcCRC8(ret_buf, RET_LEN - 1));
            Serial1.write(ret_buf, RET_LEN);
            triggerLedState(GENERIC_ERROR);
            break;
        }

        switch (GET_TYP(Rx_buff)) {
            case MSG_T_RET:
                if (GET_DST(Rx_buff) == MY_ID) {
                    Serial1.write(Tx_buff, REQ_LEN);
                    triggerLedState(RETRANSMITTING);
                }
                break;
            case MSG_T_RES:
                triggerLedState(TRANSMITTING);
                memcpy(Tx_buff, Rx_buff, RES_LEN);
                Serial3.write(Tx_buff, RES_LEN);
                Serial4.write(Tx_buff, RES_LEN);
                awaiting_res = 0;
                break;
        }
    }
}
