#include "uart_helper.h"
#include "wiring_private.h"
#include "const_defs.h"

Uart Serial3(&sercom3, SERIAL3_RX, SERIAL3_TX, SERIAL3_RX_PAD, SERIAL3_TX_PAD);
void SERCOM3_Handler() { Serial3.IrqHandler(); }

Uart Serial4(&sercom0, SERIAL4_RX, SERIAL4_TX, SERIAL4_RX_PAD, SERIAL4_TX_PAD);
void SERCOM0_Handler() { Serial4.IrqHandler(); }

void setup_sercoms() {
    pinPeripheral(SERIAL3_TX, SERIAL3_FUNCTION);
    pinPeripheral(SERIAL3_RX, SERIAL3_FUNCTION);

    pinPeripheral(SERIAL4_TX, SERIAL4_FUNCTION);
    pinPeripheral(SERIAL4_RX, SERIAL4_FUNCTION);
}
