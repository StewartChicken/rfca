/*

Done last week:
  - Created the Config_t struct template (it's blank right now)
  - Sent USB commands via Serial to Teensy from rfca.py
  - Can send JSON file containing config data from rfca.py 
  - Can parse JSON file for command type on Teensy

Todo:
  - Define config struct data
  - Take JSON data and update struct
  - Write SD card driver
  - Wihtin rfca.py, when sweep is issued, package the command and .csv name in JSON 
      and send to Teensy
  - When 'sweep' command is received, create a new .csv file with the corresponding name 
      (it should be blank) in the Teensy's SD card
*/




// This is the main file
#include "config.h"

#include <Arduino.h>
#include <ArduinoJson.h>

Config_t sweep_config; // Global config -- contains signal sweep parameters

// Helper function declarations
bool readLine(Stream &s, String &out, uint32_t timeout_ms);
bool readN(Stream &s, String &out, size_t n, uint32_t timeout_ms); 

void setup() {
  Serial.begin(9600);
  while(!Serial) {
    // Wait for Serial connection
  }
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

    if (strcmp(cmd, "config") == 0) {

      // Pull config data from the JSON string and update the config struct
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
  int config_1 = cfg["config1"];
  int config_2 = cfg["config2"];
  int config_3 = cfg["config3"];
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
