
// Documentation
// https://arduinojson.org/v7/
// https://docs.arduino.cc/libraries/sd/#SD%20class
// https://docs.arduino.cc/libraries/sd/#File%20class

#ifndef _SD_INTERFACE_H
#define _SD_INTERFACE_H

// Libraries
#include <Arduino.h>
#include <ArduinoJson.h>
#include <SD.h>

// Data
#include "../../common_defs.h"

// The config file and sweep data will be stored at these paths on the SD card
static const char* CONFIG_PATH = "/config.json";
static const char* CAL_PATH = "/cal.json";
static const char* DATA_PATH = "/data";


// SD Init functions
status_t SD_init(void);
bool SD_does_config_exist(void);
status_t SD_init_default_config(void);
bool SD_does_cal_data_exist(void);
status_t SD_init_default_cal_data(void);
bool SD_does_data_dir_exist(void);
status_t SD_init_data_dir(void);

// Data update functions
status_t SD_update_config(const JsonObject& doc);
status_t SD_update_cal(const JsonObject& doc);
status_t SD_add_sweep(const char* sweep_name); // Sweep name
status_t SD_add_data(const char* sweep_name, const float data[12]);
status_t SD_delete_sweep(const char* sweep_name);
// TODO: void SD_add_sweep_data(uint32_t, uint32_t); // Time interval, Voltage

// Data retrieval functions
status_t SD_get_config(JsonDocument& doc); // Retrieve the config.json file from the SD card
status_t SD_get_cal(JsonDocument& doc); // Retrieve the cal.json file from the SD card
status_t SD_get_filenames(char filenames[][64], const uint8_t maxFiles, uint8_t* file_count); // TODO: refactor
status_t SD_get_sweep_csv(const char* sweep_name, String &csv_out);

#endif // _SD_INTERFACE_H