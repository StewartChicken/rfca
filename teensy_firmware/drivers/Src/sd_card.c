#include <Arduino.h>
#include <SD.h>
#include "../Inc/sd_card.h"

// This function is responsible for:
//  1. Connecting to the SD card
//  2. Checking for the existence of a config.json file
//  3. Creating the config.json file if it dne
void SD_init() {
  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("SD initialization failed!");
    while (1);
  }
  Serial.println("SD initialization done.");
}

bool SD_does_config_exist() {
  return SD.exists(CONFIG_PATH);
}

void SD_init_default_config() {
  File cfg = SD.open(CONFIG_PATH, FILE_WRITE);

  cfg.println("{");
  cfg.println("}");
  cfg.flush();
  cfg.close();

  Serial.println("config.json created.");
}

bool SD_does_data_dir_exist(void) {
  return SD.exists(DATA_PATH);
}

void SD_init_data_dir(void) {
  SD.mkdir(DATA_PATH);
}

void SD_update_config(JsonObject cfg)
{
  // First, delete the old config.json 
  SD.remove(CONFIG_PATH);

  File f = SD.open(CONFIG_PATH, FILE_WRITE);
  if(!f) {
    Serial.println("ERROR: Could not open /config.json for writing");
    return;
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

void SD_add_sweep(char* sweep_name)
{
  
  char file_path[256]; // Max length of file path is 256 bytes = 256 characters
  int n = snprintf(file_path, sizeof(file_path), "%s/%s", DATA_PATH, sweep_name);

  if (n <= 0 || n >= (int)sizeof(file_path)) {
    // truncated or formatting error
    return;
  }

  File f = SD.open(file_path, FILE_WRITE);
  f.flush();
  f.close();
  
}

void SD_delete_sweep(char* sweep_name)
{
  char file_path[256];

  int n = snprintf(file_path, sizeof(file_path), "%s/%s", DATA_PATH, sweep_name);

  if (n <= 0 || n >= (int)sizeof(file_path)) {
    // truncated or formatting error
    return;
  }

  // Check if the sweep file exists
  File f = SD.open(file_path, FILE_READ);
  if(!f) {
    return;
  }

  f.flush();
  f.close();

  SD.remove(file_path);

}

Config_t SD_get_config()
{
  Config_t cfg;

  File f = SD.open(CONFIG_PATH, FILE_READ);
  
  // ~1.5x file size + headroom.
  size_t sz = f.size();
  DynamicJsonDocument doc(sz * 3 / 2 + 256);

  DeserializationError err = deserializeJson(doc, f);
  if(err) {
    // Pass
  }

  f.close();

  JsonObjectConst root = doc.as<JsonObjectConst>();
  
  cfg.config1 = root["config1"];
  cfg.config2 = root["config2"];
  cfg.config3 = root["config3"];
  cfg.config4 = root["config4"];

  return cfg;
}
