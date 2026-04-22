#include "led_helper.h"
#include "hamming_helper.h"
#include "variant.h"

// ---- LED state machine (nieblokująca) ----
unsigned long _led_timer    = 0;
unsigned long _led_duration = 500;   // domyślny czas trwania stanu tymczasowego [ms]
STATE _current_temp_state   = IDLE;
bool  _in_temp_state        = false;

static void _applyColor(uint8_t r, uint8_t g, uint8_t b) {
    analogWrite(LED_PIN_R, r);
    analogWrite(LED_PIN_G, g);
    analogWrite(LED_PIN_B, b);
}

void setupStateLed() {
    pinMode(LED_PIN_R, OUTPUT);
    pinMode(LED_PIN_G, OUTPUT);
    pinMode(LED_PIN_B, OUTPUT);
}

static void _setColor(STATE state) {
    switch (state) {
        case IDLE:          _applyColor(255, 255, 255); break; // biały
        case SLEEP:         _applyColor( 10,  10,  10); break; // szary
        case RECEIVING:     _applyColor(  0,   0, 255); break; // niebieski
        case RETRANSMITTING:_applyColor(  0, 255,   0); break; // zielony
        case TRANSMITTING:  _applyColor(255, 255,   0); break; // żółty
        case GENERIC_ERROR: _applyColor(255,   0,   0); break; // czerwony
        case NONE:          _applyColor(  0,   0,   0); break; // wyłączony
    }
}

void setLedState(STATE state) {
    _current_temp_state = state;
    _in_temp_state      = false;
    _setColor(state);
    // BEZ delay() - nieblokująca!
}

void triggerLedState(STATE state) {
    // Ustaw stan tymczasowy na _led_duration ms, potem wróć do IDLE
    _current_temp_state = state;
    _in_temp_state      = true;
    _led_timer          = millis();
    _setColor(state);
    // BEZ delay() - nieblokująca!
}

void updateLedTask() {
    if (_in_temp_state && (millis() - _led_timer >= _led_duration)) {
        _in_temp_state = false;
        _setColor(IDLE);
    }
}

// ---- Hamming checksum (zamiennik CRC8) ----
// Zgodny interfejs: calcCRC8(buf, len) -> uint8_t
uint8_t calcCRC8(byte* buf, uint8_t len) {
    return hammingCalc(buf, len);
}
