// This is the main file
#include "config.h"

#include <Arduino.h>
#include <ArduinoJson.h>

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

void setup() {
  Serial.begin(9600);
  while(!Serial) {
    // Wait for Serial connection
  }
}

void loop() {

  if (Serial.available()) {
    
    // Expect: "LEN <number>\n"
    String hdr;
    if (!readLine(Serial, hdr, 1000)) {
      // No full header yet
      return;
    }

    // Parse header
    size_t spacePos = hdr.indexOf(' ');
    size_t json_len = hdr.substring(spacePos + 1).toInt();
    
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

    const char *cmd = doc["cmd"] | "";
    if (strcmp(cmd, "config") == 0) {
      // Access your config: doc["data"] is an object
      JsonObject cfg = doc["data"].as<JsonObject>();

      // Example: read some fields safely
      // int sample_rate = cfg["sample_rate"] | 48000;
      // const char *mode = cfg["mode"] | "default";

      int config_1 = cfg["config1"];
      int config_2 = cfg["config2"];
      int config_3 = cfg["config3"];

      Serial.println("Parsed it !!!");
    } else {
      Serial.println("ERR unknown_cmd");
    }

  }

  // Read USB signal
  // 
  // if 'config':
  //    update config struct
  // elif 'sweep':
  //    create sweep.csv in SD
  //    read config struct, init sweep params
  //    create local data array for voltage data
  //    
  //    for port in config.active_ports:
  //      configure SP8T from config data
  //      
  //      for frequency in config.sweep_frequencies:
  //        send update to laptop
  //        generate SDP-S USB data
  //        config SDP-S via USB
  //        read analog inputs
  //        write analog data to data array
  //    
  //    processData() --> This might be done on the laptop depending on how we store the data
  //    write dataArray to .csv on SD card
  //    send "complete" signal to laptop via USB
  // elif (other):
  //    

}
