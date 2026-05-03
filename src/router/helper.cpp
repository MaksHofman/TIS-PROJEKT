#include <Arduino.h>
#include "!common/config.h"
#include "!common/proto.h"

extern byte rxBuff[BUFF_SIZE], txBuff[BUFF_SIZE];

void clearRx() {
    memset((void*)rxBuff, 0, BUFF_SIZE);
}

void clearTx() {
    memset((void*)txBuff, 0 , BUFF_SIZE);
}

uint8_t validateMsg(uint8_t* msg, size_t len) {
    /* Zwraca 0 jeśli wiadomość jest poprawna, 1 jeśli jest za krótka, 2 jeśli nie zgadzają się SYS_ID lub res */
    if( len < _MIN_READ_LEN)
        return 1;
    if(!MSG_VALID(rxBuff, len) )
        return 2;
    return 0;
}