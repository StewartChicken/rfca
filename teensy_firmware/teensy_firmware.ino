/*

Done last week:
  - Created the Config_t struct template (it's blank right now)
  - Sent USB commands via Serial to Teensy from rfca.py
  - Can send JSON file containing config data from rfca.py 
  - Can parse JSON file for command type on Teensy

  - SD card is initialized on Boot
  - config.json (on the SD card) is used to initialize Config_t on boot
  - If it dne, config.json is created as a blank json
  - Using the config cli command writes config.json to  the SD card and updates the config struct

Todo:
  - Define config struct data
  - config --get command returns the config.json that is currently on the SD card
  - Upon initialization, create a 'Data' folder on SD card if it doesn't exist
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
  SD_init();
  sweep_config = SD_get_config();

  Serial.println(sweep_config.config1);
  Serial.println(sweep_config.config2);
  Serial.println(sweep_config.config3);
  Serial.println(sweep_config.config4);

  // Populate Config_t from the config file stored on the SD card

  // If the config is updated by the user, we do the following: 
  // 1. Parse the config.json file they sent
  // 2. Write the parameters to the SD card's config.json file
  // 3. Update Config_t sweep_config
  
}

void loop() {

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
      JsonObject cfg = doc["data"].as<JsonObject>();
      updateConfig(cfg, &sweep_config);

      Serial.println("Updated the sweep configuration");
    } 
    else if (strcmp(cmd, "sweep") == 0) {
      
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
