
#include "../Inc/sp8t.h"

/**
 * @brief Configure Teensy GPIO pins for sp8t peripheral
 * @return: void
 * TODO: Error handling
 */
void sp8t_init() {
    pinMode(SP8T_PIN_ENABLE, OUTPUT);
    pinMode(SP8T_PIN_LS, OUTPUT);
    pinMode(SP8T_PIN_V1, OUTPUT);
    pinMode(SP8T_PIN_V2, OUTPUT);
    pinMode(SP8T_PIN_V3, OUTPUT);
}

/**
 * @brief Open specified port and close the rest
 * @param port - values between 1 and 8
 * @return: void
 * TODO: Error handling
 */
void sp8t_enablePort(const sp8t_port_t port) {
    
    // Always LOW
    digitalWrite(SP8T_PIN_ENABLE, LOW);
    digitalWrite(SP8T_PIN_LS, LOW);

    // Subtract one for bit masking
    uint8_t port_mux = (uint8_t)port - 1;

    uint8_t v1_val = port_mux & SP8T_V1_MSK; 
    uint8_t v2_val = port_mux & SP8T_V2_MSK; 
    uint8_t v3_val = port_mux & SP8T_V3_MSK; 

    // MUX pins
    digitalWrite(SP8T_PIN_V1, v1_val);
    digitalWrite(SP8T_PIN_V2, v2_val);
    digitalWrite(SP8T_PIN_V3, v3_val);
}

/**
 * @brief Set mux_in lines to digital 0
 * @return: void
 * TODO: Error handling
 */
void sp8t_reset() {
    digitalWrite(SP8T_PIN_ENABLE, LOW);
    digitalWrite(SP8T_PIN_LS, LOW);
    digitalWrite(SP8T_PIN_V1, LOW);
    digitalWrite(SP8T_PIN_V2, LOW);
    digitalWrite(SP8T_PIN_V3, LOW);
}