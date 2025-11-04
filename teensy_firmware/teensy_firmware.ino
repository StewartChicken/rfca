
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

void setup() {
    Serial.begin(115200);
    while (!Serial) {} 

    // From sd_card.h
    // Initialize the SD card hardware
    // Check if the SD card contains:
    //  - config.json
    //  - ./data
    // If dne, create the files
    SD_init();
    if(!SD_does_config_exist())
        SD_init_default_config();

    if(!SD_does_data_dir_exist())
        SD_init_data_dir();

    // Initialize the sweep_config struct by reading the
    //  config.json file on the SD card
    sweep_config = SD_get_config();
    
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
            if(!err) 
                processCommand(doc); // Process incoming command and data
            
            sendResponse();
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

void processCommand(JsonDocument doc) {
    
    // Read command
    const char* cmd = doc["cmd"] | "";
    
    // Parse and Process
    if(strcmp(cmd, "config") == 0) {
        JsonObject cfg = doc["data"].as<JsonObject>();
        SD_update_config(cfg);
        sweep_config = SD_get_config();
    }
    else if(strcmp(cmd, "sweep") == 0) {
        // Pass
    }
    else if(strcmp(cmd, "calibrate") == 0) {
        // Pass
    }
    else if(strcmp(cmd, "list") == 0) {
        // Pass
    }
    else if(strcmp(cmd, "retrieve") == 0) {
        // Pass
    }
    else if(strcmp(cmd, "delete") == 0) {
        // Pass
    }
    
}

void sendResponse() {
    JsonDocument resp_doc;
    //char resp_str[BUFFER_SIZE];

    resp_doc["status"] = "ok";
    resp_doc["data"] = "data_temp";
    serializeJson(resp_doc, Serial); // Write JSON to USB

    //size_t len = serializeJson(response, resp_str);
    
    //Serial.write()
}

