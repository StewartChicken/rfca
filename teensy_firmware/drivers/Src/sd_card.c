#include <Arduino.h>
#include <SD.h>
#include "../Inc/sd_card.h"


status_t SD_init() {
  if (!SD.begin(BUILTIN_SDCARD))
    return STATUS_ERR_SD_INIT_FAILED;

  return STATUS_OK;
}

bool SD_does_config_exist() {
  return SD.exists(CONFIG_PATH);
}

// Create the config.json file on the SD card and write default values
// Return error if unable to create
status_t SD_init_default_config() {
  File cfg = SD.open(CONFIG_PATH, FILE_WRITE);
  if(!cfg)
    return STATUS_ERR_SD_OPEN_FAIL;

  // For error handling
  size_t bytes_written = 0;

  // TODO - Create default config
  bytes_written += cfg.println("{");
  bytes_written += cfg.println("}");
  cfg.flush();
  cfg.close();

  if(bytes_written == 0)
    return STATUS_ERR_SD_WRITE_FAIL;

  return STATUS_OK;
}

bool SD_does_data_dir_exist(void) {
  return SD.exists(DATA_PATH);
}

status_t SD_init_data_dir(void) {
  // Used for error handling
  bool success;
  
  success = SD.mkdir(DATA_PATH);
  if(!success)
    return STATUS_ERR_SD_DIR_CREATE_FAIL;

  return STATUS_OK;
}

status_t SD_update_config(JsonObject cfg)
{
  // Used for error handling
  bool success;

  // First, delete the old config.json file
  success = SD.remove(CONFIG_PATH);
  if(!success)
    return STATUS_ERR_SD_REMOVE_FAIL;

  // Second, create a new config.json file
  File f = SD.open(CONFIG_PATH, FILE_WRITE);
  if(!f) 
    return STATUS_ERR_SD_OPEN_FAIL;

  size_t bytes_written = serializeJson(cfg, f);  
  f.flush();
  f.close();

  if (bytes_written == 0)
    return STATUS_ERR_SD_WRITE_FAIL;

  return STATUS_OK;
}

status_t SD_add_sweep(char* sweep_name)
{
  
  char file_path[256]; // Max length of file path is 256 bytes = 256 characters
  int n = snprintf(file_path, sizeof(file_path), "%s/%s", DATA_PATH, sweep_name);

  // TODO: Classify this
  if (n <= 0 || n >= (int)sizeof(file_path))
    return STATUS_ERR_UNKNOWN;

  File f = SD.open(file_path, FILE_WRITE);
  if(!f)
    return STATUS_ERR_SD_OPEN_FAIL;
  f.flush();
  f.close();

  return STATUS_OK;
}

status_t SD_delete_sweep(char* sweep_name)
{
  // Used for error handling
  bool success;

  char file_path[256];
  int n = snprintf(file_path, sizeof(file_path), "%s/%s", DATA_PATH, sweep_name);

  if (n <= 0 || n >= (int)sizeof(file_path)) {
    // truncated or formatting error
    return;
  }

  // Check if the sweep file exists
  File f = SD.open(file_path, FILE_READ);
  if(!f) 
    return STATUS_ERR_SD_OPEN_FAIL;

  f.flush();
  f.close();

  success = SD.remove(file_path);
  if(!success)
    return STATUS_ERR_SD_REMOVE_FAIL;

  return STATUS_OK;

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
