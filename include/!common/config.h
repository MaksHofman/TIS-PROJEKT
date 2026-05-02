#ifndef __CONFIG_H__
#define __CONFIG_H__

/* Defining DEBUG makro enables additional verbouse output in nodes */
#define DEBUG

/*
 * LoRa settings:
 * LORA_FREQ: default 868E6 (kanały co ok. 200kHz 1% duty cycle; 869.525E6 - kanał 10% duty cycle i 500 mW)
 * SPREADING_FACTOR: 6-12 (6 only with implicit header both for send and receive; default 7)
 * SIGNAL_BANDWIDTH: 7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, 250E3, and 500E3 (bandwidth in Hz; default 125 kHz)
 * PREAMBLE_LENGTH: 6-65535 (default 8)
 * SYNC_WORD: differetiating between networks
 */
#define LORA_FREQ 868E6
#define SPREADING_FACTOR 7
#define SIGNAL_BANDWIDTH 125E3
#define PREAMBLE_LANGTH 8
#define SYNC_WORD 0x12

/* 
 * Parametry protokołu
 */
#define MAX_NHOPS 1

#endif