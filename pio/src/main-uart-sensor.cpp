#include <Arduino.h>
#include "uart_helper.h"
#include "const_defs.h"

void setup() {

    // slave
    Serial1.begin(UART_BAUD);

    setup_sercoms();
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
    // if something came over Serial1 then it's from pc node and needs to be forwarded to aggregators
    if (Serial1.available()) {
        char cmd; Serial1.readBytes(&cmd, 1);

        switch (cmd) {
            case '1':
                Serial1.write('a');
                break;
        }
    }
}
