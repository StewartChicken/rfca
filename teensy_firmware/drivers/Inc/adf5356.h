
#ifndef _ADF5356_H
#define _ADF5356_H

// Libraries
#include <SPI.h>

// Data
#include "../../common_defs.h"

#define REG_WRITE_DELAY 5 // ms delay between writing to registers

// Public API functions
void adf5356_spi_init(void);
void adf5356_disable_rf(void);
void adf5356_write_freq(uint32_t frequency);

// Internal functions
static void adf5356_config_regs(uint8_t reg[14][4], const uint32_t frequency);
static void adf5356_write_regs(const uint8_t reg[14][4], const uint32_t delay_ms);
static void adf5356_spi_write(uint8_t *data, uint16_t num_bytes);
static inline uint32_t pack_word(const uint8_t bytes[4]);
static inline void unpack_word(uint8_t bytes[4], const uint32_t word);

#endif // _ADF5356_H