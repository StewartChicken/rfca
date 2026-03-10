
#include <Arduino.h>
#include <SPI.h>
#include "common_defs.h"
#include "no_os_spi.h"
#include "no_os_spi.c"

// Function declarations
int32_t teensy_spi_init(struct no_os_spi_desc **desc, const struct no_os_spi_init_param *param);
int32_t teensy_spi_write_and_read(struct no_os_spi_desc *desc, uint8_t *data, uint16_t bytes_number);
int32_t teensy_spi_remove(struct no_os_spi_desc *desc);

// SPI config structs
static struct no_os_spi_platform_ops teensy_spi_ops = {
  .init                             = teensy_spi_init,
  .write_and_read                   = teensy_spi_write_and_read,
  .transfer                         = NULL,
  .transfer_dma                     = NULL,
  .transfer_dma_async               = NULL,
  .remove                           = teensy_spi_remove,
  .transfer_abort                   = NULL
};

static struct no_os_spi_init_param spi_params = {
  .device_id                        = 0,
  .max_speed_hz                     = 1000000, // 1 MHz
  .chip_select                      = ADF_LE,  // CS is pin 10
  .mode                             = NO_OS_SPI_MODE_0,                   // DNU
  .bit_order                        = NO_OS_SPI_BIT_ORDER_MSB_FIRST, // DNU
  .lanes                            = NO_OS_SPI_SINGLE_LANE,
  .platform_ops                     = &teensy_spi_ops,
  .platform_delays                  = {0},
  .extra                            = NULL,
  .parent                           = NULL
};

static struct no_os_spi_desc *spi_desc;

int32_t garbage_data;

const uint8_t config_100_mhz[14][4] = {
    {0x00, 0x00, 0x00, 0x0D}, // Reg 13
    {0x00, 0x00, 0x15, 0xFC}, // Reg 12
    {0x00, 0x61, 0x20, 0x0B}, // Reg 11
    {0x00, 0xC0, 0x13, 0x7A}, // Reg 10
    {0x14, 0x0D, 0x3C, 0xC9}, // Reg 9
    {0x15, 0x59, 0x65, 0x68}, // Reg 8
    {0x06, 0x00, 0x00, 0xE7}, // Reg 7
    {0x35, 0x01, 0x84, 0x66}, // Reg 6
    {0x00, 0x80, 0x00, 0x25}, // Reg 5
    {0x32, 0x01, 0x0B, 0x84}, // Reg 4
    {0x00, 0x00, 0x00, 0x03}, // Reg 3
    {0x00, 0x04, 0x00, 0x32}, // Reg 2
    {0x0A, 0xD5, 0x55, 0x51}, // Reg 1
    {0x00, 0x30, 0x06, 0xE0}  // Reg 0
};


const uint8_t config_1000_mhz[14][4] = {
    {0x00, 0x00, 0x00, 0x0D}, // Reg 13
    {0x00, 0x00, 0x15, 0xFC}, // Reg 12
    {0x00, 0x61, 0x20, 0x0B}, // Reg 11
    {0x00, 0xC0, 0x13, 0x7A}, // Reg 10
    {0x14, 0x0D, 0x3C, 0xC9}, // Reg 9
    {0x15, 0x59, 0x65, 0x68}, // Reg 8
    {0x06, 0x00, 0x00, 0xE7}, // Reg 7
    {0x35, 0x41, 0x84, 0x76}, // Reg 6
    {0x00, 0x80, 0x00, 0x25}, // Reg 5
    {0x32, 0x01, 0x0B, 0x84}, // Reg 4
    {0x00, 0x00, 0x00, 0x03}, // Reg 3
    {0x00, 0x04, 0x00, 0x32}, // Reg 2
    {0x03, 0x55, 0x55, 0x51}, // Reg 1
    {0x00, 0x30, 0x08, 0x20}  // Reg 0
};

