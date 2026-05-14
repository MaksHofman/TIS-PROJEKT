#ifndef __COLOR_HELPER_H
#define __COLOR_HELPER_H

uint8_t readColor(uint8_t color);

void setupColorSensor();

void calibrateColorSensor();

// TODO: make enum
#define COLOR_RED 0
#define COLOR_GREEN 1
#define COLOR_BLUE 2
#define COLOR_CLEAR 3
#define COLOR_NONE 4
#define TOTAL 5

#define CAL_DELAY 5000

#endif
