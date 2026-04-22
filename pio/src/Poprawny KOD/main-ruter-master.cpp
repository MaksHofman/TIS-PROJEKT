/*
 * W1 / W2 - Router Master (węzeł łączący slave'y z forwarderami)
 * Sprzęt: Arduino MKR WAN 1310
 *
 * Serial1 → magistrala BUS (slave'y S1/S4/S5 dla W1, S2/S3 dla W2)
 * Serial3 → forwarder W3 (lub W4 dla W2)
 * Serial4 → forwarder W4 (lub W3 dla W2)
 *
 * NIE ZMIENIAMY podłączenia sprzętowego.
 */

#include <Arduino.h>
#include "led_helper.h"
#include "uart_helper.h"
#include "const_defs.h"
#include "proto_defs.h"

// ===== KONFIGURACJA - zmień MY_ID dla W2 =====
#define MY_ID W1_ID

#if MY_ID == W2_ID
  #define MY_SLAVE1    S2_ID
  #define MY_SLAVE2    S3_ID
  #define SERIAL3_NEI  W4_ID
  #define SERIAL4_NEI  W3_ID
#elif MY_ID == W1_ID
  #define MY_SLAVE1    S1_ID
  #define MY_SLAVE2    S4_ID
  #define MY_SLAVE3    S5_ID
  #define SERIAL3_NEI  W3_ID
  #define SERIAL4_NEI  W4_ID
#endif

#define SLAVE_TIMEOUT 2000   // ms - czas oczekiwania na odpowiedź slave'a

// ===== Globalne bufory =====
byte Tx_buff[BUFF_LEN];
byte Rx_buff[BUFF_LEN];
byte ret_buf[RET_LEN];

uint8_t  awaiting_res = 0;   // 0 = brak oczekiwania; inaczej: bit0=1, bity1..7 = ID slave'a
unsigned long sent    = 0;

// ===== Pomocnik: sprawdź czy DST należy do naszych slave'ów =====
static inline bool isMyslave(uint8_t dst) {
    if (dst == MY_SLAVE1 || dst == MY_SLAVE2) return true;
#ifdef MY_SLAVE3
    if (dst == MY_SLAVE3) return true;
#endif
    return false;
}

/*
 * handleRHS - obsługa odebrania wiadomości od forwarderów (Serial3 / Serial4)
 * "Right Hand Side" = strona W0 → forwarder → my
 */
void handleRHS(Uart &from, uint8_t from_id) {
    while (from.available() > 0) {
        uint8_t typ = (from.peek() & 0x0E) >> 1;
        uint8_t exp_len = 0;

        switch (typ) {
            case MSG_T_REQ:  exp_len = REQ_LEN;  break;
            case MSG_T_RES:  exp_len = RES_LEN;  break;
            case MSG_T_DEAD: exp_len = DEAD_LEN; break;
            case MSG_T_RET:  exp_len = RET_LEN;  break;
            default:
                from.read(); // śmieciowy bajt
                continue;
        }

        if (from.available() < exp_len) return; // poczekaj na resztę

        triggerLedState(RECEIVING);
        memset(Rx_buff, 0, BUFF_LEN);
        from.readBytes(Rx_buff, exp_len);

        if (!hammingVerify(Rx_buff, exp_len)) {
            // Błąd nie do naprawienia
            while (from.available()) from.read();
            SET_DST(ret_buf, from_id);
            SET_CRC(ret_buf, calcCRC8(ret_buf, RET_LEN - 1));
            from.write(ret_buf, RET_LEN);
            triggerLedState(GENERIC_ERROR);
            return;
        }

        switch (GET_TYP(Rx_buff)) {
            case MSG_T_REQ: {
                uint8_t dst = GET_DST(Rx_buff);
                if (!isMyslave(dst)) break; // nie do nas - pomiń

                triggerLedState(TRANSMITTING);
                awaiting_res = 1 | (dst << 1);
                sent = millis();
                memcpy(Tx_buff, Rx_buff, REQ_LEN);
                Serial1.write(Tx_buff, REQ_LEN);
                break;
            }
            case MSG_T_RET: {
                // Forwarder prosi o retransmisję naszej ostatniej odpowiedzi
                if (GET_DST(Rx_buff) == MY_ID) {
                    // Retransmitujemy ostatnią wiadomość w Tx_buff
                    uint8_t tx_typ = GET_TYP(Tx_buff);
                    if (tx_typ == MSG_T_RES)
                        from.write(Tx_buff, RES_LEN);
                    else if (tx_typ == MSG_T_DEAD)
                        from.write(Tx_buff, DEAD_LEN);
                    triggerLedState(RETRANSMITTING);
                }
                break;
            }
            default:
                break; // inne typy ignorujemy od strony forwardera
        }
    }
}

