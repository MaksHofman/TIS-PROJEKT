#include <Arduino.h>
#include <cstdint>
#include <cstring>
#include "!common/config.h"
#include "!common/proto.h"
#include "!common/proto_helper.h"
#include "aggregator/node_activity.h"
#include "aggregator/identity.h"

extern byte rxBuff[BUFF_SIZE], txBuff[BUFF_SIZE];
extern unsigned long seenMsgs[MSGS_TO_REMEMBER];

/* Przyjmuje pierwszy hop i usuwa je wszystkie */
void freeHops(struct hop *first) {
    struct hop *current = first;
    struct hop *next_node;

    while (current != NULL) {
        // Zapisujemy adres następnego elementu
        next_node = current->next;
        // Zwalniamy aktualny element
        free(current);
        // przechodzimy do następnego
        current = next_node;
    }
}

/* Przyjmuje pierwszy hop i usuwa je wszystkie */
void freeAwaitingRTT(struct awaitingRTT *first) {
    struct awaitingRTT *current = first;
    struct awaitingRTT *next_node;

    while (current != NULL) {
        // Zapisujemy adres następnego elementu
        next_node = current->next;
        // Zwalniamy hopy
        freeHops(current->hop);
        // Zwalniamy aktualny element
        free(current);
        // przechodzimy do następnego
        current = next_node;
    }
}

struct awaitingRTT* processRead() {
    int num_hops = GET_NHOP(rxBuff);

    // Zabezpieczenie przed brakiem hopów
    if (num_hops < 1) {
        return NULL;
    }

    // Alokujemy i od razu poprawnie inicjalizujemy pierwszy element
    struct hop *firstHop = (struct hop*)malloc(sizeof(struct hop));
    if (firstHop == NULL) {
        return NULL;
    }
    firstHop->hop = GET_HOP_NODE_ID(rxBuff, 1);
    firstHop->next = NULL;

    // currHop będzie zawsze wskazywał na OSTATNI dodany do listy element (tzw. tail)
    struct hop *currHop = firstHop;

    // Przechodzimy przez resztę elementów
    for (int i = 2; i <= num_hops; i++) {
        // Alokujemy nowy "następny" węzeł
        struct hop *nextHop = (struct hop*)malloc(sizeof(struct hop));
        
        if (nextHop == NULL) {
            freeHops(firstHop);
            return NULL;
        }

        // Inicjalizujemy całkowicie nowy element
        nextHop->hop = GET_HOP_NODE_ID(rxBuff, i);
        nextHop->next = NULL;

        // Skoro nowy element jest gotowy, podpinamy go do ostatniego...
        currHop->next = nextHop;
        
        // ...i przesuwamy "wskaźnik ostatniego elementu" na ten nowo dodany
        currHop = nextHop;
    }

    struct awaitingRTT *read = (struct awaitingRTT*)malloc(sizeof(struct awaitingRTT));
    if (read == NULL) {
        freeHops(firstHop);
        return NULL;
    }
    read->timestamp1 = GET_TIMESTAMP1(rxBuff);
    read->hop = firstHop;
    read->next = NULL;

    return read;
}

bool haveISeenIt() {
    unsigned long it = GET_TIMESTAMP1(rxBuff); 
    for (int i = 0; i < MSGS_TO_REMEMBER; i++) {
        if (seenMsgs[i] == it)
            return true;
    }
    return false;
}

void seeItNow() {
    memmove(&seenMsgs[1], &seenMsgs[0], (MSGS_TO_REMEMBER - 1) * sizeof(unsigned long));
    seenMsgs[0] = GET_TIMESTAMP1(rxBuff);
}

void printPath(uint8_t len) {
    uint8_t type = GET_TYPE(rxBuff);
    uint8_t nhops_in_msg = 0;

    if (type == MSG_T_READ)
        nhops_in_msg = ((len - 9) * 2) - 1;
    else if (type == MSG_T_MEAS_RTT)
        nhops_in_msg = ((len - 13) * 2) - 1;

    // Ostatni jest 0, więc jest ich o 1 mniej (bo W0 nie zapisuje się jako HOP)
    if (GET_HOP_NODE_ID(rxBuff, nhops_in_msg) == 0)
        nhops_in_msg -= 1;

    Serial.print(F(", przez: "));
    for (int i = 1; i <= nhops_in_msg; i++) {
        Serial.print(F("W"));
        Serial.print(GET_HOP_NODE_ID(rxBuff, i), DEC);
        if (i != nhops_in_msg)
            Serial.print(F(", "));
    }

}

void prepMeasResp(uint8_t len) {
    clearTx();

    // kopiujemy całość
    memcpy(txBuff, rxBuff, len);

    // Zmieniamy nadawcę na nas i typ routingu
    SET_SRC(txBuff, MY_ID);
    SET_ROUTING(txBuff, 1);

    // Odwrócenie ścieżki routingu kierowanego
    uint8_t nhops_in_msg = ((len - 9) * 2) - 1;
    // Ostatni jest 0, więc jest ich o 1 mniej (bo W0 nie zapisuje się jako HOP)
    if (GET_HOP_NODE_ID(txBuff, nhops_in_msg) == 0)
        nhops_in_msg -= 1;

    SET_NHOP(txBuff, nhops_in_msg);

    for (int i = 1; i <= nhops_in_msg; i++)
        SET_HOP(txBuff, (nhops_in_msg - i + 1), GET_HOP_NODE_ID(rxBuff, i));

}