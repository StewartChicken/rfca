#include <Arduino.h>
#include <SD.h>
#include "../Inc/sd_card.h"

// This function is responsible for:
//  1. Connecting to the SD card
//  2. Checking for the existence of a config.json file
//  3. Creating the config.json file if it dne
void SD_init()
{

    // Initialize SD card
    if (!SD.begin(BUILTIN_SDCARD)) {
        Serial.println("SD initialization failed!");
        while (1);
    }
    Serial.println("SD initialization done.");

    // Check if config.json exists on the SD card
    // If it does not, create it with default values
    if(!SD.exists(CONFIG_PATH)) {
      Serial.println("config.json not found; creating default config...");

      File cfg = SD.open(CONFIG_PATH, FILE_WRITE);
      if (!cfg) {
        Serial.println("ERROR: could not create config.json");
        return;
      }

      // Create default config (blank .json for now)
      cfg.println("{");
      cfg.println("}");
      cfg.flush();
      cfg.close();

      Serial.println("config.json created.");
    }
    else 
    {
      Serial.println("config.json found");
    }
}

void SD_update_config(JsonObject cfg)
{
  // First, delete the old config.json 
  SD.remove(CONFIG_PATH);

  File f = SD.open(CONFIG_PATH, FILE_WRITE);
  if(!f) {
    Serial.println("ERROR: Could not open /config.json for writing");
    return false;
  }

  size_t written = serializeJson(cfg, f);  
  f.flush();
  f.close();

  if (written == 0) {
    Serial.println("ERROR: Nothing was written to /config.json");
    return;
  }

  Serial.print("Wrote ");
  Serial.print(written);
  Serial.println(" bytes to /config.json");
}

Config_t SD_get_config()
{
  Config_t cfg;

  if (!SD.exists(CONFIG_PATH))
  {
    Serial.println("Config file missing! Could not retrieve config data");
    return;
  }

  File f = SD.open(CONFIG_PATH, FILE_READ);
  if (!f) 
  {
    Serial.println("ERROR: Could not open config.json file");
    return;
  }

  // ~1.5x file size + headroom.
  size_t sz = f.size();
  DynamicJsonDocument doc(sz * 3 / 2 + 256);

  DeserializationError err = deserializeJson(doc, f);
  f.close();

  if (err) {
    Serial.print("ERROR: JSON parse failed: ");
    Serial.println(err.c_str());
    return;
  }

  JsonObjectConst root = doc.as<JsonObjectConst>();
  if (root.isNull())
  {
    Serial.println("ERROR: root is not an object");
    return;
  }

  cfg.config1 = root["config1"];
  cfg.config2 = root["config2"];
  cfg.config3 = root["config3"];
  cfg.config4 = root["config4"];

  return cfg;
}
