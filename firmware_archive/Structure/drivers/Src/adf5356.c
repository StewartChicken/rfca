
#include "../Inc/adf5356.h"

// Configure all pins to initial state
// Should implement a pseudo-SPI: instead of CS, we use LE which is active HIGH
void ADF_init() {
    // TODO: Configure for pseudo-SPI
    pinMode(ADF_LE, OUTPUT);
    pinMode(ADF_DATA, OUTPUT); 
    pinMode(ADF_MISO, OUTPUT); // TODO: Not used, don't config?
    pinMode(ADF_CLK, OUTPUT);

    // Pulled low until data must be written
    digitalWrite(LE, LOW);
}

void ADF_set_out_freq(uint32_t frequency) {
    // TODO: Compute register values and write via SPI line
}
