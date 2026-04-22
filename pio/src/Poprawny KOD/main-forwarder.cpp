/*
 * W3 / W4 - Forwarder (pośrednik między W0/agregator a masterami W1/W2)
 * Sprzęt: Arduino MKR WAN 1310
 *
 * Serial1 → strona W0 (agregator)
 * Serial3 → master W1 lub W2 (zależy od MY_ID - patrz niżej)
 * Serial4 → master W1 lub W2 (zależy od MY_ID)
 *
 * W3: Serial3→W1, Serial4→W2
 * W4: Serial3→W2, Serial4→W1
 *
 * NIE ZMIENIAMY podłączenia sprzętowego.
 */

#include <Arduino.h>
#include "led_helper.h"
#include "uart_helper.h"
#include "const_defs.h"
#include "proto_defs.h"

// ===== KONFIGURACJA - zmień MY_ID dla W4 =====
#define MY_ID W3_ID

#if MY_ID == W4_ID
  #define SERIAL4_NEI  W1_ID
  #define SERIAL3_NEI  W2_ID
  // Parzyste ID → przez Serial4 (W1), nieparzyste → Serial3 (W2)
  #define THROUGH_SERIAL4(DST) (!((DST) & 0x1))
  #define THROUGH_SERIAL3(DST)  (((DST) & 0x1))
#elif MY_ID == W3_ID
  #define SERIAL4_NEI  W2_ID
  #define SERIAL3_NEI  W1_ID
  // Nieparzyste ID → Serial4 (W2), parzyste → Serial3 (W1)
  #define THROUGH_SERIAL4(DST)  (((DST) & 0x1))
  #define THROUGH_SERIAL3(DST) (!((DST) & 0x1))
#endif

#define LHS_TIMEOUT 4000   // ms - timeout oczekiwania na odpowiedź od mastera

// ===== Globalne bufory =====
byte Tx_buff[BUFF_LEN];
byte Rx_buff[BUFF_LEN];
byte ret_buf[RET_LEN];

uint8_t  awaiting_res = 0;   // bit0=1 + bity1..7 = ID docelowego slave'a
unsigned long sent    = 0;

/*
 * handleLHS - obsługa odebrania od masterów (Serial3 / Serial4)
 * "Left Hand Side" = strona slave'ów
 */
void handleLHS(Uart &from, uint8_t from_id) {
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

        switch (GET_TYP(Rx_buff)) {
            case MSG_T_RET:
                // Master prosi o retransmisję zapytania
                from.write(Tx_buff, REQ_LEN);
                triggerLedState(RETRANSMITTING);
                break;

            case MSG_T_RES:
                // Odpowiedź - prześlij do W0
                if (awaiting_res) {
                    memcpy(Tx_buff, Rx_buff, RES_LEN);
                    Serial1.write(Tx_buff, RES_LEN);
                    triggerLedState(TRANSMITTING);
                    awaiting_res = 0;
                }
                break;

            case MSG_T_DEAD:
                // Informacja o awarii - prześlij do W0
                if (awaiting_res) {
                    memcpy(Tx_buff, Rx_buff, DEAD_LEN);
                    Serial1.write(Tx_buff, DEAD_LEN);
                    triggerLedState(TRANSMITTING);
                    awaiting_res = 0;
                }
                break;

            default:
                break;
        }
    }
}

void setup() {
    Serial1.begin(UART_BAUD);   // do aggregatora / W0
    Serial3.begin(UART_BAUD);   // do mastera
    Serial4.begin(UART_BAUD);   // do mastera

    setup_sercoms();

    memset(Tx_buff, 0, BUFF_LEN);
    memset(Rx_buff, 0, BUFF_LEN);
    memset(ret_buf, 0, RET_LEN);

    SET_TYP(ret_buf, MSG_T_RET);
    setLedState(IDLE);
}

void loop() {
    updateLedTask();

    // ---- Obsługa P→L: wiadomości od W0 (Serial1) ----
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
            SET_DST(ret_buf, W0_ID);
            SET_CRC(ret_buf, calcCRC8(ret_buf, RET_LEN - 1));
            Serial1.write(ret_buf, RET_LEN);
            triggerLedState(GENERIC_ERROR);
            break;
        }

        switch (GET_TYP(Rx_buff)) {
            case MSG_T_REQ: {
                memcpy(Tx_buff, Rx_buff, REQ_LEN);
                uint8_t dst_id = GET_DST(Tx_buff);
                bool sent_ok = false;

                if (THROUGH_SERIAL4(dst_id)) {
                    Serial4.write(Tx_buff, REQ_LEN);
                    sent_ok = true;
                } else if (THROUGH_SERIAL3(dst_id)) {
                    Serial3.write(Tx_buff, REQ_LEN);
                    sent_ok = true;
                }

                if (sent_ok) {
                    triggerLedState(TRANSMITTING);
                    sent = millis();
                    awaiting_res = 1 | (dst_id << 1);
                } else {
                    triggerLedState(GENERIC_ERROR);
                }
                break;
            }

            case MSG_T_RET: {
                // W0 prosi o retransmisję naszej ostatniej wiadomości do niego
                uint8_t tx_typ = GET_TYP(Tx_buff);
                if (tx_typ == MSG_T_DEAD)
                    Serial1.write(Tx_buff, DEAD_LEN);
                else if (tx_typ == MSG_T_RES)
                    Serial1.write(Tx_buff, RES_LEN);
                triggerLedState(RETRANSMITTING);
                break;
            }

            default:
                break;
        }
    }

    // ---- Obsługa L→P: odpowiedzi od masterów ----
    handleLHS(Serial4, SERIAL4_NEI);
    handleLHS(Serial3, SERIAL3_NEI);

    // ---- Timeout oczekiwania na mastera ----
    if (awaiting_res && (millis() - sent > LHS_TIMEOUT)) {
        uint8_t dead_node = (awaiting_res >> 1);
        memset(Tx_buff, 0, BUFF_LEN);
        SET_DST(Tx_buff, W0_ID);
        SET_TYP(Tx_buff, MSG_T_DEAD);
        SET_SRC(Tx_buff, MY_ID);
        SET_DEAD(Tx_buff, dead_node);
        SET_CRC(Tx_buff, calcCRC8(Tx_buff, DEAD_LEN - 1));

        Serial1.write(Tx_buff, DEAD_LEN);
        triggerLedState(GENERIC_ERROR);
        awaiting_res = 0;
    }
}
