
// Last thing done: Send/Receive JSON from Teensy via Serial monitor
// Todo: 
//  - Implement remaining commands for use with Serial Monitor
//  - Ensure errors are propagating correctly
//  - Verify use with CLI

// Libraries
#include <Arduino.h>
#include <ArduinoJson.h>

// Data
#include "error.h"
#include "config.h"

// Drivers
#include "./drivers/Inc/sd_card.h"
#include "./drivers/Src/sd_card.c"

Config_t sweep_config;

// Variables to process incoming data
static const size_t BUFFER_SIZE = 10 * 1024; // 10 KB Max input string
static char buffer[BUFFER_SIZE];
static size_t idx = 0;

// Main loop error handling. This is the status which is communicated to the Python Host
static status_t global_status = STATUS_OK;

void setup() {
    status_t temp_status;

    Serial.println("Initializing SD...");

    // From sd_card.h
    // Initialize the SD card hardware
    // Check if the SD card contains:
    //  - config.json
    //  - ./data
    // If dne, create the files
    temp_status = SD_init();
    if(!SD_does_config_exist())
        temp_status = SD_init_default_config();

    if(!SD_does_data_dir_exist())
        temp_status = SD_init_data_dir();

    // Initialize the sweep_config struct by reading the
    //  config.json file on the SD card
    temp_status = SD_set_config(&sweep_config);

    Serial.println("Done Initializing SD");
    
    // Open Serial communication (USB-CDC for Teensy 4.1) after peripheral initialization
    // Wait for host to run python script
    Serial.begin(9600); // Baud is irrelevant for USB-CDC
    while (!Serial) {} 

    Serial.println("Serial Initialized");

}

// Main Loop structure:
// 1. Check for incoming Serial data
// 2. Read character by character and populate buffer string
// 3. If '\n', command is complete and ready for processing
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

    // Response doc to client
    JsonDocument response;

    // Parse and Process
    if(strcmp(cmd, "config") == "0") {
        // Pass
    }
    else if(strcmp(cmd, "sweep") == 0) {
        const char *sweep_name = data.as<const char*>();
        cmd_status = SD_add_sweep(sweep_name);

        response["status"] = "OK";
        response["data"]["CMD"] = cmd;
        response["data"]["resp_data"] = "Sweep conducted!";
        
        serializeJson(response, Serial);
        return cmd_status;
    }
    else if(strcmp(cmd, "calibrate") == 0) {
        // Pass
    }
    else if(strcmp(cmd, "list") == 0) {
        const uint8_t MAX_FILES = 20;
        char filenames[MAX_FILES][64];
        uint8_t file_count = 0;

        cmd_status = SD_get_filenames(filenames, MAX_FILES, &file_count);

        response["status"] = "OK";
        
        JsonObject data = response["data"].to<JsonObject>();
        data["CMD"] = cmd;

        JsonArray arr = data["resp_data"].to<JsonArray>();

        // Populate array
        for (uint8_t i = 0; i < file_count; ++i) {
            arr.add(filenames[i]);  // duplicates the C-string into the doc
        }

        serializeJson(response, Serial);
        return cmd_status;
    }
    else if(strcmp(cmd, "retrieve") == 0) {
        // Pass
    }
    else if(strcmp(cmd, "delete") == 0) {
        const char *sweep_name = data.as<const char*>();
        cmd_status = SD_delete_sweep(sweep_name);

        response["status"] = "OK";
        response["data"]["CMD"] = cmd;
        response["data"]["resp_data"] = "Sweep Deleted!";
        
        serializeJson(response, Serial);
        return cmd_status;
    }
 }