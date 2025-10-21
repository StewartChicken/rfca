/*

Done last week:
  - Created the Config_t struct template (it's blank right now)
  - Sent USB commands via Serial to Teensy from rfca.py
  - Can send JSON file containing config data from rfca.py 
  - Can parse JSON file for command type on Teensy

  - SD card is initialized on Boot
  - config.json (on the SD card) is used to initialize Config_t on boot
  - If it dne, config.json is created as a blank json
  - Using the config cli command writes config.json to the SD card and updates the config struct
  -
  - Upon initialization, create a 'data' folder on SD card if it doesn't exist
  - sweep command creates a new data file with the specified name

Todo:
  - Build in an error handling system (return types for error prone functions)
  - Build a better comm system between the CLI and the Teensy
     - Standardize Teensy command processing: how to process the data and return info. (error data, success, etc)
     - Translation layer diagram
         - CLI -> JSON -> Serial -> Teensy
         - Teensy -> Python 
  - Design a unit and functional testing system
  - Remove un-needed prints
  - Define config struct data
  - 'list' command returns a list of the contents within the Data folder (just the .csv file names)
  - 'delete --name 'name.csv' deletes the specified .csv file from the data folder on the SD card
*/

#include <Arduino.h>
#include <ArduinoJson.h>

#include "config.h"
#include "./drivers/Inc/sd_card.h"
#include "./drivers/Src/sd_card.c"  


// Global config -- contains signal sweep parameters
// From config.h
Config_t sweep_config; 

// Helper function declarations
bool readLine(Stream &s, String &out, uint32_t timeout_ms);
bool readN(Stream &s, String &out, size_t n, uint32_t timeout_ms); 

void setup() {
  Serial.begin(9600);
  while(!Serial) {}

  // From sd_card.h
  // Initialize the SD card
  // Check if the SD card contains a config.json file
  // If it does not, create one with default values
  SD_init();
  if(!SD_does_config_exist())
    SD_init_default_config();

  if(!SD_does_data_dir_exist())
    SD_init_data_dir();
  
  // Initialize sweep_config struct
  sweep_config = SD_get_config();

  Serial.println(sweep_config.config1);
  Serial.println(sweep_config.config2);
  Serial.println(sweep_config.config3);
  Serial.println(sweep_config.config4);

  // Populate Config_t from the config file stored on the SD card
}

void loop() {

  // Loop Steps *draft*:
  // if(commandIssued(&cmd_buffer, &JsonObject)) {
  //  handle_command(str cmd, JsonObject data)
  //  respond
  if (Serial.available()) {
    
    // We wait for a header line to be sent.
    String hdr;
    if (!readLine(Serial, hdr, 1000)) {
      // No full header yet
      return;
    }

    // Parse header
    size_t spacePos = hdr.indexOf(' ');
    size_t json_len = hdr.substring(spacePos + 1).toInt();
    
    // Read each character of the subsequent JSON string
    String jsonStr;
    if (!readN(Serial, jsonStr, json_len, 5000)) {
      Serial.println("ERR timeout_body");
      return;
    }

    // Parse JSON
    // Use a DynamicJsonDocument sized to payload length + headroom
    DynamicJsonDocument doc(json_len + 256);
    DeserializationError err = deserializeJson(doc, jsonStr);
    if (err) {
      Serial.print("ERR json_");
      Serial.println(err.c_str());
      return;
    }

    // Get the command type from the JSON
    const char *cmd = doc["cmd"] | "";

    if (strcmp(cmd, "config") == 0) 
    {
      // Pull config data from the JSON string and update config.json file on the SD card
      // Update the sweep_config struct
      JsonObject cfg = doc["data"].as<JsonObject>();
      SD_update_config(cfg);
      sweep_config = SD_get_config();
    } 
    else if (strcmp(cmd, "sweep") == 0) {
      const char *sweep_name = doc["data"];

      // This adds the sweep file to the ./data/ directory on the SD card
      SD_add_sweep(sweep_name);

      // Start sweep
      // - Maybe create RTOS thread? (non-blocking solution)
      // - W/ an RTOS thread, we can continue listening to commands within the main loop
    }
    else if (strcmp(cmd, "delete") == 0) {
      const char *sweep_name = doc["data"];
      SD_delete_sweep(sweep_name);
    }
    else {
      Serial.println("ERR unknown_cmd");
    }

  }

}

void updateConfig(JsonObject cfg, Config_t *sweep_config)
{
  SD_update_config(cfg);
  *sweep_config = SD_get_config();
}

// Utility: read a line ending in '\n' (non-blocking with timeout)
bool readLine(Stream &s, String &out, uint32_t timeout_ms = 1000) {
  uint32_t start = millis();
  out = "";
  while (millis() - start < timeout_ms) {
    while (s.available()) {
      char c = (char)s.read();
      if (c == '\n') return true;
      if (c != '\r') out += c;
    }
    yield();
  }
  return false;
}

// Read exactly N bytes into a buffer (grows String); returns false on timeout
bool readN(Stream &s, String &out, size_t n, uint32_t timeout_ms = 2000) {
  uint32_t start = millis();
  out.reserve(n);
  while (out.length() < n && (millis() - start < timeout_ms)) {
    while (s.available() && out.length() < n) {
      out += (char)s.read();
    }
    yield();
  }
  return out.length() == n;
}
