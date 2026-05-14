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

uint8_t validateMsg(byte* buffer, size_t len) {
    if (len < _MIN_READ_LEN)
        return 1;

    if (!MSG_VALID(rxBuff, len))
        return 2;

    return 0;
}
