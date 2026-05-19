#include "!common/led_helper.h"
#include "!common/log.h"
#include "!common/proto.h"
#include "!common/state.h"

// change per flashed node
extern const Node MY_ID = Node::W1;

enum class RouterState {
    IDLE,
    PROCESS_PACKET,
    FORWARD_PACKET
};
RouterState currentState = RouterState::IDLE;

void setup() {
    BEGIN_DEBUG;

    // setup builtin led
    setupLed();

    // setup LoRa
    setupLoRa();
}

void loop() {
    blink();

    static Packet rxPacket = { };
    static Packet txPacket = { };
    switch (currentState) {
        case RouterState::IDLE: {
            if (receive(&rxPacket)) {
                // got any packet
                currentState = RouterState::PROCESS_PACKET;
            }
            break;
        }
        case RouterState::PROCESS_PACKET: {
            // we forward all packets, so skip checking the type

            // copy received to txPacket;
            txPacket = rxPacket;

            // check for forwarding-loop (so if we've already forwarded this packet)
            bool haveIAlreadyForwardedThisPacket = false;
            for (auto hop : txPacket.hops) {
                if (hop == MY_ID) {
                    haveIAlreadyForwardedThisPacket = true;
                    break;
                }
            }
            if (haveIAlreadyForwardedThisPacket) {
                currentState = RouterState::IDLE;
                break;
            }

            // check if we're at max hops
            if (txPacket.numHops == MAX_HOPS) {
                DEBUGLN("WARN: Max hops reached, dropping packet");
                currentState = RouterState::IDLE;
                break;
            }

            // update hops
            // TODO: check if this line works
            txPacket.hops[txPacket.numHops++] = MY_ID;

            // finally packet is ready, send it
            currentState = RouterState::FORWARD_PACKET;
            break;
        }
        case RouterState::FORWARD_PACKET: {
            // packet is already prepared by PROCESS_PACKET
            // try send and only return to IDLE on successful send
            if (send(&txPacket)) {
                currentState = RouterState::IDLE;
            }
            break;
        }
    }
}
