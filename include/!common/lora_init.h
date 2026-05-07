#ifndef __LORA_INIT_H__
#define __LORA_INIT_H__

#define SETUP_LORA do { \
    LoRa.begin(LORA_FREQ); \
    LoRa.setSpreadingFactor(SPREADING_FACTOR); \
    LoRa.setSignalBandwidth(SIGNAL_BANDWIDTH); \
    LoRa.setPreambleLength(PREAMBLE_LANGTH); \
    LoRa.setSyncWord(SYNC_WORD); \
    LoRa.enableCrc(); \
    LoRa.enableInvertIQ(); \
} while(0)

#endif