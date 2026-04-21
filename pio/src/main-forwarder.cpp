#include <Arduino.h>
// #include <CRC.h>
#include "api/Common.h"
#include "led_helper.h"
#include "uart_helper.h"
#include "const_defs.h"
#include "proto_defs.h"

/* Do ręcznej zmiany W3 lub W4 */
#define MY_ID W3_ID

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

    Serial.print("[LHS] Otrzymano "); 
    Serial.print(recv); 
    Serial.print(" bajtow od wezla ID: 0x"); 
    Serial.println(from_id, HEX);

    // setLedState(RECEIVING);

    // Coś mi się czuje, że gdzieś zgubię tą synchronizację, o której pisałem przy robieniu protokołu...
    from.readBytes(Rx_buff, min(BUFF_LEN, recv));
    setLedState(IDLE);
    setLedState(NONE);

    // No tak, rcv - 1, nieco upraszcz logikę (mam nadzieję, że synchronizacja nie zrobi brrr)
    // Trzeba na Koncentratorze podpiętym do PC zadbać o to, że jak przyjmie komendę,
    // to przed upływem timeout'u lub otrzymaniem odpowiedzi nic nie wyśle
    crc_valid = GET_CRC(Rx_buff) == calcCRC8(Rx_buff, recv - 1);
    if (!crc_valid) {
        Serial.println("[LHS] [BŁĄD] Nieprawidlowe CRC! Wysylam RET.");
        // setLedState(GENERIC_ERROR);
        SET_DST(ret_buf, from_id);
        SET_CRC(ret_buf, calcCRC8(ret_buf, RET_LEN - 1));
        from.write(ret_buf, RET_LEN);
        return;
    }

    // setLedState(TRANSMITTING);

    uint8_t msg_type = GET_TYP(Rx_buff);
    Serial.print("[LHS] CRC OK. Typ wiadomosci: 0x");
    Serial.println(msg_type, HEX);

    switch (msg_type) {
        // Nic innego w tamtą stronę nie wysyłamy, chyba że coś poszło grubo nie tak...
        case MSG_T_RET:
            Serial.println("[LHS] Zdekodowano MSG_T_RET. Ponawiam wysylanie Tx_buff.");
            from.write(Tx_buff, REQ_LEN);
            break;
        case MSG_T_RES:
            Serial.println("[LHS] Zdekodowano MSG_T_RES. Przekazuje do Serial1 i koncze oczekiwanie.");
            memcpy(Tx_buff, Rx_buff, RES_LEN);
            Serial1.write(Tx_buff, RES_LEN);
            setLedState(GENERIC_ERROR);
            setLedState(NONE);
            // Nie chce mi się już sprawdzać, czy to jest ten response,
            // na który czekamy, no nic innego się chyba nie pojawi...
            awaiting_res = 0;
            break;
        case MSG_T_DEAD:
            Serial.println("[LHS] Zdekodowano MSG_T_DEAD. Przekazuje blad do Serial1.");
            memcpy(Tx_buff, Rx_buff, DEAD_LEN);
            Serial1.write(Tx_buff, DEAD_LEN);
            setLedState(GENERIC_ERROR);
            setLedState(NONE);
            break;
        default: 
            Serial.println("[LHS] [UWAGA] Nieznany typ wiadomosci!");
            break;
    }
}

void setup() {
    // --- Inicjalizacja portu do debugowania ---
    Serial.begin(115200);
    // while(!Serial); // Odkomentuj, jeśli chcesz, by program czekał na otwarcie Serial Monitora (przydatne przy testach)
    
    Serial.println("\n==================================");
    Serial.println("Uruchamianie urzadzenia...");
    #if MY_ID==W4_ID
        Serial.println("Konfiguracja: W4");
    #elif MY_ID==W3_ID
        Serial.println("Konfiguracja: W3");
    #endif
    Serial.println("==================================\n");

    Serial1.begin(UART_BAUD);
    Serial3.begin(UART_BAUD);
    Serial4.begin(UART_BAUD);

    setup_sercoms();
    memset((void*) Tx_buff, 0, BUFF_LEN);
    memset((void*) Rx_buff, 0, BUFF_LEN);
    memset((void*) ret_buf, 0, RET_LEN);
    SET_TYP(ret_buf, MSG_T_RET);
    
    Serial.println("[SYSTEM] Setup zakonczony.");
}

