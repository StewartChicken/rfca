
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

Config_t sweep_config;

// Variables to process incoming data
static const size_t BUFFER_SIZE = 10 * 1024; // 10 KB Max input string (for now)
static char buffer[BUFFER_SIZE];
static size_t idx = 0;


// Main loop error handling. This is the status which is communicated to the Python Host
static status_t global_status = STATUS_OK;

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

    
    // Open Serial communication (USB-CDC for Teensy 4.1) after peripheral initialization
    // Wait for host to run Python script
    Serial.begin(9600); // Baud is irrelevant for USB-CDC
    while (!Serial) {} // Stuck in this loop until the Python script is run

    // TODO: Remove (dev functions)
    print_json(config_doc);
    Serial.println(sweep_config.sp8t_out_port);
}

Config_t update_config_struct(const JsonDocument& config) {
    Config_t config_struct;

    // Populate struct values
    config_struct.sp8t_out_port = config["sp8t_out_port"];

    return config_struct;
}

void print_json(const JsonDocument& doc) {
    serializeJsonPretty(doc, Serial);
    Serial.println();
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
        //      curr_freq = start;
        //
        //      while(curr_freq < stop):
        //          for port in sp8t_ports:
        //          

        // TODO: conduct sweep
        // 1) Enable correct sp8t port
        // 2) Start sweep thread (ends once each frequency value has been recorded)
        //     a) Write ADF5356 (generate signal)
        //     b) Analog Read log-amp (measure signal)
        //     c) Record voltage (write to SD card sweep.csv)
        //sp8t_enablePort(sweep_config.sp8t_out_port); // 1)

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
