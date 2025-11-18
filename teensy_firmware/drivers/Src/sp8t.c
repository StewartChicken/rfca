
#include "../Inc/sp8t.h"

void sp8t_init() {
    pinMode(PIN_ENABLE, OUTPUT);
    pinMode(PIN_LS, OUTPUT);
    pinMode(PIN_V1, OUTPUT);
    pinMode(PIN_V2, OUTPUT);
    pinMode(PIN_V3, OUTPUT);
}

// Open specified port and close the rest
// Takes argument values 1-8
void sp8t_enablePort(uint8_t port) {
    
    // Always LOW
    digitalWrite(PIN_ENABLE, LOW);
    digitalWrite(PIN_LS, LOW);

    // Subtract one for bit masking
    uint8_t port_mux = port - 1;

    uint8_t v1_val = port_mux & v1_msk; 
    uint8_t v2_val = port_mux & v2_msk; 
    uint8_t v3_val = port_mux & v3_msk; 

    // MUX pins
    digitalWrite(PIN_V1, v1_val);
    digitalWrite(PIN_V2, v2_val);
    digitalWrite(PIN_V3, v3_val);
}