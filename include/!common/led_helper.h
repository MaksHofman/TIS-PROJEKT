#ifndef __LED_HELPER_H__
#define __LED_HELPER_H__

#include <Arduino.h>

#define BLINKS_RECEIVE          1
#define BLINKS_SEND             2

#define LED_TIMOUT_MS           500

void setupLed();
void add_blinks(uint8_t to_add);
void blink();

#endif
