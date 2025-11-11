#ifndef _SD_INTERFACE_H
#define _SD_INTERFACE_H

// Libraries
#include <Arduino.h>
#include <ArduinoJson.h>
#include <SD.h>

// Data
#include "../../error.h"
#include "../../config.h"

#define SD_CS BUILTIN_SDCARD  // For Teensy 4.1 SDIO slot

// The config file will be stored at this path on the SD card
static const char* CONFIG_PATH = "/config.json";
static const char* DATA_PATH = "/data";

// Init functions
status_t SD_init(void);
bool SD_does_config_exist(void);
status_t SD_init_default_config(void);
bool SD_does_data_dir_exist(void);
status_t SD_init_data_dir(void);

// Update functions
status_t SD_update_config(JsonObject);
status_t SD_add_sweep(char*);
status_t SD_delete_sweep(char*);
//void SD_add_sweep_data(data, sweep_name); // Appends sweep_data to the end of the sweep_name file

// Mutators
status_t SD_set_config(Config_t*);
status_t SD_get_filenames(char filenames[][64], const uint8_t maxFiles, uint8_t* file_count); // TODO: 2nd 64!

#endif // _SD_INTERFACE_H