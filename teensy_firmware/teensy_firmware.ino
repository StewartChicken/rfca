
// TODO:
// Finish SD control
//  - Finish functions
//  - Change update config scope
//  - Controlled via CLI
//  - Refactor filename buffer design
// ADF5356 Driver
//  - SPI interface (init, write 32-bit word)
//  - Write reg function
//  - ...
// Error handling
//  - CLI communication
//  - CLI handling
//  - If an error is thrown in setup, how to communicate? Can't be printed
// Sweep conduction using ADF driver
// Config struct
// Handle blank responses from teensy (usually occur after first boot or reset)
// Design response data

// Just done: 1/17/2026
// - Using Arduino IDE serial to communicate with firmware
//   - Sending raw JSON packets: e.g. {cmd: "delete", data: "Sweep1"}
// - Can update Config.json and Config_t
// - Can create sweep file
// - Can list sweep files
// - Can delete sweep file
// - Created the temp_struct in the sweep function (Very last thing done)
//
// Next steps:
// - Design a frequency sweep from laptop -> firmware (What happens when the "sweep" command is issued?)
// - To start, ignore calibration necessity and assume the measured data is ideal
// - Figure out all possible combinations of sweeps (ports, output measurement) and design config system that makes each combination reachable
// - Design ADF5356 driver
//   - Set Out Frequency
// - Design a frequency sweep from Firmware perspective
//   - Function: conduct_sweep(uint8_t out_port, uint8_t[] in_ports, uint32_t start_fq, uint32_t end_fq, uint32_t intvl)
//      - open sp8t port
//      - LOOP: start_fq, end_fq, intvl
//          - set out adf5356 frequency
//          - delay
//          - analog_read each in_port

// Future notes:
// When it comes time to update the data in the Config_t struct, design needs to change in three places:
// - The config.json file which is fed as input to the CLI must reflect the data structure
// - The update_config_struct() function must be altered to extract the correct values from incoming data
// - The Config_t struct definition must be updated in common_defs.h

// Documentation
// https://arduinojson.org/v7/


// Libraries
#include <Arduino.h>
#include <ArduinoJson.h>

// Data
#include "common_defs.h"

// Drivers
#include "./drivers/Inc/sd_card.h"
#include "./drivers/Src/sd_card.c"
#include "./drivers/Inc/adf5355.h"
#include "./drivers/Src/adf5355.c"


Config_t sweep_config;

// Variables to process incoming data
static const size_t BUFFER_SIZE = 10 * 1024; // 10 KB Max input string (for now)
static char buffer[BUFFER_SIZE];
static size_t idx = 0;


// Main loop error handling. This is the status which is communicated to the Python Host
static status_t global_status = STATUS_OK;


// Function declarations
int32_t teensy_spi_init(struct no_os_spi_desc **desc, const struct no_os_spi_init_param *param);
int32_t teensy_spi_write_and_read(struct no_os_spi_desc *desc, uint8_t *data, uint16_t bytes_number);
int32_t teensy_spi_remove(struct no_os_spi_desc *desc);

// SPI config structs
static struct no_os_spi_platform_ops teensy_spi_ops = {
  .init = teensy_spi_init,
  .write_and_read = teensy_spi_write_and_read,
  .transfer = NULL,
  .transfer_dma = NULL,
  .transfer_dma_async = NULL,
  .remove = teensy_spi_remove,
  .transfer_abort = NULL
};

static struct no_os_spi_init_param spi_params = {
  .device_id = 0,
  .max_speed_hz = 1000000, // 1 MHz
  .chip_select = 10,  // CS is pin 10
  .mode = NO_OS_SPI_MODE_0,                   // DNU
  .bit_order = NO_OS_SPI_BIT_ORDER_MSB_FIRST, // DNU
  .lanes = NO_OS_SPI_SINGLE_LANE,
  .platform_ops = &teensy_spi_ops,
  .platform_delays = {0},
  .extra = NULL,
  .parent = NULL
};

static struct no_os_spi_desc *spi_desc;