const uint8_t config_2000_mhz[14][4] = {
    {0x00, 0x00, 0x00, 0x0D}, // Reg 13
    {0x00, 0x00, 0x15, 0xFC}, // Reg 12
    {0x00, 0x61, 0x20, 0x0B}, // Reg 11
    {0x00, 0xC0, 0x13, 0x7A}, // Reg 10
    {0x14, 0x0D, 0x3C, 0xC9}, // Reg 9
    {0x15, 0x59, 0x65, 0x68}, // Reg 8
    {0x06, 0x00, 0x00, 0xE7}, // Reg 7
    {0x35, 0x21, 0x84, 0x76}, // Reg 6
    {0x00, 0x80, 0x00, 0x25}, // Reg 5
    {0x32, 0x01, 0x0B, 0x84}, // Reg 4
    {0x00, 0x00, 0x00, 0x03}, // Reg 3
    {0x00, 0x04, 0x00, 0x32}, // Reg 2
    {0x03, 0x55, 0x55, 0x51}, // Reg 1
    {0x00, 0x30, 0x08, 0x20}  // Reg 0
};

const uint8_t config_4000_mhz[14][4] = {
    {0x00, 0x00, 0x00, 0x0D}, // Reg 13
    {0x00, 0x00, 0x15, 0xFC}, // Reg 12
    {0x00, 0x61, 0x20, 0x0B}, // Reg 11
    {0x00, 0xC0, 0x13, 0x7A}, // Reg 10
    {0x14, 0x0D, 0x3C, 0xC9}, // Reg 9
    {0x15, 0x59, 0x65, 0x68}, // Reg 8
    {0x06, 0x00, 0x00, 0xE7}, // Reg 7
    {0x35, 0x01, 0x84, 0x76}, // Reg 6
    {0x00, 0x80, 0x00, 0x25}, // Reg 5
    {0x32, 0x01, 0x0B, 0x84}, // Reg 4
    {0x00, 0x00, 0x00, 0x03}, // Reg 3
    {0x00, 0x04, 0x00, 0x32}, // Reg 2
    {0x03, 0x55, 0x55, 0x51}, // Reg 1
    {0x00, 0x30, 0x08, 0x20}  // Reg 0
  };


void setup() {
  Serial.begin(115200);   
  while (!Serial) {}

  pinMode(ADF_LE, OUTPUT);
  digitalWrite(ADF_LE, HIGH); // LE Idles HIGH

  Serial.println("Initializing SPI for integration testing...");
  garbage_data = no_os_spi_init(&spi_desc, &spi_params);
  if(garbage_data != 0)
      while (1) {}
    
  Serial.println("SPI Initialized");
}

void loop() {
  if (Serial.available() > 0) {      
    char c = Serial.read();          
    
    switch (c) {
      case '\n':
        break;
      case 'a':
        Serial.print("Input: ");
        Serial.println(c);
        Serial.println("Setting to 1000 mhz");
        setFreq(config_1000_mhz);
        break;
      case 'b':
        Serial.print("Input: ");
        Serial.println(c);
        Serial.println("Setting to 2000 mhz");
        setFreq(config_2000_mhz);
        break;
      case 'c':
        Serial.print("Input: ");
        Serial.println(c);
        Serial.println("Setting to 4000 mhz");
        setFreq(config_4000_mhz);
        break;
      case 'd':
        Serial.print("Input: ");
        Serial.println(c);
        high_freq();
        break;
      case 'e':
        Serial.print("Input: ");
        Serial.println(c);
        low_freq();
      default: 
        Serial.println("Undefined Input");
        break;
    }
    
  }
}

void setFreq(const uint8_t reg[14][4]) {

  for(int i = 0; i < 14; i ++) {

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

    // Copy to prevent overwrite of *reg
    uint8_t buf[4];
    memcpy(buf, reg[i], 4);

    int32_t ret = no_os_spi_write_and_read(spi_desc, buf, 4);
    delay(2);
  }
}

