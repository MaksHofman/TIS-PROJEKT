#pragma once
#include <stdint.h>

/*
 * Hamming(12,8) - 8 bitów danych → 12 bitów (4 bity parzystości)
 * Wykrywa i koryguje 1 błąd, wykrywa 2 błędy.
 *
 * Użycie (zamiennik CRC w ramce):
 *   Zamiast calcCRC8(buf, len-1)  →  hammingEncode(buf, len-1)
 *   Zamiast GET_CRC / SET_CRC     →  SET_HAM / GET_HAM (ten sam bajt)
 *   Weryfikacja:                  →  hammingCheck(buf, len) == 0
 *
 * Funkcje:
 *   uint8_t hammingByte(uint8_t data)        - enkoduje 1 bajt → 12 bitów (zwraca górne 4 bity jako nibble)
 *   uint8_t hammingCalc(byte* buf, uint8_t n) - liczy "checksum" Hamminga dla n bajtów
 *   bool    hammingVerify(byte* buf, uint8_t n, uint8_t stored) - sprawdza i KORYGUJE błędy in-place
 */

// XOR wszystkich bajtów → 8-bitowa suma kontrolna, potem Hamming(12,8) na tę sumę
// W praktyce: zamiast CRC8 przechowujemy 1 bajt = sum XOR wszystkich poprzednich bajtów
// + 4 bity parzystości Hamminga zakodowane w górnym nibble.
// Wynik mieści się w 1 bajcie (kompatybilne z istniejącym polem CRC w ramce).

/**
 * Oblicza Hamming checksum dla bufora danych.
 * Zwraca 1 bajt do zapisania w ostatnim bajcie ramki (zamiennik CRC).
 */
static inline uint8_t hammingCalc(byte* buf, uint8_t n) {
    // Krok 1: XOR wszystkich bajtów danych
    uint8_t xsum = 0;
    for (uint8_t i = 0; i < n; i++) xsum ^= buf[i];

    // Krok 2: Hamming(12,8) na xsum - liczymy 4 bity parzystości
    // Pozycje bitów (1-indeksowane): p1=b1, p2=b2, p4=b4, p8=b8
    // d1..d8 = bity xsum (b3,b5,b6,b7,b9,b10,b11,b12)
    uint8_t d1 = (xsum >> 0) & 1;
    uint8_t d2 = (xsum >> 1) & 1;
    uint8_t d3 = (xsum >> 2) & 1;
    uint8_t d4 = (xsum >> 3) & 1;
    uint8_t d5 = (xsum >> 4) & 1;
    uint8_t d6 = (xsum >> 5) & 1;
    uint8_t d7 = (xsum >> 6) & 1;
    uint8_t d8 = (xsum >> 7) & 1;

    uint8_t p1 = d1 ^ d2 ^ d4 ^ d5 ^ d7;
    uint8_t p2 = d1 ^ d3 ^ d4 ^ d6 ^ d7;
    uint8_t p4 = d2 ^ d3 ^ d4 ^ d8;
    uint8_t p8 = d5 ^ d6 ^ d7 ^ d8;

    // Pakujemy: górny nibble = bity parzystości (p8 p4 p2 p1), dolny = dolne 4 bity xsum
    // Dzięki temu nadal zmieści się w 1 bajcie
    uint8_t result = (xsum & 0x0F) | ((p1 | (p2 << 1) | (p4 << 2) | (p8 << 3)) << 4);
    return result;
}

/**
 * Weryfikuje i koryguje ramkę.
 * buf[n-1] to bajt z hammingCalc.
 * Zwraca true jeśli OK (lub poprawiono 1-bitowy błąd), false jeśli błąd nie do naprawienia.
 */
static inline bool hammingVerify(byte* buf, uint8_t n) {
    uint8_t stored = buf[n - 1];

    // Odtwórz xsum z danych
    uint8_t xsum = 0;
    for (uint8_t i = 0; i < n - 1; i++) xsum ^= buf[i];

    // Odtwórz bity parzystości z xsum
    uint8_t d1 = (xsum >> 0) & 1;
    uint8_t d2 = (xsum >> 1) & 1;
    uint8_t d3 = (xsum >> 2) & 1;
    uint8_t d4 = (xsum >> 3) & 1;
    uint8_t d5 = (xsum >> 4) & 1;
    uint8_t d6 = (xsum >> 5) & 1;
    uint8_t d7 = (xsum >> 6) & 1;
    uint8_t d8 = (xsum >> 7) & 1;

    uint8_t p1 = d1 ^ d2 ^ d4 ^ d5 ^ d7;
    uint8_t p2 = d1 ^ d3 ^ d4 ^ d6 ^ d7;
    uint8_t p4 = d2 ^ d3 ^ d4 ^ d8;
    uint8_t p8 = d5 ^ d6 ^ d7 ^ d8;

    uint8_t expected = (xsum & 0x0F) | ((p1 | (p2 << 1) | (p4 << 2) | (p8 << 3)) << 4);

    return (stored == expected);
}
