
#include "../Inc/adf5356.h"

/**
 * @brief Initialize SPI for communication with the ADF5356
 * @return void
 * TODO: Error handling
 */
void ADF_spi_init() {

  // SPI CS Pin (Latch Enable on ADF) - idles HIGH
  pinMode(ADF_LE, OUTPUT);
  digitalWrite(ADF_LE, HIGH); 

  // Begin Teensy4.1 SPI0
  // GPIO 37 - CS
  // GPIO 11 - MOSI
  // GPIO 12 - MISO (not used)
  // GPIO 13 - SCK
  SPI.begin();
}

/**
 * @brief Write the given frequency to the ADF5356
 * @param frequency - Desired output frequency: [850, 6800] (inclusive, in MHz)
 * @return: void
 * TODO: Error handling
 */
void ADF_write_freq(uint32_t frequency) {
  
  if(frequency < 850) {
    // TODO: Throw error
  }
  else if(frequency > 6800) {
    // TODO: Throw error
  }

  uint8_t reg_config[14][4] = {0}; // Overwritten below
  ADF_config_regs(reg_config, frequency);

  ADF_write_regs(reg_config, REG_WRITE_DELAY);
}

/**
 * @brief Write and read data to/from SPI.
 * @param data - The buffer with the transmitted/received data.
 * @param bytes_number - Number of bytes to write/read.
 * @return void
 * TODO: Error handling
 */
static void ADF_spi_write(uint8_t *data, uint16_t num_bytes) {

  // Steps:
  // 1. Begin SPI transaction
  // 2. Assert CS LOW
  // 3. Transfer Data
  // 4. Assert CS HIGH
  // 5. End SPI Transaction

  // TODO: Globally define?
  SPISettings spiSettings(
    1000000,      // 1 MHz SPI clockss
    MSBFIRST,     // Bit order
    SPI_MODE0     // CPOL=0, CPHA=0
  );

  digitalWrite(ADF_LE, LOW); // Pull LE low
  delayMicroseconds(1); 

  SPI.beginTransaction(spiSettings);
  SPI.transfer(data, num_bytes);   // in-place buffer
  SPI.endTransaction();

  delayMicroseconds(1);
  digitalWrite(ADF_LE, HIGH); // Latch data from ADF5356 Shift Register to Data Registers
}

/**
 * @brief Produce register values for a given frequency
 * @param reg - Array of 8-byte values containing the caller's register information
 * @param frequency - Desired output frequency: 850MHz - 6800MHz 
 * @return: void
 * TODO: Error handling
 */