void loop() {
    setLedState(NONE);

    // Z prawej do lewej
    uint8_t recvR = Serial1.available();
    uint8_t crc_valid = 0;
    uint8_t req_type;

    if (!(recvR >= REQ_LEN))
        goto left_to_right; // WARNING: goto

    Serial.print("[P->L] Serial1 otrzymal "); 
    Serial.print(recvR); 
    Serial.println(" bajtow.");

    // setLedState(RECEIVING);

    memset(Rx_buff, 0, BUFF_LEN);
    if (!recvR)
        return;
    Serial1.readBytes(Rx_buff, min(recvR, BUFF_LEN));
    setLedState(IDLE);
    setLedState(NONE);

    // Z tej strony spodziewamy się tylko req lub ret, więc mamy szczęście, że obydwie są po 2B
    crc_valid = GET_CRC(Rx_buff) == calcCRC8(Rx_buff, RES_LEN - 1);
    if (!crc_valid) {
        Serial.println("[P->L] [BŁĄD] Nieprawidlowe CRC od prawej strony. Wysylam RET do W0.");
        // setLedState(GENERIC_ERROR);
        SET_DST(ret_buf, W0_ID);
        SET_CRC(ret_buf, calcCRC8(ret_buf, RET_LEN - 1));
        Serial1.write(ret_buf, RET_LEN);
        setLedState(GENERIC_ERROR);
        setLedState(NONE);
        goto left_to_right; // WARNING: goto
    }

    req_type = GET_TYP(Rx_buff);
    Serial.print("[P->L] CRC OK. Typ wiadomosci: 0x");
    Serial.println(req_type, HEX);

    // CRC w porządku, mamy jakąś wiadomość (REQ lub RET)
    switch (req_type) {
        case MSG_T_REQ: {
            // od razu przekopiuję do Tx_buff
            memcpy(Tx_buff, Rx_buff, REQ_LEN);
            uint8_t dst_id = GET_DST(Tx_buff);
            
            Serial.print("[P->L] Zdekodowano MSG_T_REQ. Cel: 0x");
            Serial.println(dst_id, HEX);

            // Dostaliśmy request, trzeba ustalić dokąd
            if (THROUGH_SERIAL4(dst_id)) {
                Serial.println("[P->L] Przekazuje przez Serial4.");
                Serial4.write(Tx_buff, REQ_LEN);
                setLedState(GENERIC_ERROR);
                setLedState(NONE);
            } else if (THROUGH_SERIAL3(dst_id)) {
                Serial.println("[P->L] Przekazuje przez Serial3.");
                Serial3.write(Tx_buff, REQ_LEN);
                setLedState(GENERIC_ERROR);
                setLedState(NONE);
            } else {
                Serial.println("[P->L] [UWAGA] Nieznany cel! Zapalam diode statusowa.");
                // IDK, jakaś diodka statusowa
            }
            sent = millis();
            // W najgorszym przypadku zrobimy 3x tą samą operację (GET_DST()), ale zaoszczędziliśmy cały 1B RAM'u
            awaiting_res = 1 | (dst_id << 1);
            Serial.print("[SYSTEM] Oczekuje na odpowiedz. Flaga awaiting_res: ");
            Serial.println(awaiting_res, BIN);
            break;
        }
        case MSG_T_RET: {
            Serial.println("[P->L] Zdekodowano MSG_T_RET.");
            // DEAD lub RES (niestety różna długość, ale co to dla zagnieżdżonego switch'a)
            uint8_t tx_type = GET_TYP(Tx_buff);
            switch (tx_type) {
                case MSG_T_DEAD: 
                    Serial.println("[P->L] Ponawiam MSG_T_DEAD do Serial1.");
                    Serial1.write(Tx_buff, DEAD_LEN);
                    setLedState(GENERIC_ERROR);
                    setLedState(NONE);
                    break;
                case MSG_T_RES: 
                    Serial.println("[P->L] Ponawiam MSG_T_RES do Serial1.");
                    Serial1.write(Tx_buff, RES_LEN); 
                    setLedState(GENERIC_ERROR);
                    setLedState(NONE);
                    break;
                default: 
                    Serial.println("[P->L] [UWAGA] RET odebrany, ale Tx_buff nie ma ani DEAD ani RES!");
                    break;
            }
            break;
        }
        default: 
            Serial.println("[P->L] [UWAGA] Nieoczekiwany typ wiadomosci z prawej!");
            break;
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
        uint8_t dead_node = (awaiting_res >> 1);
        Serial.print("[TIMEOUT] Minelo LHS_TIMEOUT! Brak odpowiedzi od wezla 0x");
        Serial.println(dead_node, HEX);
        Serial.println("[TIMEOUT] Wysylam wiadomosc MSG_T_DEAD do Serial1.");

        memset(Tx_buff, 0, BUFF_LEN);
        SET_DST(Tx_buff, W0_ID);
        SET_TYP(Tx_buff, MSG_T_DEAD);
        SET_SRC(Tx_buff, MY_ID);
        SET_DEAD(Tx_buff, dead_node);
        SET_CRC(Tx_buff, calcCRC8(Tx_buff, DEAD_LEN - 1 ));
        Serial1.write(Tx_buff, DEAD_LEN);
        setLedState(GENERIC_ERROR);
        setLedState(NONE);
        
        awaiting_res = 0; // Warto prawdopodobnie zrzucić flagę, żeby nie wysyłać DEAD w kółko co loop (dodałem)
    }
}