static struct adf5355_init_param adf_init_params = {
    .spi_init       = &spi_params,
    .dev_id         = ADF5356,
    .freq_req       = 2000000000ULL, // Desired output frequency in Hz
    .freq_req_chan  = 0,    // 0 for RFOUTA, 1 for RFOUTB
    .clkin_freq     = 122880000, // Reference frequency in Hz
    .cp_ua          = 900, // ? Why not 90?
    .cp_neg_bleed_en   = true,
    .cp_gated_bleed_en = false,
    .cp_bleed_current_polarity_en = false, // ? What dis do?
    .mute_till_lock_en = false,
    .outa_en           = true,
    .outb_en           = false,
    .outa_power        = 3,   // ? Why not 5? Expects 0-3?
    .outb_power        = 0,
    .phase_detector_polarity_neg = false,
    .ref_diff_en = true,   // ? Why not false?    
    .mux_out_3v3_en = true,
    .ref_doubler_en = false,
    .ref_div2_en    = true,
    .mux_out_sel = ADF5355_MUXOUT_DIGITAL_LOCK_DETECT,
    .outb_sel_fund = false
 
};

static struct adf5355_dev *adf_dev_struct;            
  

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

    //config_adf_init_struct(&adf_init_struct); // Populates struct with default values
    
    
    
    // Open Serial communication (USB-CDC for Teensy 4.1) after peripheral initialization
    // Wait for host to run Python script
    Serial.begin(115200); // Baud is irrelevant for USB-CDC
    while (!Serial) {} // Stuck in this loop until the Python script is run

    // TODO: Remove (dev functions)
    //print_json(config_doc);
    //Serial.println(sweep_config.sp8t_out_port);

    Serial.println("Serial communication established. ");
    
    Serial.println("Initializing ADF5356...");
    int32_t ret = adf5355_init(&adf_dev_struct, &adf_init_params);
    
}

// Main Loop structure:
// 1. Check for incoming Serial data
// 2. Read character by character and populate buffer string
// 3. If we receive '\n', the command is complete and ready for processing
// 4. Load buffer -> JSON
// 5. Process JSON command
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


        /*

        
        // Placeholder for Config_t sweep_config struct
        struct temp_data {
            uint8_t out_ports[] = {2, 3, 6, 7}; // SP8T ports, values range from 1-8
            uint8_t num_out_ports = 4;          // size of out_ports array
            uint8_t in_ports[]  = {1, 4, 5};    // Log amp ports, values range from 1-10
            uint8_t num_in_ports = 3;           // size of in_ports array
            uint32_t start      = 400;          // MHz     
            uint32_t stop       = 6000;         // MHz
            uint32_t step       = 100;          // MHz
        };
        

        // Here's what the sweep function should look like. All of its arguments should be provided by
        //  the Config_t sweep_config struct.
        // 
        // Params:
        // - sp8_t_ports specifies which of the 8 outputs from the sp8t board will be swept over
        // - in_ports specifies which logAmp inputs will be read for each sp8t port
        // - start is start frequency in MHz
        // - stop is stop frequency in MHz
        // - intvl is spacing between measurements in MHz
        // func conduct_sweep(sp8t_ports[arr], in_ports[arr], start, stop, intvl):
        //      
        //      for port in sp8t_ports:
        //          curr_freq = start;
        //
        //          while(curr_freq < stop):
        //              adf_out(curr_freq)
        //              delay 
        //
        //              for in_port in in_ports:
        //                  power = analog_read(in_port) 
        //                  store_data(power, in_port)
        //
        //              curr_freq += intvl
        //          
        uint32_t curr_freq;

        // Loops 8 times in the worst case
        for (int i = 0; i < temp_data.num_out_ports; i ++) { 
            curr_freq = temp_data.start;

            // Loops ~ 150 times in the (very) worst case
            while(curr_freq < temp_data.stop) {
                // adf_out(curr_freq) // TODO: Write this function in the ADF driver
                // delay() // TODO: Establish delay (if needed)
                
                // Loops 10 times in the worst case
                for (int j = 0; j < temp_data.num_in_ports; j ++) {
                    //int power = analogRead(log_amp(j)) // TODO: uint32_t for power? Dynamic selection of log amps based on index
                    // power - cal_data // TODO: design cal system and cal struct
                    // SD_add_sweep_data(power (V), SP8T_port, LogAmp_port, time_stamp)
                    // Once done writing to SD, continue;
                    continue;
                }
            }
        }

        // Worst case ~15,000 iterations of the above loop
        // ~ (32 * 13) + 100 cycles overhead ~= 500 cycles to output a frequency
        // stabilize delay ~ 1ms?
        // write SD ~ 1ms?
        // Teensy 4.1 executes ~ 1.4 IPC at 600 MHz 
        //  - 1 Cycle ~= 1.66e-9 s (~1ns)
        // Each loop iteration takes 3ms at the high end?
        // 3ms * 15,000 ~= 1 min in the worst case

        // TODO: conduct sweep
        // 1) Enable correct sp8t port
        // 2) Start sweep thread (ends once each frequency value has been recorded)
        //     a) Write ADF5356 (generate signal)
        //     b) Analog Read log-amp (measure signal)
        //     c) Record voltage (write to SD card sweep.csv)
        //sp8t_enablePort(sweep_config.sp8t_out_port); // 1)



        */


        // for each freq in frequency_range:
        //   genFreq(freq);
        //   power = analogRead(logAmp) + sum math;
        //   writeSD(filename, power);

        response["status"] = status_to_str(cmd_status);
        response["cmd"] = cmd;
        response["data"]["resp_data"] = NULL; // TODO: Return sweep data?
        
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

    return cmd_status;
}

