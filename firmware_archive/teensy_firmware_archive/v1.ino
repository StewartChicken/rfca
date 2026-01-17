// TODO: Check latest GPT message, Sending JSON over serial outlines the current issue.
//          The JSON doc is being emptied by processCommand. We need to extract data before 
//          calling that function and pass individual data components as arguments (cmd, data) 
//
// Working on the processCommand function last -- Trying to populate the response JsonDocument from within the function scope


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
    
    // Open Serial communication (USB-CDC for Teensy 4.1) after peripheral initialization
    // Wait for host to run python script
    Serial.begin(9600); // Baud is irrelevant for USB-CDC
    while (!Serial) {} 
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

            // This is that data that will be sent back to the CLI
            JsonDocument response;

            response["test"] = "test";

            // Process incoming command and data, populate response to CLI
            global_status = processCommand(cmd, data, &response); 

            // Return response to CLI
            serializeJson(response, Serial);
            
            global_status = STATUS_OK; // Reset status
            idx = 0; // Reset for next message
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

//status_t processCommand(JsonDocument doc) {
status_t processCommand(const char* cmd, JsonVariant data, JsonDocument* response) {

    // Error handling
    status_t cmd_status = STATUS_OK;

    // Parse and Process
    if(strcmp(cmd, "config") == 0) {
        JsonObject cfg = data.as<JsonObject>();

        // Update config.json on the SD card
        cmd_status = SD_update_config(cfg);

        if(cmd_status != STATUS_OK)
            return cmd_status;

        // Update the config struct
        cmd_status = SD_set_config(&sweep_config);

        (*response)["status"] = status_to_str(cmd_status);
        (*response)["data"]["CMD"] = cmd;
        (*response)["data"]["request_data"] = cfg;
        (*response)["data"]["reponse_data"] = NULL;

        return cmd_status;
    }
    else if(strcmp(cmd, "sweep") == 0) {
        const char *sweep_name = data.as<const char*>();

        // This adds the sweep file to the ./data/ directory on the SD card
        cmd_status = SD_add_sweep(sweep_name);

        (*response)["status"] = status_to_str(cmd_status);
        (*response)["data"]["CMD"] = cmd;
        (*response)["data"]["request_data"] = sweep_name;
        (*response)["data"]["reponse_data"] = NULL;

        return cmd_status;
    }
    else if(strcmp(cmd, "calibrate") == 0) {
        // Pass
    }
    else if(strcmp(cmd, "list") == 0) {
        
        // String array of file names. Each filename can be up to 64 bytes (64 characters). Max 20 files
        // TODO: Motivate the max files (20 is a temp value)
        const uint8_t MAX_FILES = 20;
        char filenames[MAX_FILES][64]; // TODO: Cleanup -- there exist three literal '64' values which will need to be changed to macros
        uint8_t file_count = 0;
        
        // Populate filenames with files stored in ./data folder on the SD card
        cmd_status = SD_get_filenames(filenames, MAX_FILES, &file_count);

        (*response)["status"] = status_to_str(cmd_status);
        (*response)["data"]["CMD"] = cmd;
        (*response)["data"]["request_data"] = NULL;
        (*response)["data"]["reponse_data"] = "Da file names !!!";

        return cmd_status;
    }
    else if(strcmp(cmd, "retrieve") == 0) {
        // Pass
    }
    else if(strcmp(cmd, "delete") == 0) {
        // Pass
    }


    return cmd_status;
}

void sendResponse(status_t status, JsonDocument doc) {
    JsonDocument resp_doc;

    const char* cmd = doc["cmd"] | "";
    JsonObject data = doc["data"].as<JsonObject>();

    if(status != STATUS_OK) {
        resp_doc["status"] = "error";
    } 
    else {
        resp_doc["status"] = "ok";
    }
    
    resp_doc["data"]["CMD"] = cmd;
    resp_doc["data"]["CMD_DATA"] = data;
    serializeJson(resp_doc, Serial); // Write JSON to USB
}

