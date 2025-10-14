#ifndef _SD_INTERFACE_H
#define _SD_INTERFACE_H

// SD driver library
#include <Arduino.h>
#include <ArduinoJson.h>
#include <SD.h>
#include "../../config.h"

#define SD_CS BUILTIN_SDCARD  // For Teensy 4.1 SDIO slot

// The config file will be stored at this path on the SD card
static const char* CONFIG_PATH = "/config.json";
static const char* DATA_PATH = "/data";

// Init functions
void SD_init(void);
bool SD_does_config_exist(void);
void SD_init_default_config(void);
bool SD_does_data_dir_exist(void);
void SD_init_data_dir(void);

// Update functions
void SD_update_config(JsonObject);
void SD_add_sweep(char*);
void SD_delete_sweep(char*);
//void SD_add_sweep_data(data, sweep_name); // Appends sweep_data to the end of the sweep_name file

// Getters
Config_t SD_get_config(void);

#endif // _SD_INTERFACE_H