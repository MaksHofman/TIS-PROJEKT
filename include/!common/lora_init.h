#ifndef __LORA_INIT_H__
#define __LORA_INIT_H__

#define SETUP_LORA do { \
    if (!LoRa.begin(LORA_FREQ)) { \
        Serial.println("Starting LoRa failed!"); \
        while (1); \
    } \
    LoRa.setSpreadingFactor(SPREADING_FACTOR); \
    LoRa.setSignalBandwidth(SIGNAL_BANDWIDTH); \
    LoRa.setPreambleLength(PREAMBLE_LANGTH); \
    LoRa.setSyncWord(SYNC_WORD); \
    LoRa.enableCrc(); \
} while(0)

#endif