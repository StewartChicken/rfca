
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

  // Initialize cal.json if dne
  if(!SD_does_cal_data_exist())
    SD_status = SD_init_default_cal_data();

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
 */
status_t SD_init_default_config() {
    File cfg;
    size_t bytes_written = 0; // For error handling
    
    if(SD.exists(CONFIG_PATH)) {
      if(!SD.remove(CONFIG_PATH))
        return STATUS_ERR_SD_REMOVE_FAIL;
    }

    cfg = SD.open(CONFIG_PATH, FILE_WRITE);
    if(!cfg)
      return STATUS_ERR_SD_OPEN_FAIL;

    // Default config.json
    bytes_written += cfg.println("{");
    bytes_written += cfg.println("\"sp8t_out_ports\":[1],");
    bytes_written += cfg.println("\"start_freq\":1000,");
    bytes_written += cfg.println("\"stop_freq\":6000,");
    bytes_written += cfg.println("\"step_size\":500,");
    bytes_written += cfg.println("\"delay_ms\": 10");  
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
    
    if(SD.exists(CAL_PATH)) {
      if(!SD.remove(CAL_PATH)) 
        return STATUS_ERR_SD_REMOVE_FAIL;
    }

    cal = SD.open(CAL_PATH, FILE_WRITE);
    if(!cal)
      return STATUS_ERR_SD_OPEN_FAIL;

    // Default values are all zeroes
    bytes_written += cal.println("{");

    bytes_written += cal.println("\"out1_in1\":[0],");
    bytes_written += cal.println("\"out2_in1\":[0],");
    bytes_written += cal.println("\"out3_in1\":[0],");
    bytes_written += cal.println("\"out4_in1\":[0],");
    bytes_written += cal.println("\"out5_in1\":[0],");
    bytes_written += cal.println("\"out6_in1\":[0],");
    bytes_written += cal.println("\"out7_in1\":[0],");
    bytes_written += cal.println("\"out8_in1\":[0],");

    bytes_written += cal.println("\"out1_in2\":[0],");
    bytes_written += cal.println("\"out1_in3\":[0],");
    bytes_written += cal.println("\"out1_in4\":[0],");
    bytes_written += cal.println("\"out1_in5\":[0],");
    bytes_written += cal.println("\"out1_in6\":[0],");
    bytes_written += cal.println("\"out1_in7\":[0],");
    bytes_written += cal.println("\"out1_in8\":[0],");
    bytes_written += cal.println("\"out1_in9\":[0],");
    bytes_written += cal.println("\"out1_in10\":[0]");        

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
 */
bool SD_does_data_dir_exist() {
  return SD.exists(DATA_PATH);
}

/**
 * @brief Create the data directory (empty)
 * @return: status_t
 */
status_t SD_init_data_dir() {

  if(!SD.mkdir(DATA_PATH)) 
    return STATUS_ERR_SD_DIR_CREATE_FAIL;

  return STATUS_OK;
}

/**
 * @brief Given a config JSON file, store it on the SD card
 * @param cfg - File to write to SD
 * @return: status_t
 */
status_t SD_update_config(const JsonObject& cfg) {
  File f;
  size_t bytes_written;

  // First, delete the old config.json file
  if(SD.exists(CONFIG_PATH)) {
    if(!SD.remove(CONFIG_PATH))
      return STATUS_ERR_SD_REMOVE_FAIL;
  }

  f = SD.open(CONFIG_PATH, FILE_WRITE);
  if(!f)
    return STATUS_ERR_SD_OPEN_FAIL;

  bytes_written = serializeJson(cfg, f);  
  f.flush();
  f.close();

  if (bytes_written == 0) 
    return STATUS_ERR_SD_WRITE_FAIL;

  return STATUS_OK;
}

/**
 * @brief Given a cal JSON file, store it on the SD card
 * @param cfg - File to write to SD
 * @return: status_t
 */
status_t SD_update_cal(const JsonObject& cal) {
  File f;
  size_t bytes_written;

  // First, delete the old config.json file
  if(SD.exists(CAL_PATH)) {
    if(!SD.remove(CAL_PATH))
      return STATUS_ERR_SD_REMOVE_FAIL;
  }

  f = SD.open(CAL_PATH, FILE_WRITE);
  if(!f)
    return STATUS_ERR_SD_OPEN_FAIL;

  bytes_written = serializeJson(cal, f);  
  f.flush();
  f.close();

  if (bytes_written == 0) 
    return STATUS_ERR_SD_WRITE_FAIL;

  return STATUS_OK;
}

/**
 * @brief Add a new sweep to the SD card
 * @param sweep_name - sweep to create
 * @return: status_t
 */
status_t SD_add_sweep(const char* sweep_name) {
    char file_path[MAX_FILE_PATH_LENGTH]; // Max length of file path is 256 bytes = 256 characters

    // Create "<DATA_PATH>/<sweep_name>.csv" string in file_path buffer
    int n = snprintf(file_path, sizeof(file_path), "%s/%s.csv", DATA_PATH, sweep_name);
    if (n <= 0 || n >= (int)sizeof(file_path))
        return STATUS_ERR_SD_PATH_TOO_LONG;

    // Recreate file fresh (avoid appending to an existing sweep)
    // TODO: Handle existing file
    if(SD.exists(file_path)) {
      if(!SD.remove(file_path))
        return STATUS_ERR_SD_REMOVE_FAIL;
    }

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
 */
status_t SD_add_data(const char* sweep_name, const float data[CSV_COL_NUM]) {
  char file_path[MAX_FILE_PATH_LENGTH];

  // Build "<DATA_PATH>/<sweep_name>.csv"
  int n = snprintf(file_path, sizeof(file_path), "%s/%s.csv", DATA_PATH, sweep_name);
  if (n <= 0 || n >= (int)sizeof(file_path))
      return STATUS_ERR_SD_PATH_TOO_LONG;

  File f = SD.open(file_path, FILE_WRITE);
  if (!f)
      return STATUS_ERR_SD_OPEN_FAIL;

  // Append one CSV row: p0,p1,...,p9\n
  // Use enough precision for voltages; adjust digits if you want.
  size_t bytes_written = 0;
  for (int i = 0; i < CSV_COL_NUM; i++) {
      if (i > 0) bytes_written += f.print(',');
      bytes_written += f.print(data[i], 4);   // 4 digits after decimal
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
 */
status_t SD_delete_sweep(const char* sweep_name)
{
  
  char file_path[MAX_FILE_PATH_LENGTH];
  int n = snprintf(file_path, sizeof(file_path), "%s/%s.csv", DATA_PATH, sweep_name);
  if (n <= 0 || n >= (int)sizeof(file_path))
    return STATUS_ERR_SD_PATH_TOO_LONG;

  // Check if the sweep file exists
  File f = SD.open(file_path, FILE_READ);
  if(!f) 
    return STATUS_ERR_SD_OPEN_FAIL;

  f.flush();
  f.close();

  if(SD.exists(file_path)) {
    if(!SD.remove(file_path))
      return STATUS_ERR_SD_REMOVE_FAIL;
  }

  return STATUS_OK;
}

/**
 * @brief Reads config from SD card and copies to the argument
 * @param doc - Receives the SD data
 * @return: status_t
 */
status_t SD_get_config(JsonDocument& doc) {
   
  File config = SD.open(CONFIG_PATH, FILE_READ);
  if(!config) 
    return STATUS_ERR_SD_OPEN_FAIL;

  DeserializationError err = deserializeJson(doc, config);
  if(err) 
    return STATUS_ERR_SD_READ_FAIL;

  config.close();

  return STATUS_OK;
}

/**
 * @brief Reads cal from SD card and copies to the argument
 * @param doc - Receives the SD data
 * @return: status_t
 */
status_t SD_get_cal(JsonDocument& doc) {
  File cal = SD.open(CAL_PATH, FILE_READ);
  if(!cal) 
    return STATUS_ERR_SD_OPEN_FAIL;

  DeserializationError err = deserializeJson(doc, cal);
  if(err) 
    return STATUS_ERR_SD_READ_FAIL;

  cal.close();

  return STATUS_OK;
}

/**
 * @brief Returns the list of data files on the SD card
 * @param filenames - Where the list of names will be stored (64 max)
 * @param maxFiles - Most files to return
 * @param file_count - Num files returned
 * @return: status_t
 */ 
status_t SD_get_filenames(char filenames[][MAX_FILE_PATH_LENGTH], const uint8_t maxFiles, uint8_t *file_count)
{
  *file_count = 0;

  File dir = SD.open(DATA_PATH);
  if(!dir)
    return STATUS_ERR_SD_OPEN_FAIL;

  while(*file_count < maxFiles) {
    File entry = dir.openNextFile();
    if(!entry)
      break;
    
    snprintf(filenames[*file_count], MAX_FILE_PATH_LENGTH, "%s", entry.name());
    (*file_count) ++;

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
 */ 
status_t SD_get_sweep_csv(const char* sweep_name, String &csv_out) {
  char file_path[MAX_FILE_PATH_LENGTH];

  int n = snprintf(file_path, sizeof(file_path), "%s/%s.csv", DATA_PATH, sweep_name);
  if (n <= 0 || n >= (int)sizeof(file_path)) 
    return STATUS_ERR_SD_PATH_TOO_LONG;

  File f = SD.open(file_path, FILE_READ);
  if (!f) 
    return STATUS_ERR_SD_OPEN_FAIL;

  csv_out = "";

  while(f.available()) {
    csv_out += (char)f.read();
  }

  f.close();

  return STATUS_OK;
}

/**
 * @brief Returns a specific row from the specified sweep CSV
 * @param sweep_name - Name of sweep to retrieve
 * @param row_out - Receives the row contents (without trailing newline)
 * @param row_num - Zero-based row index
 * @return: status_t
 */
status_t SD_get_sweep_csv_row(const char* sweep_name, String &row_out, uint32_t row_num) {
  char file_path[MAX_FILE_PATH_LENGTH];

  int n = snprintf(file_path, sizeof(file_path), "%s/%s.csv", DATA_PATH, sweep_name);
  if (n <= 0 || n >= (int)sizeof(file_path))
    return STATUS_ERR_SD_PATH_TOO_LONG;

  File f = SD.open(file_path, FILE_READ);
  if (!f)
    return STATUS_ERR_SD_OPEN_FAIL;

  row_out = "";

  uint32_t current_row = 0;
  String current_line = "";
  
  while (f.available()) {
    char c = (char)f.read();

    // Handle line endings
    if (c == '\n') {
      if (current_row == row_num) {
        row_out = current_line;
        f.close();
        return STATUS_OK;
      }

      current_line = "";
      current_row++;
    } else {
      current_line += c;
    }
  }

  f.close();
  return STATUS_OK; // TODO: This should actually be an error
}


/**
 * @brief Get size of a sweep file in bytes
 * @param sweep_name - name of the sweep
 * @param size_out - pointer to store size
 * @return status_t
 */
status_t SD_get_sweep_size(const char* sweep_name, uint32_t* size_out)
{
  
  char file_path[MAX_FILE_PATH_LENGTH];
  int n = snprintf(file_path, sizeof(file_path), "%s/%s.csv", DATA_PATH, sweep_name);
  if (n <= 0 || n >= (int)sizeof(file_path))
    return STATUS_ERR_SD_PATH_TOO_LONG;

  File f = SD.open(file_path, FILE_READ);
  if (!f)
    return STATUS_ERR_SD_OPEN_FAIL;

  *size_out = f.size();   // 🔑 key line

  f.close();
  return STATUS_OK;
}