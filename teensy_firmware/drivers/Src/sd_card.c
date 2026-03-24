
#include <Arduino.h>
#include <SD.h>
#include "../Inc/sd_card.h"

/**
 * @brief Init Teensy 4.1 SDIO for SD card
          Check for existence of config file and data directory
          If DNE, create
 * @return: status_t
 */
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

/**
 * @brief Check if the config file exists
 * @return: bool
 */
bool SD_does_config_exist() {
    return SD.exists(CONFIG_PATH);
}

/**
 * @brief Create the config file with default values
 * @return: status_t
 * TODO: Set up default values, it creates an empty config for now
 */
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

/**
 * @brief Check if the calibration file exists
 * @return: bool
 */
bool SD_does_cal_data_exist() {
  return SD.exists(CAL_PATH);
}


/**
 * @brief Create the calibration file with default values (all zero)
 * @return: status_t
 */
status_t SD_init_default_cal_data() {
    File cal;
    size_t bytes_written = 0; // For error handling
    
    SD.remove(CAL_PATH);
    cal = SD.open(CAL_PATH, FILE_WRITE);
    if(!cal)
      return STATUS_ERR_SD_OPEN_FAIL;

    // Default values are all zeroes
    bytes_written += cal.println("{");

    bytes_written += cal.println("\"out1_in1\":0,");
    bytes_written += cal.println("\"out2_in1\":0,");
    bytes_written += cal.println("\"out3_in1\":0,");
    bytes_written += cal.println("\"out4_in1\":0,");
    bytes_written += cal.println("\"out5_in1\":0,");
    bytes_written += cal.println("\"out6_in1\":0,");
    bytes_written += cal.println("\"out7_in1\":0,");
    bytes_written += cal.println("\"out8_in1\":0,");

    bytes_written += cal.println("\"out1_in2\":0,");
    bytes_written += cal.println("\"out1_in3\":0,");
    bytes_written += cal.println("\"out1_in4\":0,");
    bytes_written += cal.println("\"out1_in5\":0,");
    bytes_written += cal.println("\"out1_in6\":0,");
    bytes_written += cal.println("\"out1_in7\":0,");
    bytes_written += cal.println("\"out1_in8\":0,");
    bytes_written += cal.println("\"out1_in9\":0,");
    bytes_written += cal.println("\"out1_in10\":0");        

    bytes_written += cal.println("}");

    cal.flush();
    cal.close();

    if(bytes_written == 0)
      return STATUS_ERR_SD_WRITE_FAIL;

  return STATUS_OK;
}

/**
 * @brief Check if the data directory exists
 * @return: bool
 * TODO: Error handling
 */
bool SD_does_data_dir_exist() {
  return SD.exists(DATA_PATH);
}

/**
 * @brief Create the data directory (empty)
 * @return: status_t
 * TODO: Error handling
 */
status_t SD_init_data_dir() {

  if(!SD.mkdir(DATA_PATH)) {
    return STATUS_ERR_SD_DIR_CREATE_FAIL;
  }

  return STATUS_OK;
}

/**
 * @brief Given a config JSON file, store it on the SD card
 * @param cfg - File to write to SD
 * @return: status_t
 * TODO: Error handling
 */
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

/**
 * @brief Add a new sweep to the SD card
 * @param sweep_name - sweep to create
 * @return: status_t
 * TODO: Error handling
 *       Create a .csv file, not just an arbitrary sweep file
 */
status_t SD_add_sweep(const char* sweep_name) {
    char file_path[256]; // Max length of file path is 256 bytes = 256 characters

    // Create "<DATA_PATH>/<sweep_name>.csv"
    int n = snprintf(file_path, sizeof(file_path), "%s/%s.csv", DATA_PATH, sweep_name);
    if (n <= 0 || n >= (int)sizeof(file_path))
        return STATUS_ERR_UNKNOWN;

    // Recreate file fresh (avoid appending to an existing sweep)
    // TODO: Handle existing file
    SD.remove(file_path);

    File f = SD.open(file_path, FILE_WRITE);
    if(!f) 
        return STATUS_ERR_SD_OPEN_FAIL;

    // 10 blank columns (10 voltage readings)
    size_t bytes_written = f.println("out_port,frequency,LA0,LA1,LA2,LA3,LA4,LA5,LA6,LA7,LA8,LA9");
    f.flush();
    f.close();

    if (bytes_written == 0)
      return STATUS_ERR_SD_WRITE_FAIL;

    return STATUS_OK;
}

