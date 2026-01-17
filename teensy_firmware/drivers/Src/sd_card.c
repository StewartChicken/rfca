
#include <Arduino.h>
#include <SD.h>
#include "../Inc/sd_card.h"

status_t SD_init() {
  status_t SD_status = STATUS_OK;

  if(!SD.begin(BUILTIN_SDCARD))
    return STATUS_ERR_SD_INIT_FAIL;

  // Initialize config.json if dne
  if(!SD_does_config_exist())
    SD_status = SD_init_default_config();

  if(SD_status != STATUS_OK)
    return SD_status;

  // Initialize ./data dir if dne
  if(!SD_does_data_dir_exist())
        SD_status = SD_init_data_dir();

  return SD_status;
}

bool SD_does_config_exist() {
    return SD.exists(CONFIG_PATH);
}

status_t SD_init_default_config() {
    File cfg;
    size_t bytes_written = 0; // For error handling
    
    SD.remove(CONFIG_PATH);
    cfg = SD.open(CONFIG_PATH, FILE_WRITE);
    if(!cfg)
      return STATUS_ERR_SD_OPEN_FAIL;

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

  if(!SD.mkdir(DATA_PATH)) {
    return STATUS_ERR_SD_DIR_CREATE_FAIL;
  }

  return STATUS_OK;
}

status_t SD_update_config(const JsonObject& cfg) {
  File f;
  size_t bytes_written;

  // First, delete the old config.json file
  SD.remove(CONFIG_PATH);
  f = SD.open(CONFIG_PATH, FILE_WRITE);
  if(!f)
    return STATUS_ERR_SD_OPEN_FAIL;

  bytes_written = serializeJson(cfg, f);  
  f.flush();
  f.close();

  if (bytes_written == 0) {
    return STATUS_ERR_SD_WRITE_FAIL;
  }

  return STATUS_OK;
}

status_t SD_add_sweep(const char* sweep_name) {
    char file_path[256]; // Max length of file path is 256 bytes = 256 characters
    int n = snprintf(file_path, sizeof(file_path), "%s/%s", DATA_PATH, sweep_name);

    File f;

    // TODO: Classify this
    if (n <= 0 || n >= (int)sizeof(file_path))
        return STATUS_ERR_UNKNOWN;

    f = SD.open(file_path, FILE_WRITE);
    if(!f) {
        return STATUS_ERR_SD_OPEN_FAIL;
    }

    f.flush();
    f.close();

    return STATUS_OK;
}

// TODO: Redesign; not compatible with current error handling system
status_t SD_delete_sweep(const char* sweep_name)
{
  // Used for error handling
  bool success;

  char file_path[256];
  int n = snprintf(file_path, sizeof(file_path), "%s/%s", DATA_PATH, sweep_name);

  if (n <= 0 || n >= (int)sizeof(file_path)) {
    // truncated or formatting error
    // TODO: Categorize error
    return STATUS_ERR_UNKNOWN;
  }

  // Check if the sweep file exists
  File f = SD.open(file_path, FILE_READ);
  if(!f) 
    return STATUS_ERR_SD_OPEN_FAIL;

  f.flush();
  f.close();

  Serial.println(file_path);

  success = SD.remove(file_path);
  if(!success){
    return STATUS_ERR_SD_REMOVE_FAIL;
  }

  return STATUS_OK;
}

// Reads config.json on SD card and returns a JsonDocument containing its data
status_t SD_get_config(JsonDocument& doc) {
   
  File config = SD.open(CONFIG_PATH, FILE_READ);
  if(!config) {
    return STATUS_ERR_SD_OPEN_FAIL;
  }

  DeserializationError err = deserializeJson(doc, config);
  config.close();
  if(err) {
    return STATUS_ERR_SD_READ_FAIL;
  }

  return STATUS_OK;
}

// TODO: Refactor (I don't like the hard-coded 64-byte size)
status_t SD_get_filenames(char filenames[][64], const uint8_t maxFiles, uint8_t* file_count)
{
  *file_count = 0;

  File dir = SD.open(DATA_PATH);
  if(!dir)
    return STATUS_ERR_SD_OPEN_FAIL;

  File entry;
  while((*file_count < maxFiles) && (entry = dir.openNextFile())) {
    const char* file_name = entry.name();

    strncpy(filenames[*file_count], file_name, 63); // TODO: Third 64!
    (*file_count)++;

    entry.close();
  }

  dir.close();

  return STATUS_OK;
}

