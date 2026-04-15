#include <Arduino.h>
#include "uart_helper.h"
#include "proto_defs.h"
#include "const_defs.h"

void setup() {
    Serial.begin(115200);
    while (!Serial);

    // pc node
    Serial1.begin(UART_BAUD);

    // aggregator nodes
    Serial3.begin(UART_BAUD);
    Serial4.begin(UART_BAUD);

    setup_sercoms();
}

static void forward(Uart &src, const char *src_name, Uart &dst, const char *dst_name) {
    if (!src.available()) return;

    uint8_t buf[RES_LEN];  // max rozmiar ramki
    size_t n = src.readBytes(buf, sizeof(buf));

    Serial.print("[ROUTER] ");
    Serial.print(src_name);
    Serial.print(" -> ");
    Serial.print(dst_name);
    Serial.print(" (");
    Serial.print(n);
    Serial.println("B)");

    dst.write(buf, n);
}

void loop() {
    // if something came over Serial1 then it's from pc node and needs to be forwarded to aggregators
    if (Serial1.available()) {
        uint8_t buf[RES_LEN];
        size_t n = Serial1.readBytes(buf, sizeof(buf));

        if (n > 0) {
            uint8_t dst = GET_DST(buf);
            // TODO: now fowards to both, but should actually use dst
            Serial3.write(buf, n);
            Serial4.write(buf, n);
        }
    }

    // otherwise forward from aggregators to pc node
    forward(Serial3, "AGGR_left",  Serial1, "PC");
    forward(Serial4, "AGGR_right", Serial1, "PC");
}
