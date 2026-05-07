#include <Arduino.h>
#include <sys/_intsup.h>
#include "!common/config.h"
#include "!common/proto.h"
#include "sensor/color_helper.h"
#include "sensor/identity.h"
#include "!common/proto_helper.h"

extern byte txBuff[BUFF_SIZE], rxBuff[BUFF_SIZE];

/* WARNING: trzeba dodać przed wysłaniem TIMESTAMP1 */
void prepRead() {
    clearTx();
    SET_SRC(txBuff, MY_ID);
    SET_RES(txBuff, RES);
    SET_TYPE(txBuff, MSG_T_READ);
    SET_ROUTING(txBuff, 0);
    SET_R(txBuff, readColor(COLOR_RED));
    SET_G(txBuff, readColor(COLOR_GREEN));
    SET_B(txBuff, readColor(COLOR_BLUE));
    SET_NHOP(txBuff, 0);
    // Tutaj GET_LEN(MSG) zadziała, bo mamy "pełną wiadomość"
    SET_SYS_CODE(txBuff, GET_LEN(txBuff), SYS_CODE);
}

bool doIHaveToRespond() {
    return !IS_SENSOR(GET_SRC(rxBuff)) && GET_ROUTING(rxBuff);
}

/* Przed wywołaniem funkcji należy upewnić się, że:
 * 1. Przyszła do nas wiadomość z naszego systemu (validateMsg(...))
 * 2. Wiadomość ma routing kierowany i została nadana przez jeden z węzłów (doIHaveToRespond())
 */
void prepMeasResp(uint8_t len, unsigned long timestamp2) {
    clearTx();

    // Ustawiamy czas jak tylko dostaniemy wiadomość
    SET_TIMESTAMP2(txBuff, timestamp2);

    // Kopiujemy co dostaliśmy do timestampa
    memcpy(txBuff, rxBuff, 8);

    // Kopiujemy ścieżkę wiadomości
    memcpy((void*)(((uint8_t*)(txBuff)) + 12), (void*)(((uint8_t*)(rxBuff)) + 8), len - 8);

    // Zmieniamy nadawcę na nas i typ wiadomości na pomiar RTT
    SET_SRC(txBuff, MY_ID);
    SET_TYPE(txBuff, MSG_T_MEAS_RTT);

    // Odwrócenie ścieżki routingu kierowanego
    uint8_t nhops_in_msg = ((len - 9) * 2) - 1;
    // Ostatni jest 0, więc jest ich o 1 mniej (bo W0 nie zapisuje się jako HOP)
    if (GET_HOP_NODE_ID(rxBuff, nhops_in_msg) == 0)
        nhops_in_msg -= 1;

    SET_NHOP(txBuff, nhops_in_msg);

    for (int i = 1; i <= nhops_in_msg; i++)
        SET_HOP(txBuff, (nhops_in_msg - i + 1), GET_HOP_NODE_ID(rxBuff, i));

}