void setup() {
    Serial1.begin(UART_BAUD);   // BUS do slave'ów
    Serial3.begin(UART_BAUD);   // do forwardera
    Serial4.begin(UART_BAUD);   // do forwardera

    setup_sercoms();

    memset(Tx_buff, 0, BUFF_LEN);
    memset(Rx_buff, 0, BUFF_LEN);
    memset(ret_buf, 0, RET_LEN);

    SET_TYP(ret_buf, MSG_T_RET);
    setLedState(IDLE);
}

void loop() {
    updateLedTask();

    // ---- Obsługa forwarderów (RHS) ----
    handleRHS(Serial4, SERIAL4_NEI);
    handleRHS(Serial3, SERIAL3_NEI);

    // ---- Timeout slave'a ----
    if (awaiting_res && (millis() - sent) > SLAVE_TIMEOUT) {
        triggerLedState(GENERIC_ERROR);
        memset(Tx_buff, 0, BUFF_LEN);
        SET_DST(Tx_buff, W0_ID);
        SET_TYP(Tx_buff, MSG_T_DEAD);
        SET_SRC(Tx_buff, MY_ID);
        SET_DEAD(Tx_buff, (awaiting_res >> 1));
        SET_CRC(Tx_buff, calcCRC8(Tx_buff, DEAD_LEN - 1));

        Serial3.write(Tx_buff, DEAD_LEN);
        Serial4.write(Tx_buff, DEAD_LEN);
        awaiting_res = 0;
        return;
    }

    // ---- Odczyt z BUS slave'ów (LHS) ----
    while (Serial1.available() > 0) {
        // Jeśli nie oczekujemy odpowiedzi - wyrzuć
        if (!awaiting_res) {
            Serial1.read();
            continue;
        }

        // Sprawdź timeout jeszcze raz wewnątrz pętli
        if ((millis() - sent) > SLAVE_TIMEOUT) break;

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

        if (Serial1.available() < exp_len) break; // poczekaj

        triggerLedState(RECEIVING);
        memset(Rx_buff, 0, BUFF_LEN);
        Serial1.readBytes(Rx_buff, exp_len);

        if (!hammingVerify(Rx_buff, exp_len)) {
            while (Serial1.available()) Serial1.read();
            SET_DST(ret_buf, (awaiting_res >> 1));
            SET_CRC(ret_buf, calcCRC8(ret_buf, RET_LEN - 1));
            Serial1.write(ret_buf, RET_LEN);
            triggerLedState(GENERIC_ERROR);
            break;
        }

        switch (GET_TYP(Rx_buff)) {
            case MSG_T_RET:
                // Slave prosi o retransmisję
                if (GET_DST(Rx_buff) == MY_ID) {
                    Serial1.write(Tx_buff, REQ_LEN);
                    triggerLedState(RETRANSMITTING);
                }
                break;

            case MSG_T_RES:
                // Odpowiedź od slave'a - prześlij do forwarderów
                triggerLedState(TRANSMITTING);
                memcpy(Tx_buff, Rx_buff, RES_LEN);
                Serial3.write(Tx_buff, RES_LEN);
                Serial4.write(Tx_buff, RES_LEN);
                awaiting_res = 0;
                break;

            default:
                break;
        }
    }
}