void high_freq() {
  uint8_t test_data[] = {0x35, 0x41, 0x84, 0x66}; // R6 - 1700 MHz
  uint16_t num_bytes = 4;
  int32_t ret = no_os_spi_write_and_read(spi_desc, test_data, num_bytes);
}

void low_freq() {
  uint8_t test_data[] = {0x35, 0x21, 0x84, 0x66}; // R6 - 850 MHz
  uint16_t num_bytes = 4;
  int32_t ret = no_os_spi_write_and_read(spi_desc, test_data, num_bytes);
}


/**
 * @brief Initialize the SPI communication peripheral.
 * @param desc - The SPI descriptor.
 * @param param - The structure that contains the SPI parameters.
 * @return 0 in case of success, -1 otherwise.
 */
int32_t teensy_spi_init(struct no_os_spi_desc **desc, const struct no_os_spi_init_param *param) {

  // Steps:
  // 1. Begin SPI
  // 2. Allocate *desc
  // 3. Transfer params to *desc
  // 4. Config Chip Select pin
  // 5. return

  if (!desc || !param) return -1;

  // Allocate desc
  struct no_os_spi_desc *local_desc;
  local_desc = (struct no_os_spi_desc *)no_os_calloc(1, sizeof(*local_desc));
  if (!local_desc)
    return -1;

  local_desc->device_id       = param->device_id;
  local_desc->max_speed_hz    = param->max_speed_hz;
  local_desc->chip_select     = param->chip_select;
  local_desc->mode            = param->mode;
  local_desc->bit_order       = param->bit_order;
  local_desc->lanes           = param->lanes;
  local_desc->extra           = param->extra;

  //pinMode(local_desc->chip_select, OUTPUT);
  digitalWrite(local_desc->chip_select, HIGH); // LE Idles in HIGH state

  *desc = local_desc;

  // Begin Teensy4.1 SPI0
  SPI.begin();

  // 0 = OK status
  return 0;
}

/**
 * @brief Write and read data to/from SPI.
 * @param desc - The SPI descriptor.
 * @param data - The buffer with the transmitted/received data.
 * @param bytes_number - Number of bytes to write/read.
 * @return 0 in case of success, -1 otherwise.
 */
int32_t teensy_spi_write_and_read(struct no_os_spi_desc *desc, uint8_t *data, uint16_t bytes_number) {

  // Steps:
  // 1. Begin SPI transaction
  // 2. Assert CS LOW
  // 3. Transfer Data
  // 4. Assert CS HIGH
  // 5. End SPI Transaction

  if (!desc || !data || bytes_number == 0) return -1;

  // TODO: Globally define?
  SPISettings spiSettings(
    1000000,      // 1 MHz SPI clockss
    MSBFIRST,     // Bit order
    SPI_MODE0     // CPOL=0, CPHA=0
  );

  /*
  // Using desc:
  SPISettings spiSettings(
    desc->max_speed_hz,      // 1 MHz SPI clock
    desc->bit_order,         // Bit order
    desc->mode               // CPOL=0, CPHA=0
  );
  */

  digitalWrite(desc->chip_select, LOW); // Pull LE low
  delayMicroseconds(1); 

  SPI.beginTransaction(spiSettings);
  SPI.transfer(data, bytes_number);   // in-place buffer
  SPI.endTransaction();

  delayMicroseconds(1);
  digitalWrite(desc->chip_select, HIGH); // Latch data from ADF5356 Shift Register to Data Registers

  // 0 = OK status
  return 0;
}

/**
 * @brief Free the resources allocated by no_os_spi_init().
 * @param desc - The SPI descriptor.
 * @return 0 in case of success, -1 otherwise.
 */
int32_t teensy_spi_remove(struct no_os_spi_desc *desc) {

  if (!desc) return -1;

  SPI.end();
  digitalWrite(desc->chip_select, HIGH);
  no_os_free(desc);

  // 0 = OK status
  return 0;
}
