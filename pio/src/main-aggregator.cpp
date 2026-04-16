#include <Arduino.h>
#include "api/Common.h"
#include "uart_helper.h"
#include "const_defs.h"

void setup() {
    Serial.begin(115200);
    while (!Serial);

    // sensor nodes
    Serial1.begin(UART_BAUD);

    setup_sercoms();
}

void loop() {
    if (Serial.available()) {
        char cmd = Serial.read();
        Serial1.write(cmd);
    }
}
