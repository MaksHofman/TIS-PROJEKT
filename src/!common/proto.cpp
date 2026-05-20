#include "!common/proto.h"

#include <LoRa.h>
#include "!common/led_helper.h"
#include "!common/config.h"
#include "variant.h"

bool isSynced = false;
unsigned long nextTimeSlot = 0;

bool send(Packet *packet) {
    // not our time slot yet — bail out, caller will retry next loop
    // (aggregator always goes through since nextTimeSlot starts at 0)
    if (millis() < nextTimeSlot) {
        return false;
    }

    add_blinks(BLINKS_SEND);

    // actually send packet
    LoRa.beginPacket();
    LoRa.write((const uint8_t*) packet, PACKET_SIZE);
    LoRa.endPacket(true);

    // update next time slot
    nextTimeSlot += EPOCH_TIME;

    // enable receive mode (since beginPacket() disables it)
    LoRa.receive();

    return true;
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
    // TODO: implement
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

        return packetSize;
    } else {
        // Serial.print("WARN: Unexpected packet size: "); Serial.println(packetSize);

        // clear buffer
        while (LoRa.available()) LoRa.read();

        // and somewhat signalize error
        return 0;
    }
}

extern unsigned long nextTimeSlot;
void setupLoRa() {
    LoRa.begin(LORA_FREQ);
    LoRa.setSpreadingFactor(SPREADING_FACTOR);
    LoRa.setSignalBandwidth(SIGNAL_BANDWIDTH);
    LoRa.setPreambleLength(PREAMBLE_LENGTH);
    LoRa.setSyncWord(SYNC_WORD);
    LoRa.enableCrc();
    LoRa.enableInvertIQ();

    LoRa.receive();

    pinMode(A1, INPUT);
    randomSeed(analogRead(A1));
    nextTimeSlot = random(EPOCH_TIME);
}
