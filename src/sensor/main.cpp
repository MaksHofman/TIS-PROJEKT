#include "!common/config.h"
#include "!common/led_helper.h"
#include "!common/log.h"
#include "!common/proto.h"
#include "!common/state.h"
#include "sensor/color_helper.h"

extern const Node MY_ID = Node::SENSOR;

enum class SensorState {
    IDLE,
    PROCESS_PACKET,
    SEND_COLOR_MEASUREMENT,
    RESPOND_TIME_TRACE
};
SensorState currentState = SensorState::IDLE;
unsigned long lastColorSent = 0;

void setup() {
    BEGIN_DEBUG;

    // setup color sensor
    setupColorSensor();
    calibrateColorSensor();

    // setup builtin led
    setupLed();

    // setup LoRa
    setupLoRa();
}

void loop() {
    // Mruganko
    blink();

    Packet rxPacket;
    switch (currentState) {
        case SensorState::IDLE: {
            // network is most important, so handle it first
            if (receive(&rxPacket)) {
                // got any packet
                currentState = SensorState::PROCESS_PACKET;
            } else if (millis() - lastColorSent > READ_TIMEOUT) {
                // no packet and we haven't sent color in a while
                currentState = SensorState::SEND_COLOR_MEASUREMENT;
            }
            break;
        }
        case SensorState::PROCESS_PACKET: {
            if (rxPacket.type == PacketType::TRACE_REQ) {
                currentState = SensorState::RESPOND_TIME_TRACE;
            } else {
                DEBUG("Unknown packet type: "); DEBUGLN(rxPacket.type);
                currentState = SensorState::IDLE;
            }
            break;
        }
        case SensorState::SEND_COLOR_MEASUREMENT: {
            Packet packet {
                .src = MY_ID,
                .dst = Node::AGGREGATOR,
                .type = PacketType::COLOR_MEAS,
                .data = {
                    .colorMeasurement = DataColorMeasurement {
                        .red = readColor(COLOR_RED),
                        .green = readColor(COLOR_GREEN),
                        .blue = readColor(COLOR_BLUE),
                    }
                }
            };
            send(&packet);

            currentState = SensorState::IDLE;
            break;
        }
        case SensorState::RESPOND_TIME_TRACE: {
            Packet packet {
                .src = MY_ID,
                .dst = Node::AGGREGATOR,
                .type = PacketType::TRACE_RES,
                .data = {
                    .traceResponse = DataTraceResponse {
                        .timestamp = static_cast<uint16_t>(millis())
                    }
                }
            };
            send(&packet);

            currentState = SensorState::IDLE;
            break;
        }
    }
}