Config_t update_config_struct(const JsonDocument& config) {
    Config_t config_struct;

    // Populate struct values
    config_struct.sp8t_out_port = config["sp8t_out_port"];

    return config_struct;
}

// TODO: Define values in common_defs.h ? They're all hardcoded for now
void config_adf_init_struct(adf5355_init_param *init_struct) {
    init_struct->spi_init            = NULL;       // For SPI communication
    init_struct->dev_id               = ADF5356; 

    // Signal config
    init_struct->freq_req                        = 2000000000ULL; // Desired output frequency in Hz.
    init_struct->freq_req_chan                   = 0;          // 0 for RFOUTA, 1 for RFOUTB
    init_struct->clkin_freq                      = 122880000;  // Reference frequency in Hz
    init_struct->ref_doubler_en                  = false;
    init_struct->ref_div2_en                     = true; 

    // Reg 6
    init_struct->cp_ua                           = 900; // ? Why not 90?
    init_struct->cp_neg_bleed_en                = true;
    init_struct->cp_gated_bleed_en              = false;
    init_struct->cp_bleed_current_polarity_en    = false; // ? What dis do?

    init_struct->mute_till_lock_en              = false;
    init_struct->outa_en                        = true;
    init_struct->outb_en                        = false;
    init_struct->outa_power                     = 3; // ? Why not 5? Expects 0-3?
    init_struct->outb_power                     = 0;

    init_struct->phase_detector_polarity_neg    = false;
    init_struct->ref_diff_en                    = true;  // ? Why not false?    
    init_struct->mux_out_3v3_en                  = true;
                    
    init_struct->mux_out_sel                     = ADF5355_MUXOUT_DIGITAL_LOCK_DETECT;
    init_struct->outb_sel_fund                   = false;
}

void conduct_sweep() {

}

void print_json(const JsonDocument& doc) {
    serializeJsonPretty(doc, Serial);
    Serial.println();
}



/**
 * @brief Initialize the SPI communication peripheral.
 * @param desc - The SPI descriptor.
 * @param param - The structure that contains the SPI parameters.
 * @return 0 in case of success, -1 otherwise.
 */
int32_t teensy_spi_init(struct no_os_spi_desc **desc, const struct no_os_spi_init_param *param) {

  Serial.println("Entered the teensy_spi_init() function!");
  // Steps:
  // 1. Begin SPI
  // 2. Allocate *desc
  // 3. Transfer params to *desc
  // 4. Config Chip Select pin
  // 5. return

  if (!desc || !param) return -1;

  // Begin Teensy4.1 SPI0
  SPI.begin();

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

  pinMode(local_desc->chip_select, OUTPUT);
  digitalWrite(local_desc->chip_select, LOW); // Idles in the low state

  *desc = local_desc;

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
    1000000,      // 1 MHz SPI clock
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

  SPI.beginTransaction(spiSettings);

  /*
  for(int i = 0; i < bytes_number; i ++) {
    SPI.transfer(data[i]);
  }
    */
  SPI.transfer(data, bytes_number);   // in-place buffer

  SPI.endTransaction();

  digitalWrite(desc->chip_select, HIGH); // Latch data in ADF5356 Shift Register
  delayMicroseconds(2);
  digitalWrite(desc->chip_select, LOW);

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

  digitalWrite(desc->chip_select, HIGH);
  no_os_free(desc);

  // 0 = OK status
  return 0;
}