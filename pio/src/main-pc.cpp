#include "const_defs.h"
#include "proto_defs.h"
#include "uart_helper.h"
#include <Arduino.h>

void setup() {
    // pc communication side
    Serial.begin(115200);
    while (!Serial);

    Serial1.begin(UART_BAUD);
    Serial3.begin(UART_BAUD);
    setup_sercoms();
}

void loop() {
    // TODO: deduplicate
    if (Serial1.available()) {
        uint8_t buf[RES_LEN];
        size_t n = Serial1.readBytes(buf, sizeof(buf));
        Serial.print("[PC<-RA] bajty: ");
        Serial.println(n);
    }

    if (Serial3.available()) {
        uint8_t buf[RES_LEN];
        size_t n = Serial3.readBytes(buf, sizeof(buf));
        Serial.print("[PC<-RB] bajty: ");
        Serial.println(n);
    }
}
