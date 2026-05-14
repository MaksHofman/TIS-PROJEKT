#include "LoRa.h"
#include "!common/config.h"
#include "!common/proto.h"
#include "!common/proto_helper.h"
#include "!common/led_helper.h"

unsigned long last_master_message_time = 0;

extern byte rxBuff[BUFF_SIZE], txBuff[BUFF_SIZE];

void send(byte* data, size_t size, int node_id) {
    Serial.print(millis()); Serial.println(": send()");
    // master (W0) zawsze może nadawać
    if (node_id != W0_ID && last_master_message_time + 2*EPOCH_TIME < millis()) {
        Serial.println("warning: stale master time. exiting...");
        return;
    }

    add_blinks(BLINKS_SEND);

    unsigned long node_time_window_offset = MSG_TIME * node_id;
    Serial.print("node delay: "); Serial.println(node_time_window_offset);
    while (millis() - last_master_message_time < node_time_window_offset) {
        ;
    }

    LoRa.idle();
    LoRa.beginPacket();
    LoRa.write(data, size);
    LoRa.endPacket();
    LoRa.receive();
}

size_t receive() {
    Serial.print(millis()); Serial.println(": receive()");
    int packet_size = LoRa.parsePacket();
    if (packet_size) {
        add_blinks(BLINKS_RECEIVE);

        clearRx();
        LoRa.readBytes(rxBuff, packet_size);

        if (GET_SRC(rxBuff) == W0_ID) {
            last_master_message_time = millis();
            Serial.print("got sync. last master time: "); Serial.println(last_master_message_time);
        }
    }

    return packet_size;
}
