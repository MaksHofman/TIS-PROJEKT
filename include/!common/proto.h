#ifndef __PROTO_H__
#define __PROTO_H__

#include <Arduino.h>

// between 1 and 5 hops
#define MAX_HOPS 5

enum Node : uint8_t {
    AGGREGATOR,
    W1,
    W2,
    W3,
    W4,
    W5,
    W6,
    SENSOR
};

// number of nodes present in the network
#define NUM_NODES 8

extern const Node MY_ID;

enum PacketType : uint8_t {
    /**
     * Message with color measurement values. Sent from sensor only.
     */
    COLOR_MEAS,

    /**
     * Propagation time measurement request.
     *
     * Contains no data.
     */
    TRACE_REQ,

    /**
     * Propagation time measurement response.
     */
    TRACE_RES
};

typedef struct __attribute__((packed)) {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} DataColorMeasurement;

typedef struct __attribute__((packed)) {
    uint16_t timestamp;
} DataTraceResponse;

typedef struct __attribute__((packed)) {
    // from/to who
    Node src : 3;
    Node dst : 3;

    // payload type and contents
    PacketType type : 2;
    union {
        DataColorMeasurement colorMeasurement;
        DataTraceResponse traceResponse;
    } data;

    // TODO: storing this in a different way should allow for using 2 bytes less per Packet
    uint8_t numHops;
    Node hops[MAX_HOPS];
} Packet;

#define PACKET_SIZE sizeof(Packet)

// TODO: recalculate
#define MSG_TIME      300
#define EPOCH_TIME    (MSG_TIME * NUM_NODES)

bool send(Packet* packet);

size_t receive(Packet* packet);

void setupLoRa();

#endif
