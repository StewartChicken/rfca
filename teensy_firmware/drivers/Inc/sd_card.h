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

void SD_init(void);
void SD_update_config(JsonObject);
Config_t SD_get_config(void);

#endif // _SD_INTERFACE_H