static void ADF_config_regs(uint8_t reg[14][4], const uint32_t frequency) {

  // TODO: Error checking for frequency range

  /* Formula:
  *   N = INT + (FRAC1 + FRAC2/MOD2) / (MOD1)
  *   INT is stored in R0
  *   FRAC1 is stored in R1
  *   FRAC2/MOD2 are stored in R2/R13
  *   MOD1 is prefixed to 2^24
  */

  // If we divide output by 2 by changing R6:
  // Range = (3400 - 6800) / 2 = 
  //  (1700, 3400)
  //
  // If we divide by 4, we get
  // Range = (3400 - 6800) / 4=
  //  (850 - 1700)

  uint8_t divider;
  if(frequency < 1700) {
    divider = 4;
  }
  else if(frequency < 3400) {
    divider = 2;
  }
  else {
    divider = 1;
  }

  uint32_t divided_frequency = frequency * divider;

  // Hard coded from GUI 
  const float PFD = 30.72;
  float N = (float)(divided_frequency) / PFD;

  // To be computed: R0, R1, R2
  // Skip R2 for now, set FRAC2 to 0 and MOD2 to 1

  // The only registers we'll be changing for the time being are R0, R1, and R2
  uint8_t reg_base[14][4] = {
    {0x00, 0x00, 0x00, 0x0D}, // Reg 13
    {0x00, 0x00, 0x15, 0xFC}, // Reg 12
    {0x00, 0x61, 0x20, 0x0B}, // Reg 11
    {0x00, 0xC0, 0x13, 0x7A}, // Reg 10
    {0x14, 0x0D, 0x3C, 0xC9}, // Reg 9
    {0x15, 0x59, 0x65, 0x68}, // Reg 8
    {0x06, 0x00, 0x00, 0xE7}, // Reg 7
    {0x35, 0x01, 0x84, 0x66}, // Reg 6 // Modified to get frequencies lower than 3500
    {0x00, 0x80, 0x00, 0x25}, // Reg 5
    {0x32, 0x01, 0x0B, 0x84}, // Reg 4
    {0x00, 0x00, 0x00, 0x03}, // Reg 3

    {0x00, 0x00, 0x00, 0x12}, // Reg 2 // Fixed for now: FRAC2 = 0, MOD2 = 1

    {0x03, 0x55, 0x55, 0x51}, // Reg 1 // Configured below
    {0x00, 0x30, 0x08, 0x20}  // Reg 0 // Configured below
  };

  // Update R6
  uint32_t r6 = pack_word(reg_base[7]);
  r6 &= ~((uint32_t)0xE << 20); // Clear RF DIVIDER SELECT field, bits 21-23

  // TODO: There is a better way to do this (ternary?)
  if(divider == 2) {
     r6 |= ((uint32_t)0x1 << 21);
  } else if(divider == 4) {
    r6 |= ((uint32_t)0x2 << 21);
  }
  unpack_word(reg_base[7], r6);
 
  // Update R0
  uint16_t INT = (int)(N);
  uint32_t r0 = pack_word(reg_base[13]);
  r0 &= ~((uint32_t)0xFFFF << 4); // Clear INT field, bits 4-19
  r0 |= (((uint32_t)INT & 0xFFFFu) << 4);  
  unpack_word(reg_base[13], r0);

  // Update R1
  N -= (float)(INT);
  uint32_t FRAC1 = (uint32_t)(N * 16777216.0f); // 2^24
  uint32_t r1 = pack_word(reg_base[12]);
  r1 &= ~((uint32_t)0xFFFFFF << 4); // Clear FRAC1 field, bits 4-27
  r1 |= (((uint32_t)FRAC1 & 0xFFFFFFu) << 4);
  unpack_word(reg_base[12], r1);

  // Copy reg_base into reg
  memcpy(reg, reg_base, sizeof(reg_base));
}

/**
 * @brief Write contents of 'reg' array to ADF5356 via SPI
 * @param reg - Bytes to write - 56 total
 * @param delay_ms - Delay between reg writes (ms)
 * @return void
 * TODO: Error handling
 */
static void ADF_write_regs(const uint8_t reg[14][4], const uint32_t delay_ms) {

  for(int i = 0; i < 14; i ++) {

#if ENABLE_DEBUG_PRINTS
    // Print Header
    Serial.print("Writing to register R");
    Serial.print(13 - i);
    Serial.print(": ");
    if ((13 - i) < 10) Serial.print(" ");
    Serial.print("0x");

    // Print Data
    for (int j = 0; j < 4; j++) {
      if (reg[i][j] < 0x10) Serial.print("0");
      Serial.print(reg[i][j], HEX);
    }
    Serial.println();
#endif

    // Copy to prevent overwrite of *reg
    uint8_t buf[4];
    memcpy(buf, reg[i], 4);

    // Write 4 bytes (a single 32-bit register)
    //uint32_t ret = no_os_spi_write_and_read(spi_desc, buf, 4);
    ADF_spi_write(buf, 4);
    delay(delay_ms);
  }

#if ENABLE_DEBUG_PRINTS
  Serial.println("Done");
#endif
}

/**
 * @brief Take a four-byte array and pack it into a single 32-bit word
 * @param bytes - Four byte array to be packed into 32bit word
 * @return: uint32_t (packed word)
 */
static inline uint32_t pack_word(const uint8_t bytes[4]) {
    return ((uint32_t)bytes[0] << 24) |
           ((uint32_t)bytes[1] << 16) |
           ((uint32_t)bytes[2] <<  8) |
           ((uint32_t)bytes[3] <<  0);
}

/**
 * @brief Take a 32 bit word and unpack it into a four byte array
 * @param bytes - Four byte array to be populated by word
 * @param word - 32-bit word to be unpacked
 * @return: void
 */
static inline void unpack_word(uint8_t bytes[4], const uint32_t word) {
    bytes[0] = (uint8_t)(word >> 24);
    bytes[1] = (uint8_t)(word >> 16);
    bytes[2] = (uint8_t)(word >>  8);
    bytes[3] = (uint8_t)(word >>  0);
}