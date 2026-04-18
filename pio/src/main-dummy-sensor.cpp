#include <Arduino.h>
#include <CRC.h>
#include "uart_helper.h"
#include "const_defs.h"
#include "proto_defs.h"

/* Na razie trzeba zmieniać ręcznie, ale to można poprawić */
#define MY_ID S2_ID

/* Master się przypisze sam */
#if MY_ID==S2_ID || MY_ID==S3_ID
#define MASTER_ID W2_ID
#elif MY_ID==S1_ID || MY_ID==S4_ID || MY_ID==S5_ID
#define MASTER_ID W1_ID
#endif

byte Tx_buff[BUFF_LEN], Rx_buff[BUFF_LEN];
byte ret_msg[RET_LEN];

void setup() {
    Serial1.begin(UART_BAUD);
    setup_sercoms();

    memset((void*) Tx_buff, 0, BUFF_LEN);
    memset((void*) Rx_buff, 0, BUFF_LEN);
    /* Coś do usypiania, jak zadziała reszta to można spróbować */
    // // Opcjonalnie: upewnienie się, że tryb głębokiego snu jest wyłączony, 
    // // aby instrukcja WFI usypiała tylko CPU (IDLE mode), a nie całe urządzenie (STANDBY).
    // SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
    /* #TODO: Dodać jakieś ledy RGB do statusu */
    SET_DST(ret_msg, MASTER_ID);
    SET_TYP(ret_msg, MSG_T_RET);
    SET_CRC(ret_msg, calcCRC8(ret_msg, RET_LEN - 1));
}

void loop() {
    uint8_t recv = Serial1.available();
    uint8_t crc_valid = 0;
    if(recv >= REQ_LEN) {
        // Czyszczenia bufora nigdy za dużo
        memset((void*) Rx_buff, 0, BUFF_LEN);
        /* Teoretycznie możemy się akurat wstrzelić tutaj w trakcie odbierania, ale pal 5, może zadziała */
        Serial1.readBytes(Rx_buff, min(recv, BUFF_LEN));
        switch(GET_TYP(Rx_buff)) {
            case MSG_T_REQ:
            /* Tu trzeba przygotować odpowiedź */
            // Odpowiadamy tylko na REQ skierowane do nas z poprawnym CRC8, inaczej prosimy o retransmisję
            crc_valid = calcCRC8(Rx_buff, REQ_LEN - 1) == GET_CRC(Rx_buff);
            // Pomyłka, CRC się zgadza, ale to nie powinno tu trafić
            if(GET_DST(Rx_buff) != MY_ID && crc_valid)
                break;

            // Może akurat trafiło na dst
            if(!crc_valid) {
                Serial1.write(ret_msg, RET_LEN);
                break;
            }

            // Czyścimy bufor nadawczy
            memset((void*) Rx_buff, 0, BUFF_LEN);
            // Odpowiedzi zawsze do W0
            SET_DST(Tx_buff, W0_ID);
            SET_TYP(Tx_buff, MSG_T_RES);
            SET_SRC(Tx_buff, MY_ID);
            // To miało być jako odczyt w dummy węźle
            SET_READ(Tx_buff, MY_ID);
            SET_CRC(Tx_buff, calcCRC8(Tx_buff, RES_LEN - 1));
            
            case MSG_T_RET:
            /* Z węzła dummy można odesłać tylko MSG_T_RES */
            // Jak dostaniemy req nie ret to sprawdzamy 2x to samo, ale co tam, jakość kodu miała się nie liczyć
            if( GET_DST(Rx_buff) == MY_ID &&
                calcCRC8(Rx_buff, RET_LEN - 1) == GET_CRC(Rx_buff) )
                    Serial1.write(Tx_buff, RES_LEN);
            break;
            default:
            /* 
             * Otrzymaliśmy wiadomość, która nie powinna trafić do węzła dummy,
             * można dać tu jeszcze jakieś mryganie LEDEM na czarwono czy coś
             */
            // czyścimy bufor Serial1
            while(Serial1.available())
                Serial1.readBytes(Rx_buff, min(recv,BUFF_LEN));
            // Czyścimy bufor na odebrane wiadomości
            memset((void*) Rx_buff, 0, BUFF_LEN);
        }
        /* #TODO: Sytuacja w której to CRC8 w wiadomości do nas się nie zgada,
         * bo aktualnie to jak przekłamie coś na typie wiadomości to to po prostu olewamy,
         * tak samo jak CRC się nie zgadza (WARNING: Jak przekłamie w 2 strony to może powstać pętla,
         * w której przerzucamy się retransmisją i retransmitujemy retransmisję xD)
         */
    }
    /* Jeszcze raz coś do snu, zobaczymy jak pójdzie bez spania */
    /* Jakiś LED status przed snem? */
    // SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk; 
    // __WFI();

}