#include "!common/led_helper.h"
#include "!common/log.h"
#include "!common/proto.h"

extern const Node MY_ID = Node::SENSOR;

#define TIME_TRACE_TIMEOUT 10000
#define REPORT_TIMEOUT     2000

enum class AggregatorState {
    IDLE,
    PROCESS_PACKET,
    SEND_TIME_TRACE,
    SHOW_REPORT
};
AggregatorState currentState = AggregatorState::IDLE;
unsigned long lastTraceSent = 0;
unsigned long lastReportShown = 0;

bool isNodeAlive[NUM_NODES] = { false };

void setup() {
    BEGIN_DEBUG;

    // setup builtin LED
    setupLed();

    // setup LoRa
    setupLoRa();
}

void loop() {
    blink();

    Packet rxPacket;
    switch (currentState) {
        case AggregatorState::IDLE: {
            if (receive(&rxPacket)) {
                // got any packet
                currentState = AggregatorState::PROCESS_PACKET;
            } else if (millis() - lastTraceSent > TIME_TRACE_TIMEOUT && false /* TODO: disable for now */) {
                // no packet and it's time for a time trace
                currentState = AggregatorState::SEND_TIME_TRACE;
                lastTraceSent = millis();
            } else if (millis() - lastReportShown > REPORT_TIMEOUT) {
                // no packet and it's time for a report
                currentState = AggregatorState::SHOW_REPORT;
                lastReportShown = millis();
            }
            break;
        }
        case AggregatorState::PROCESS_PACKET: {
            if (rxPacket.type == PacketType::COLOR_MEAS) {
                DEBUG("Got color measurement: "); DEBUG("("); DEBUG(rxPacket.data.colorMeasurement.red); DEBUG(", "); DEBUG(rxPacket.data.colorMeasurement.green); DEBUG(", "); DEBUG(rxPacket.data.colorMeasurement.blue); DEBUGLN(")");

                // TODO: check number of hops instead of all
                DEBUG("\tvia: ");
                for (auto node : rxPacket.hops) {
                    DEBUG(node); DEBUG(" ");
                    isNodeAlive[node] = true;   // set alive for report

                }
                DEBUGLN("");
            }
            break;
        }
        case AggregatorState::SEND_TIME_TRACE: {
            // TODO: implement, we won't get here yet
            break;
        }
        case AggregatorState::SHOW_REPORT: {
            // TODO: pretty print
            DEBUG("Report: ");
            for (int i = 0; i < NUM_NODES; i++) {
                DEBUG(i); DEBUG(": ");
                DEBUG(isNodeAlive[i] ? "alive" : "dead");
                DEBUGLN("");

                // set is dead for next report
                isNodeAlive[i] = false;
            }
            break;
        }
    }
}
