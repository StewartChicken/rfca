
// Serial cmd input commands:
// {cmd: "a", data: NULL} // 1000 MHz output
// {cmd: "b", data: NULL} // 2000 MHz output
// {cmd: "c", data: NULL} // 3000 MHz output
// {cmd: "d", data: NULL} // 4000 MHz output
// {cmd: "config", data: {sp8t_out_port: 3, start_freq: 1000, stop_freq: 1000, step_size: 1000, delay_ms: 1000}} // Config update
// {cmd: "sweep", data: "sweep1"} // call (unfinished) sweep function

// Notes for 2/24:
// - Added a delay option to the initFreq function which allows you to pass the delay between register writes as an argument to the Serial command line. 
//    This lets us control the delays and monitor individual register writes if we want to
//
//    EX: {cmd: "a", data: 1200} // set to 1000 MHz with 1200 ms delay between register writes
//
// - Next step: 
//    - Don't run the initialization sequence every time, only the first time. Then use the update sequence.
//    - Fix connections
// - https://ez.analog.com/rf/f/q-a/564861/i-m-trying-to-use-the-spi-test-points-on-the-ev-adf5356sd1z-board-with-my-own-spi-controller-to-configure-the-adf5356-my-spi-controller-is-not-configuring-the-device-correctly-and-ideas-why-thanks?
//
// - Adding 1.5k resistors fixed the write issue
// - By modifying registers R0 and R1 (and keeping all others fixed), we can configure any frequency between 3500 and 6000 MHz.
// - To get below 3500 MHz (as low as 1 MHz), we also need to modify R6 to divide the output by more than 1 and adjust R0, R1 accordingly
//   ^^^^^^^ That's the next step

// Libraries
#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPI.h>

// Data
#include "common_defs.h"

// Drivers
#include "./drivers/Inc/sd_card.h"
#include "./drivers/Src/sd_card.c"
#include "./drivers/Inc/no_os_spi.h"
#include "./drivers/Src/no_os_spi.c"

// Function declarations
status_t processCommand(const char* cmd, JsonVariant data);
Config_t update_config_struct(const JsonDocument& config);
void conduct_sweep(); // TODO: Update this declaration
void print_json(const JsonDocument& doc);
void initFreq(uint32_t frequency, const uint32_t delay_ms);
void initFreq(const uint8_t reg[14][4], const uint32_t delay_ms);
int32_t teensy_spi_init(struct no_os_spi_desc **desc, const struct no_os_spi_init_param *param);
int32_t teensy_spi_write_and_read(struct no_os_spi_desc *desc, uint8_t *data, uint16_t bytes_number);
int32_t teensy_spi_remove(struct no_os_spi_desc *desc);

const uint8_t config_1000_mhz[14][4] = {
  {0x00, 0x00, 0x00, 0x0D}, // Reg 13
  {0x00, 0x00, 0x15, 0xFC}, // Reg 12
  {0x00, 0x61, 0x20, 0x0B}, // Reg 11
  {0x00, 0xC0, 0x13, 0x7A}, // Reg 10
  {0x14, 0x0D, 0x3C, 0xC9}, // Reg 9
  {0x15, 0x59, 0x65, 0x68}, // Reg 8
  {0x06, 0x00, 0x00, 0xE7}, // Reg 7
  {0x35, 0x41, 0x84, 0x66}, // Reg 6
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
  {0x35, 0x21, 0x84, 0x66}, // Reg 6
  {0x00, 0x80, 0x00, 0x25}, // Reg 5
  {0x32, 0x01, 0x0B, 0x84}, // Reg 4
  {0x00, 0x00, 0x00, 0x03}, // Reg 3
  {0x00, 0x04, 0x00, 0x32}, // Reg 2
  {0x03, 0x55, 0x55, 0x51}, // Reg 1
  {0x00, 0x30, 0x08, 0x20}  // Reg 0
};