/**
 * @brief Append a row of measured data to the specified sweep
 * @param sweep_name - sweep to append data to
 * @param data       - array of 10 voltage values to append
 * @return: status_t
 * TODO: Error handling
 * TODO: Check floating point precision
 */
status_t SD_add_data(const char* sweep_name, const float data[12]) {
  char file_path[256];

  // Build "<DATA_PATH>/<sweep_name>.csv"
  int n = snprintf(file_path, sizeof(file_path), "%s/%s.csv", DATA_PATH, sweep_name);
  if (n <= 0 || n >= (int)sizeof(file_path))
      return STATUS_ERR_UNKNOWN;

  File f = SD.open(file_path, FILE_WRITE);
  if (!f)
      return STATUS_ERR_SD_OPEN_FAIL;

  // Append one CSV row: v0,v1,...,v9\n
  // Use enough precision for voltages; adjust digits if you want.
  size_t bytes_written = 0;
  for (int i = 0; i < 12; i++) {
      if (i > 0) bytes_written += f.print(',');
      bytes_written += f.print(data[i], 6);   // 6 digits after decimal
  }
  bytes_written += f.print('\n');

  f.flush();
  f.close();

  if (bytes_written == 0)
      return STATUS_ERR_SD_WRITE_FAIL;

  return STATUS_OK;  
}

/**
 * @brief Delete a sweep off the SD card
 * @param sweep_name - sweep to delete
 * @return: status_t
 * TODO: Error handling
 */
status_t SD_delete_sweep(const char* sweep_name)
{
  // Used for error handling
  bool success;

  char file_path[256];
  int n = snprintf(file_path, sizeof(file_path), "%s/%s", DATA_PATH, sweep_name);

  if (n <= 0 || n >= (int)sizeof(file_path)) {
    // truncated or formatting error
    return STATUS_ERR_UNKNOWN;
  }

  // Check if the sweep file exists
  File f = SD.open(file_path, FILE_READ);
  if(!f) 
    return STATUS_ERR_SD_OPEN_FAIL;

  f.flush();
  f.close();

  success = SD.remove(file_path);
  if(!success){
    return STATUS_ERR_SD_REMOVE_FAIL;
  }

  return STATUS_OK;
}

/**
 * @brief Reads config from SD card and copies to the argument
 * @param doc - Receives the SD data
 * @return: status_t
 * TODO: Error handling
 */
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

/**
 * @brief Returns the list of data files on the SD card
 * @param filenames - Where the list of names will be stored (64 max)
 * @param maxFiles - Most files to return
 * @param file_count - Num files returned
 * @return: status_t
 * TODO: Error handling
         Refactor (I don't like the hard-coded 64-byte size)
 */ 
status_t SD_get_filenames(char filenames[][64], const uint8_t maxFiles, uint8_t *file_count)
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

/**
 * @brief Returns the .csv file of the specified sweep
 * @param sweep_name - Name of sweep to retrieve
 * @param csv_out - Receives file contents
 * @return: status_t
 * TODO: Error handling
 */ 
status_t SD_get_sweep_csv(const char* sweep_name, String &csv_out) {
  char file_path[256];

  int n = snprintf(file_path, sizeof(file_path), "%s/%s.csv", DATA_PATH, sweep_name);
  if (n <= 0 || n >= (int)sizeof(file_path)) {
    return STATUS_ERR_UNKNOWN;
  }

  File f = SD.open(file_path, FILE_READ);
  if (!f) {
    return STATUS_ERR_SD_OPEN_FAIL;
  }

  csv_out = "";

  while(f.available()) {
    csv_out += (char)f.read();
  }

  f.close();

  return STATUS_OK;
}