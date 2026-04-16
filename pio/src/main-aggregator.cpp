#include <Arduino.h>
#include "uart_helper.h"
#include "const_defs.h"
char cmd;

void setup() {
    Serial.begin(115200);
    while (!Serial);

    // sensor nodes
    Serial1.begin(UART_BAUD);

    setup_sercoms();
}

void loop() {
    if (Serial.available()) {

        Serial.readBytes(&cmd, 1);
        Serial1.write(cmd);
    }

    if (Serial1.available()) {
        Serial1.readBytes(&cmd, 1);
        Serial.println(cmd);
    }
}
