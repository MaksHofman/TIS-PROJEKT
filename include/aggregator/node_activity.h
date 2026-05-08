#ifndef __NODE_ACTIVITY_H__
#define __NODE_ACTIVITY_H__

#define GET_ACTIVE(ACT_BUFF,NODE_ID)            (((ACT_BUFF) >> ((NODE_ID) - 1)) & 1)
#define SET_ACTIVE(ACT_BUFF,NODE_ID,STATE)      ((ACT_BUFF) = ((ACT_BUFF) & ~(1 << (uint8_t)((NODE_ID) - 1))) | \
                                                                            ((STATE) << ((NODE_ID) - 1)))

struct hop {
    uint8_t hop;
    struct hop *next;
};

struct awaitingRTT {
    uint8_t timestamp1;
    struct hop *hop;
    struct awaitingRTT *next;
};

struct awaitingRTT* processRead();
void freeHops(struct hop *hop);
void freeAwaitingRTT(struct awaitingRTT *first);


#endif