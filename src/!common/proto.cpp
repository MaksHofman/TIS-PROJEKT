#include "!common/proto.h"

#include <LoRa.h>
#include "!common/led_helper.h"
#include "!common/config.h"

bool isSynced = false;
unsigned long nextTimeSlot = 0;

void send(Packet *packet) {
    if (!isSynced) {
        Serial.println("WARN: Not synced, dropping packet...");
        return;
    }

    // now wait for our time slot
    // for aggregator this will always be false, thus it will never wait
    while (millis() < nextTimeSlot) {
        ;
    }

    add_blinks(BLINKS_SEND);

    // actually send packet
    LoRa.beginPacket();
    LoRa.write((const uint8_t*) packet, PACKET_SIZE);
    LoRa.endPacket();

    // update next time slot
    nextTimeSlot += EPOCH_TIME;

    // enable receive mode (since beginPacket() disables it)
    LoRa.receive();
}

unsigned long getOffsetToOurSlot(Node receivedSourceId) {
    int slotsToWait = ((MY_ID - receivedSourceId) % NUM_NODES);

    if (slotsToWait < 0)
        slotsToWait += NUM_NODES;

    // adjust to the begining of our slot
    slotsToWait--;

    return slotsToWait * MSG_TIME;
}

void validatePacket(Packet *packet) {

}

size_t receive(Packet *packet) {
    int packetSize = LoRa.parsePacket();
    if (packetSize == PACKET_SIZE) {
        add_blinks(BLINKS_RECEIVE);

        LoRa.readBytes((char*) packet, PACKET_SIZE);

        if (MY_ID != Node::AGGREGATOR) {
            // save offset to our next time slot
            nextTimeSlot = millis() + getOffsetToOurSlot(packet->src);
            isSynced = true;
        }
    } else {
        Serial.print("WARN: Unexpected packet size: "); Serial.println(packetSize);
    }
}

void setupLoRa() {
    LoRa.begin(LORA_FREQ);
    LoRa.setSpreadingFactor(SPREADING_FACTOR);
    LoRa.setSignalBandwidth(SIGNAL_BANDWIDTH);
    LoRa.setPreambleLength(PREAMBLE_LANGTH);
    LoRa.setSyncWord(SYNC_WORD);
    LoRa.enableCrc();
    LoRa.enableInvertIQ();

    LoRa.receive();
}
