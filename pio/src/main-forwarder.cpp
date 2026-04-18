#include <Arduino.h>
#include <CRC.h>
#include "api/Common.h"
#include "uart_helper.h"
#include "const_defs.h"
#include "proto_defs.h"

/* Do ręcznej zmiany W3 lub W4 */
#define MY_ID W4_ID

/* Kocham preprocessing */
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
    uint8_t recv = from.available();
    uint8_t crc_valid = 0;

    // Tu może przyjść: ret, dead, res
    if (!(recv >= RET_LEN))
        return;
        
    // Coś mi się czuje, że gdzieś zgubię tą synchronizację, o której pisałem przy robieniu protokołu...
    from.readBytes(Rx_buff, min(BUFF_LEN, recv));

    // No tak, rcv - 1, nieco upraszcz logikę (mam nadzieję, że synchronizacja nie zrobi brrr)
    // Trzeba na Koncentratorze podpiętym do PC zadbać o to, że jak przyjmie komendę,
    // to przed upływem timeout'u lub otrzymaniem odpowiedzi nic nie wyśle
    crc_valid = GET_CRC(Rx_buff) == calcCRC8(Rx_buff, recv - 1);
    if (!crc_valid) {
        SET_DST(ret_buf, from_id);
        SET_CRC(ret_buf, calcCRC8(ret_buf, RET_LEN - 1));
        from.write(ret_buf, RET_LEN);
        return;
    }

    switch (GET_TYP(Rx_buff)) {
        // Nic innego w tamtą stronę nie wysyłamy, chyba że coś poszło grubo nie tak...
        case MSG_T_RET: from.write(Tx_buff, REQ_LEN); break;
        case MSG_T_RES:
        memcpy(Tx_buff, Rx_buff, RES_LEN);
        Serial1.write(Tx_buff, RES_LEN);
        // Nie chce mi się już sprawdzać, czy to jest ten response,
        // na który czekamy, no nic innego się chyba nie pojawi...
        awaiting_res = 0;
        break;
        case MSG_T_DEAD:
        memcpy(Tx_buff, Rx_buff, DEAD_LEN);
        Serial1.write(Tx_buff, DEAD_LEN);
        break;
        default: break;
    }
    
}

void setup() {
    Serial1.begin(UART_BAUD);

    Serial3.begin(UART_BAUD);
    Serial4.begin(UART_BAUD);

    setup_sercoms();
    /* #TODO: JAkieś statusy LED'em RGB, uśpienie */
    memset((void*) Tx_buff, 0, BUFF_LEN);
    memset((void*) Rx_buff, 0, BUFF_LEN);
    memset((void*) ret_buf, 0, RET_LEN);
    SET_TYP(ret_buf, MSG_T_RET);
}

void loop() {
    // Z prawej do lewej
    uint8_t recvR = Serial1.available();
    uint8_t crc_valid = 0;

    if (!(recvR >= REQ_LEN))
        goto left_to_right; // WARNING: goto

    memset(Rx_buff, 0, BUFF_LEN);
    Serial1.readBytes(Rx_buff, min(recvR, BUFF_LEN));

    // Z tej strony spodziewamy się tylko req lub ret, więc mamy szczęście, że obydwie są po 2B
    crc_valid = GET_CRC(Rx_buff) == calcCRC8(Rx_buff, RES_LEN - 1);
    if (!crc_valid) {
        SET_DST(ret_buf, W0_ID);
        SET_CRC(ret_buf, calcCRC8(ret_buf, RET_LEN - 1));
        Serial1.write(ret_buf, RET_LEN);
        goto left_to_right; // WARNING: goto
    }

    // CRC w porządku, mamy jakąś wiadomość (REQ lub RET)
    switch (GET_TYP(Rx_buff)) {
        case MSG_T_REQ:
        // od razu przekopiuję do Tx_buff
        memcpy(Tx_buff, Rx_buff, REQ_LEN);
        // Dostaliśmy request, trzeba ustalić dokąd
        if (THROUGH_SERIAL4(GET_DST(Tx_buff))) {
            Serial4.write(Tx_buff, REQ_LEN);
        } else if (THROUGH_SERIAL3(GET_DST(Tx_buff))) {
            Serial3.write(Tx_buff, REQ_LEN);
        } else {
            // IDK, jakaś diodka statusowa
        }
        sent = millis();
        // W najgorszym przypadku zrobimy 3x tą samą operację (GET_DST()), ale zaoszczędziliśmy cały 1B RAM'u
        awaiting_res = 1 | (GET_DST(Tx_buff) << 1);

        break;
        case MSG_T_RET:
        // DEAD lub RES (niestety różna długość, ale co to dla zagnieżdżonego switch'a)
        switch (GET_TYP(Tx_buff)) {
            case MSG_T_DEAD: Serial1.write(Tx_buff, DEAD_LEN); break;
            case MSG_T_RES: Serial1.write(Tx_buff, RES_LEN); break;
            default: break;
        }
        default: break;
    }

    // Z lewej do prawej
    left_to_right:
    // Jak na nic nie czekamy to możemy sobie odpocząć
    if (!awaiting_res)
        return;

    // Tu nie powinno być sytuacji, że z 2 stron jest to samo, bo są różne mastery
    handleLHS(Serial4, SERIAL4_NEI);
    handleLHS(Serial3, SERIAL3_NEI);

    // timeout
    if (awaiting_res && (millis() - sent > LHS_TIMEOUT)) {
        memset(Tx_buff, 0, BUFF_LEN);
        SET_DST(Tx_buff, W0_ID);
        SET_TYP(Tx_buff, MSG_T_DEAD);
        SET_SRC(Tx_buff, MY_ID);
        SET_DEAD(Tx_buff, (awaiting_res >> 1));
        SET_CRC(Tx_buff, calcCRC8(Tx_buff, DEAD_LEN - 1 ));
        Serial1.write(Tx_buff, DEAD_LEN);
    }
}