#ifndef __UART_HELPER_H
#define __UART_HELPER_H

#include <Arduino.h>

extern Uart Serial3;

extern Uart Serial4;

/*
 * Should be run after SerialX.begin().
 */
void setup_sercoms();

#endif