const uint8_t config_3000_mhz[14][4] = {
  {0x00, 0x00, 0x00, 0x0D}, // Reg 13
  {0x00, 0x00, 0x15, 0xFC}, // Reg 12
  {0x00, 0x61, 0x20, 0x0B}, // Reg 11
  {0x00, 0xC0, 0x13, 0x7A}, // Reg 10
  {0x14, 0x0D, 0x3C, 0xC9}, // Reg 9
  {0x15, 0x59, 0x65, 0x68}, // Reg 8
  {0x06, 0x00, 0x00, 0xE7}, // Reg 7
  {0x35, 0x21, 0x84, 0x66}, // Reg 6
  {0x00, 0x80, 0x00, 0x25}, // Reg 5
  {0x32, 0x01, 0x0B, 0x84}, // Reg 4
  {0x00, 0x00, 0x00, 0x03}, // Reg 3
  {0x00, 0x00, 0x00, 0x12}, // Reg 2
  {0x05, 0x00, 0x00, 0x01}, // Reg 1
  {0x00, 0x30, 0x0C, 0x30}  // Reg 0
};

const uint8_t config_4000_mhz[14][4] = {
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
  {0x03, 0x55, 0x55, 0x51}, // Reg 1
  {0x00, 0x30, 0x08, 0x20}  // Reg 0
};

// SPI config structs
struct no_os_spi_platform_ops teensy_spi_ops = {
  .init                             = teensy_spi_init,
  .write_and_read                   = teensy_spi_write_and_read,
  .transfer                         = NULL,
  .transfer_dma                     = NULL,
  .transfer_dma_async               = NULL,
  .remove                           = teensy_spi_remove,
  .transfer_abort                   = NULL
};

struct no_os_spi_init_param spi_params = {
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

struct no_os_spi_desc *spi_desc;


Config_t sweep_config;

// Variables to process incoming data
static const size_t BUFFER_SIZE = 10 * 1024; // 10 KB Max input string (for now)
static char buffer[BUFFER_SIZE];
static size_t idx = 0;


// Main loop error handling. This is the status which is communicated to the Python Host
static status_t global_status = STATUS_OK;

int32_t garbage_data;

#define ENABLE_DEBUG_PRINTS 1

void setup() {

  // From sd_card.h
  // Initialize the SD card hardware
  // Check if the SD card contains:
  //  - config.json
  //  - ./data
  // If dne, create the files
  global_status = SD_init();

  // Get Config from SD card and populate sweep_config struct
  JsonDocument config_doc;
  global_status = SD_get_config(config_doc);
  sweep_config = update_config_struct(config_doc);

  // SPI CS Pin (Latch Enable on ADF)
  // Idles HIGH
  pinMode(ADF_LE, OUTPUT);
  digitalWrite(ADF_LE, HIGH); 

  garbage_data = no_os_spi_init(&spi_desc, &spi_params);
  
  // TODO: Handle error
  if(garbage_data != 0)
    while (1) {}

  // TODO: If an error is present, send immediately to CLI once connected
  Serial.begin(115200);   
  while (!Serial) {}
}

void loop() {
  if(Serial.available()) {
    // Read character off the top of the buffer
    char c = (char)Serial.read();

    if(c == '\n') { // '\n' termination indicates end of JSON command
      buffer[idx] = '\0';

      // Write buffer string to JSON
      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, buffer); 
      if(!err){}
          // Pass for now
          // TODO: Handle error
      
      // Extract the command and the data from the incoming CLI request
      const char* cmd = doc["cmd"] | "";
      JsonVariant data = doc["data"];

      // Processes and executes command, issues response to CLI
      global_status = processCommand(cmd, data);
      idx = 0;
    } 
    else if(idx < BUFFER_SIZE - 1) { 
      buffer[idx] = c;
      idx ++;
    } 
    else {
      // Overflow?
    }

  }
}

// TODO: Incorporate error handling
/**
 * @brief Given a string command and associated data, process that command into action
 * @param cmd - String with specified command for HW
 * @param data - Data associated with command
 * @return: status_t - Error status
 */
