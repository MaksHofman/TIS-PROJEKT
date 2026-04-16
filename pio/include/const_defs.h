#ifndef __CONST_DEFS_H
#define __CONST_DEFS_H

// color sensor pins
#define S0 2
#define S1 3
#define S2 4
#define S3 5
#define OUT_PIN 6

#define COLOR_RED 0
#define COLOR_GREEN 1
#define COLOR_BLUE 2
#define COLOR_CLEAR 3
#define TOTAL 4

#define CAL_DELAY 5000

// protocol pins
// Serial3 (SERCOM3)
#define SERIAL3_TX 0
#define SERIAL3_RX 1
#define SERIAL3_RX_PAD SERCOM_RX_PAD_1
#define SERIAL3_TX_PAD UART_TX_PAD_0
#define SERIAL3_FUNCTION PIO_SERCOM

// Serial4 (SERCOM0)
#define SERIAL4_TX A3
#define SERIAL4_RX A4
#define SERIAL4_RX_PAD SERCOM_RX_PAD_1
#define SERIAL4_TX_PAD UART_TX_PAD_0
#define SERIAL4_FUNCTION PIO_SERCOM_ALT

#define UART_BAUD   128

#endif
