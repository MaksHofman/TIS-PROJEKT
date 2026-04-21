#include <Arduino.h>
// #include <CRC.h>
#include "api/Common.h"
#include "led_helper.h"
#include "uart_helper.h"
#include "const_defs.h"
#include "proto_defs.h"

/* Oczywiście to może być W2 lub W1 */
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

/* Obsługa prawej strony (od W3 i W4) */
void handleRHS(Uart &from, uint8_t from_id) {
    /* Odbieramy od strony PC po naszej linii */
    uint8_t recv = from.available();
    uint8_t crc_valid = 0;
    uint8_t dst = 0;
    if(recv >= REQ_LEN) {
        setLedState(RECEIVING);
        memset((void*) Rx_buff, 0, BUFF_LEN);
        /* Teoretycznie możemy się akurat wstrzelić tutaj w trakcie odbierania, ale pal 5, może zadziała */
        from.readBytes(Rx_buff, min(recv, BUFF_LEN));
        /* Nie powinniśmy od tej strony otrzymać nic dłuższego, tylko request i retransmisja */
        switch (GET_TYP(Rx_buff)) {
            case MSG_T_REQ:
            crc_valid = calcCRC8(Rx_buff, REQ_LEN - 1) == GET_CRC(Rx_buff);

            /* To nie jest wiadomość do moich niewolników */
            dst = GET_DST(Rx_buff);
            if( dst != MY_SLAVE1 &&
                dst != MY_SLAVE2 &&
#if MY_ID==W1_ID
                dst != MY_SLAVE3 &&
#endif
                crc_valid)
                    break;

            // CRC się nie zgadza
            if(!crc_valid) {
                setLedState(GENERIC_ERROR);
                SET_DST(ret_buf, from_id);
                SET_CRC(ret_buf, calcCRC8(ret_buf, RET_LEN - 1));
                from.write(ret_buf, RET_LEN);
                break;
            } else {
                setLedState(TRANSMITTING);
                // Wiadomość to request do jednego z moich SLAVE'ów i ma poprawne CRC, forwarduję
                // Czekamy na odpowiedź
                awaiting_res = 1 | (GET_DST(Rx_buff) << 1);
                sent = millis();
                // W sumie mógłbym dać Tx_buf na prawo i lewo oddzielny, bo trochę RAM'u mam,
                // ale nie spodziewam się nowej wiadomości z prawej jak oczekuję z lewej,
                // więc na razie tryb ECO
                memcpy(Tx_buff, Rx_buff, REQ_LEN);
                Serial1.write(Tx_buff, REQ_LEN);
            }

            break;
            case MSG_T_RET:
            setLedState(TRANSMITTING);
            if( GET_DST(Rx_buff) == MY_ID &&
                calcCRC8(Rx_buff, RET_LEN - 1) == GET_CRC(Rx_buff) )
                    from.write(Tx_buff, RES_LEN);
            break;
            default:
            /*
             * Otrzymaliśmy wiadomość, która nie powinna trafić do tego węzła,
             */
            setLedState(GENERIC_ERROR);
            // czyścimy bufor from
            while(from.available())
                from.readBytes(Rx_buff, min(recv,BUFF_LEN));
            // Czyścimy bufor na odebrane wiadomości
            memset((void*) Rx_buff, 0, BUFF_LEN);
        }
    }
    memset((void*) Rx_buff, 0, BUFF_LEN);
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
    setLedState(IDLE);

    handleRHS(Serial4, SERIAL4_ID);
    handleRHS(Serial3, SERIAL3_ID);

    if (awaiting_res && (millis() - sent) > SLAVE_TIMEOUT) {
        // sent dead
        setLedState(GENERIC_ERROR);

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

    // Ogarnianie lewej strony
    uint8_t recv = Serial1.available();
    uint8_t crc_valid = 0;
    // Odwrócony warunek, żeby nie zagnieżdżać kilku if'ów
    if (!(awaiting_res && (millis() - sent) <= SLAVE_TIMEOUT))
        return;

    if (recv >= RET_LEN) {
        setLedState(RECEIVING);
        Serial1.readBytes(Rx_buff, min(BUFF_LEN, recv));

        setLedState(TRANSMITTING);
        switch (GET_TYP(Rx_buff)) {
            case MSG_T_RET:
            if( GET_DST(Rx_buff) == MY_ID &&
                calcCRC8(Rx_buff, RET_LEN - 1) == GET_CRC(Rx_buff) )
                // W tą stronę retransmitujemy tylko req
                    Serial1.write(Tx_buff, REQ_LEN);
            break;
            case MSG_T_RES:
            crc_valid = calcCRC8(Rx_buff, RES_LEN - 1) == GET_CRC(Rx_buff);
            if (!crc_valid) {
                SET_DST(ret_buf, (awaiting_res >> 1));
                SET_CRC(ret_buf, calcCRC8(ret_buf, RET_LEN - 1));
                Serial1.write(ret_buf, RET_LEN);
                break;
            }
            // CRC się zgadza, to lecimy dalej
            memcpy(Tx_buff, Rx_buff, RES_LEN);
            Serial3.write(Tx_buff, RES_LEN);
            Serial4.write(Tx_buff, RES_LEN);

            awaiting_res = 0;
            break;
            default:
            // Nic innego nie będzie, a najwyżej to zignorujemy
            while(Serial1.available())
                Serial1.readBytes(Rx_buff, min(recv,BUFF_LEN));
            // Czyścimy bufor na odebrane wiadomości
            memset((void*) Rx_buff, 0, BUFF_LEN);
        }
    }

}