status_t processCommand(const char* cmd, JsonVariant data) {

  // Error handling
  status_t cmd_status = STATUS_OK;

  JsonDocument response;

  // Parse and Process
    if(strcmp(cmd, "config") == 0) {
        JsonObject cfg = data.as<JsonObject>();

        // Update config.json on the SD card
        cmd_status = SD_update_config(cfg);
        // TODO: Handle error

        // Assuming updating the config doesn't throw an error, the next step is to 
        // retrieve the updated config from the SD card and use its data to update
        // the sweep_config struct.

        // First we pull the data from the SD card
        JsonDocument config_doc;
        cmd_status = SD_get_config(config_doc);
        // TODO: Handle error

        // Assuming no error is thrown, proceed to update the struct
        sweep_config = update_config_struct(config_doc);

        // Construct the response to the CLI
        // TODO: Response is constructed assuming no error is thrown. Add the case where the error is thrown.
        response["status"] = status_to_str(cmd_status);
        response["cmd"] = cmd;
        response["sd_config"] = config_doc; // This is the config data the SD card contains

        // Send response
        serializeJson(response, Serial);
    }
    else if(strcmp(cmd, "sweep") == 0) {
        const char *sweep_name = data.as<const char*>();
        cmd_status = SD_add_sweep(sweep_name);

        // Uncomment to configure individual frequencies
        /*
        // For integration testing
        uint32_t frequency = sweep_config.frequency;
        initFreq(frequency);
        */

        JsonDocument config_doc;
        cmd_status = SD_get_config(config_doc);
        // TODO: Handle error

        conduct_sweep();
        

        response["status"] = status_to_str(cmd_status);
        response["cmd"] = cmd;
        response["data"]["resp_data"] = config_doc; 
        
        serializeJson(response, Serial);
    }
    else if(strcmp(cmd, "calibrate") == 0) {
        // TODO: Calibrate lol
        // Pass for now
    }
    else if(strcmp(cmd, "list") == 0) {
        const uint8_t MAX_FILES = 20;
        char filenames[MAX_FILES][64];
        uint8_t file_count = 0;

        cmd_status = SD_get_filenames(filenames, MAX_FILES, &file_count);
        
        response["status"] = "OK";
        response["cmd"] = cmd;
        
        JsonObject data = response["data"].to<JsonObject>();
        JsonArray arr = data["resp_data"].to<JsonArray>();

        // Populate array
        for (uint8_t i = 0; i < file_count; ++i) {
            arr.add(filenames[i]);  // duplicates the C-string into the doc
        }

        serializeJson(response, Serial);
    }
    else if(strcmp(cmd, "retrieve") == 0) {
        // TODO: Retrieve the specified file and send to CLI
        // Pass
    }
    else if(strcmp(cmd, "delete") == 0) {
        const char *sweep_name = data.as<const char*>();
        cmd_status = SD_delete_sweep(sweep_name);
        
        response["status"] = status_to_str(cmd_status);
        response["cmd"] = cmd;
        response["data"]["resp_data"] = "Sweep Deleted!";
        
        serializeJson(response, Serial);
    }
    // These are debug commands
    else if(strcmp(cmd, "a") == 0) {
#if ENABLE_DEBUG_PRINTS
      Serial.println("Got an a, initializing Freq to 1000 MHz");
#endif
      const uint32_t delay_ms = data.as<uint32_t>();
      initFreq(1000, delay_ms);
    }
    else if(strcmp(cmd, "b") == 0) {
#if ENABLE_DEBUG_PRINTS
      Serial.println("Got a b, initializing Freq to 2000 MHz");
#endif
      const uint32_t delay_ms = data.as<uint32_t>();
      initFreq(2000, delay_ms);
    }
    else if(strcmp(cmd, "c") == 0) {
#if ENABLE_DEBUG_PRINTS
      Serial.println("Got a c, initializing Freq to 3000 MHz");
#endif
      const uint32_t delay_ms = data.as<uint32_t>();
      initFreq(3000, delay_ms);
    }
    else if(strcmp(cmd, "d") == 0) {
#if ENABLE_DEBUG_PRINTS
      Serial.println("Got a d, initializing Freq to 4000 MHz");
#endif
      const uint32_t delay_ms = data.as<uint32_t>();
      initFreq(4000, delay_ms);
    }
    else if(strcmp(cmd, "freq") == 0) { // {cmd: "freq", data: 3500} // Set output to 3500 MHz
#if ENABLE_DEBUG_PRINTS
      Serial.println("Got an e, initializing custom frequency");
#endif
      const uint32_t freq = data.as<uint32_t>();
      const uint8_t reg_config[14][4] = {0};
      configADFReg(reg_config, freq); // Populate reg_config with values required to obtain desired freq
      initFreq(reg_config, 5); // Delay 5 ms between register writes
    }

    return cmd_status;
}


