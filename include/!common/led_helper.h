#ifndef __LED_HELPER_H__
#define __LED_HELPER_H__

#define LED_TIMOUT_MS           500
#define SETUP_LED               do { \
                                    pinMode(LED_BUILTIN, OUTPUT); \
                                    digitalWrite(LED_BUILTIN, LOW); \
                                } while(0)

void add_blinks(uint8_t to_add);
void blink();

#endif