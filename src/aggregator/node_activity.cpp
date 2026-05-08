#include <Arduino.h>
#include <cstdlib>
#include "!common/config.h"
#include "!common/proto.h"
#include "aggregator/node_activity.h"

extern byte rxBuff[BUFF_SIZE];

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