// TODO: Incorporate error handling
/**
 * @brief Read JSON doc and return a Config_t struct with that data
 * @param config - Data that will be used to populate the struct
 * @return: Config_t struct containing config information
 */
Config_t update_config_struct(const JsonDocument& config) {
    Config_t config_struct;

    // Populate struct values
    config_struct.sp8t_out_port = config["sp8t_out_port"];
    config_struct.start_freq = config["start_freq"];
    config_struct.stop_freq = config["stop_freq"];
    config_struct.step_size = config["step_size"];
    config_struct.delay_ms = config["delay_ms"];

    return config_struct;
}

// TODO: Incorporate error handling
/**
 * @brief Conduct sweep w/ in hardware according to config file
 * @param TODO
 * @return: TODO
 */
void conduct_sweep() {
  uint32_t curr_freq = sweep_config.start_freq;
  uint32_t stop_freq = sweep_config.stop_freq;
  uint32_t step_size = sweep_config.step_size;
  uint32_t delay_ms = sweep_config.delay_ms;

#if ENABLE_DEBUG_PRINTS
  Serial.println("Sweep Config Values:");
  Serial.print("start_freq: ");
  Serial.println(curr_freq);
  Serial.print("stop_freq: ");
  Serial.println(stop_freq);
  Serial.print("step_size: ");
  Serial.println(step_size);
  Serial.print("delay_ms: ");
  Serial.println(delay_ms);
#endif
  
  while(curr_freq <= stop_freq) {
    initFreq(curr_freq, 5);
    curr_freq += step_size;
    delay(delay_ms);
  }

  // For now, we only output the start_freq so that we can select individual frequencies rather than a full sweep
  //initFreq(curr_freq, 5);
  
}

/**
 * @brief Produce register values for a given frequency
 * @param reg - Array of 8-byte values containing the caller's register information
 * @param frequency - Desired output frequency: 850MHz - 6800MHz 
 * @return: void
 * TODO: Error handling
 */
void configADFReg(uint8_t reg[14][4], const uint32_t frequency) {

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
    {0x32, 0x01, 0x0B, 0x84}, // Reg 4s
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

/**
 * @brief Print a JSON doc to serial
 * @param doc - Json document to be serialized
 * @return: void
 */
void print_json(const JsonDocument& doc) {
    serializeJsonPretty(doc, Serial);
    //Serial.println(); // Remove when using CLI
}

// Overloaded
// TODO: Incorporate error handling
/**
 * @brief Choose values to hardcode to ADF5356 registers
 * @param reg - Bytes to write - 56 total
 * @return TODO: Error code
 */
void initFreq(uint32_t frequency, const uint32_t delay_ms) {

#if ENABLE_DEBUG_PRINTS
  Serial.print("\nSetting Frequency (MHz): ");
  Serial.println(frequency);
  Serial.print("Delay between reg. writes (ms): ");
  Serial.println(delay_ms);
#endif

  switch (frequency) {
    case 1000:
      initFreq(config_1000_mhz, delay_ms);
      break;
    case 2000:
      initFreq(config_2000_mhz, delay_ms);
      break;
    case 3000:
      initFreq(config_3000_mhz, delay_ms);
      break;
    case 4000:
      initFreq(config_4000_mhz, delay_ms);
      break;
    default:
      //Serial.println("Received un-mapped frequency");
      break;
  };
}

// Overloaded
// TODO: Incorporate error handling
/**
 * @brief Write contents of 'reg' array to ADF5356 via SPI
 * @param reg - Bytes to write - 56 total
 * @return TODO: Error code
 */
void initFreq(const uint8_t reg[14][4], const uint32_t delay_ms) {

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

    uint32_t ret = no_os_spi_write_and_read(spi_desc, buf, 4);
    delay(delay_ms);
  }

#if ENABLE_DEBUG_PRINTS
  Serial.println("Done");
#endif
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
