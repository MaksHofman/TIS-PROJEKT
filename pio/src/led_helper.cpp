#include "led_helper.h"
#include "USB/USBAPI.h"
#include "variant.h"

unsigned long _led_timer = 0;
unsigned long _led_duration = 0;
STATE _current_temp_state = IDLE;

void setupStateLed() {
    pinMode(LED_PIN_R, OUTPUT);
    pinMode(LED_PIN_G, OUTPUT);
    pinMode(LED_PIN_B, OUTPUT);
}

void setLedColor(uint8_t red, uint8_t green, uint8_t blue) {
    Serial.print("setLedColor: "); Serial.print(red); Serial.print(", "); Serial.print(green); Serial.print(", "); Serial.println(blue);
    analogWrite(LED_PIN_R, red);
    analogWrite(LED_PIN_G, green);
    analogWrite(LED_PIN_B, blue);
}

void setLedState(STATE state) {
    switch (state) {
        case IDLE: {
            // white
            setLedColor(255, 255, 255);
            break;
        }
        case SLEEP: {
            // dim white - call it gray
            setLedColor(10, 10, 10);
            break;
        }
        case RECEIVING: {
            // blue
            setLedColor(0, 0, 255);
            break;
        }
        case RETRANSMITTING: {
            // green (cause blue+yellow)
            setLedColor(0, 255, 0);
            break;
        }
        case TRANSMITTING: {
            // yellow
            setLedColor(255, 255, 0);
            break;
        }
        case GENERIC_ERROR: {
            // red
            setLedColor(255, 0, 0);
            break;
        }
        case NONE: {
            // red
            setLedColor(0, 0, 0);
            break;
        }
    }

    Serial.print("state: ");Serial.print(state);Serial.print("\t");
    Serial.println("delaying");
    delay(1000);
}


uint8_t calcCRC8(byte* buff, uint8_t len) {
    return 1;
}
