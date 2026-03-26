


// Libraries
#include <Arduino.h>
#include <ArduinoJson.h>

// Data
#include "common_defs.h"

// Drivers
#include "./drivers/Inc/sd_card.h"
#include "./drivers/Src/sd_card.c"
#include "./drivers/Inc/sp8t.h"
#include "./drivers/Src/sp8t.c"
#include "./drivers/Inc/adf5356.h"
#include "./drivers/Src/adf5356.c"

// Application function declarations
static Config_t update_config_struct(const JsonDocument& config);
static status_t processCommand(const char* cmd, JsonVariant data);
static status_t conduct_sweep(const char* sweep_name); 

// Debug function(s)
static void print_json(const JsonDocument& doc) __attribute__((unused));

// Contains data for conducting signal sweeps
// Defined in common_defs.h
static Config_t sweep_config;

// Variables to process incoming data
// TODO: Cleanup
static const size_t BUFFER_SIZE = 10 * 1024; // 10 KB Max input string (for now)
static char buffer[BUFFER_SIZE];
static size_t idx = 0;

// Application level error handling. This is the status which is communicated to the Python Host
static status_t global_status = STATUS_OK;


void setup() {

  // From sd_card.h
  // Initialize the SD card hardware
  // Check if the SD card contains:
  //  - config.json
  //  - ./data
  // If dne, create the files
  global_status = SD_init();

  // Get Config from SD card and populate sweep_config struct
  JsonDocument config_doc;
  global_status = SD_get_config(config_doc);
  sweep_config = update_config_struct(config_doc);

  // Communication with ADF5356
  ADF_spi_init();

  // For LogAmps
  analogReadResolution(ADC_RESOLUTION);  

  // GPIO config for sp8t mux
  sp8t_init();
  
  // TODO: If an error is present, send immediately to CLI once connected
  Serial.begin(115200);   
  while (!Serial) {}
}

void loop() {
  if(Serial.available()) {
    // Read character off the top of the buffer
    char c = (char)Serial.read();

    if(c == '\n') { // '\n' termination indicates end of JSON command
      buffer[idx] = '\0';

      // Write buffer string to JSON
      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, buffer); 
      if(!err){}
          // Pass for now
          // TODO: Handle error
      
      // Extract the command and the data from the incoming CLI request
      const char* cmd = doc["cmd"] | "";
      JsonVariant data = doc["data"];

      // Processes and executes command, issues response to CLI
      acknowledge_CLI(cmd);
      global_status = processCommand(cmd, data);
      complete_data(cmd);

      idx = 0;
    } 
    else if(idx < BUFFER_SIZE - 1) { 
      buffer[idx] = c;
      idx ++;
    } 
    else {
      // Overflow?
    }

  }
}

static void acknowledge_CLI(const char* cmd) {
  JsonDocument ack;
  ack["type"] = "ack";
  ack["cmd"] = cmd;
  ack["status"] = "OK";

  // Send response
  serializeJson(ack, Serial);
  Serial.println();
}

// TODO: Rename 
static void complete_data(const char* cmd) {
  JsonDocument complete;
  complete["type"] = "complete";
  complete["cmd"] = cmd;
  complete["status"] = "OK";

  // Send response
  serializeJson(complete, Serial);
  Serial.println();
}


/**
 * @brief Given a string command and associated data, process that command into action
 * @param cmd - String with specified command for HW
 * @param data - Data associated with command
 * @return: status_t - Error status
 * TODO: Error handling
 */
static status_t processCommand(const char* cmd, JsonVariant data) {

  // Error handling
  status_t cmd_status = STATUS_OK;

  JsonDocument response;

  response["type"] = "data";
  response["cmd"] = cmd;

  // Parse and Process
    if(strcmp(cmd, "config") == 0) {
        JsonObject cfg = data.as<JsonObject>();

        // Update config.json on the SD card
        cmd_status = SD_update_config(cfg);
        // TODO: Handle error

        // Assuming updating the config doesn't throw an error, the next step is to 
        // retrieve the updated config from the SD card and use its data to update
        // the sweep_config struct.

        // First we pull the data from the SD card
        JsonDocument config_doc;
        cmd_status = SD_get_config(config_doc);
        // TODO: Handle error

        // Assuming no error is thrown, proceed to update the struct
        sweep_config = update_config_struct(config_doc);

        // Construct the response to the CLI
        // TODO: Response is constructed assuming no error is thrown. Add the case where the error is thrown.
        response["status"] = status_to_str(cmd_status);
        response["data"]["config_params"] = config_doc; // This is the config data the SD card contains

    }
    else if(strcmp(cmd, "sweep") == 0) {
        const char *sweep_name = data.as<const char*>();
        
        // Add the .csv file to the SD card (no data yet)
        cmd_status = SD_add_sweep(sweep_name);
        // TODO: Handle error

        // Uses the file-scoped sweep_config struct to run a sweep and record data
        // TODO: Adjust time-out. This function takes too long and the CLI times out (doesn't wait for response from firmware)
        cmd_status = conduct_sweep(sweep_name);
        
        JsonDocument config_doc;
        cmd_status = SD_get_config(config_doc);

        response["status"] = status_to_str(cmd_status);
        response["data"]["config_params"] = config_doc; 
        
    }
    else if(strcmp(cmd, "calibrate") == 0) {
        // TODO: Calibrate lol
        // Pass for now
    }
    else if(strcmp(cmd, "list") == 0) {
        const uint8_t MAX_FILES = 20;
        char filenames[MAX_FILES][64];
        uint8_t file_count = 0;

        cmd_status = SD_get_filenames(filenames, MAX_FILES, &file_count);
        
        response["status"] = "OK";
        
        JsonObject data = response["data"].to<JsonObject>();
        JsonArray arr = data["files"].to<JsonArray>();

        // Populate array
        for (uint8_t i = 0; i < file_count; ++i) {
            arr.add(filenames[i]);  // duplicates the C-string into the doc
        }

    }
    else if(strcmp(cmd, "retrieve") == 0) {
        const char *sweep_name = data.as<const char*>();

        String csv_data;

        // Un-implemented for now
        cmd_status = SD_get_sweep_csv(sweep_name, csv_data);

        response["status"] = "OK";
        response["data"]["sweep_name"] = sweep_name;
        response["data"]["sweep_data"] = csv_data;

    }
    else if(strcmp(cmd, "delete") == 0) {
        const char *sweep_name = data.as<const char*>();
        cmd_status = SD_delete_sweep(sweep_name);
        
        response["status"] = status_to_str(cmd_status);
        response["data"]["sweep_name"] = sweep_name;
    }
    // Dev command
    else if(strcmp(cmd, "freq") == 0) { // {cmd: "freq", data: 3500} // Set output to 3500 MHz
#if ENABLE_DEBUG_PRINTS
      Serial.println("Got 'freq' command, initializing frequency");
      Serial.print("Frequency: ");
#endif
      const uint32_t freq = data.as<uint32_t>();
#if ENABLE_DEBUG_PRINTS
      Serial.println(freq);
#endif
      ADF_write_freq(freq);
      delay(500);

      // Slope and Intercept depend on frequency
      float slope = 0.00434*pow(((float)freq / 1000.0), 3) - 0.0441*pow(((float)freq / 1000.0), 2) + 0.123*((float)freq / 1000.0) + 18.6;
      float intercept = -0.0153*pow(((float)freq / 1000.0), 3) + 0.209*pow(((float)freq / 1000.0), 2) - 1.21*((float)freq / 1000.0) - 61.8;

      float total_power = 0;
      const uint16_t WINDOW = 200;
      for(int i = 0; i < WINDOW; i ++) {
        float raw = analogRead(LOG_AMP_0);
        float voltage = (raw * (float)ADC_REF_VOLTAGE) / (float)ADC_MAX_VALUE; // Convert raw ADC value to voltage (V)

        // Power (dB) = (Measured Voltage (mV) / Slope) + Intercept
        // ^^^ This comes from the ADL5507 Log-Ammp datasheet
        float power = (voltage * 1000.0) / slope + intercept;

        total_power += power;
      }

      total_power /= (float)WINDOW;

#if ENABLE_DEBUG_PRINTS
      Serial.print("Average power: ");
      Serial.println(total_power);
#endif

//#if ENABLE_DEBUG_PRINTS
#if 0
      Serial.print("Raw reading: ");
      Serial.println(raw);
      Serial.print("Voltage: ");
      Serial.println(voltage);
      Serial.print("Slope: ");
      Serial.println(slope);
      Serial.println("Intercept: ");
      Serial.println(intercept);
      Serial.print("Power reading: ");
      Serial.println(power);
#endif 

    }

    // Send response
    serializeJson(response, Serial);
    Serial.println();

    return cmd_status;
}

/**
 * @brief Conduct sweep w/ in hardware according to config file
 * @param TODO
 * @return: status_t
 * TODO: Error handling
 */
static status_t conduct_sweep(const char* sweep_name) {
  sp8t_port_t *ports = sweep_config.sp8t_out_ports; 
  uint32_t start_freq = sweep_config.start_freq; // All in MHz
  uint32_t curr_freq = start_freq; 
  uint32_t stop_freq = sweep_config.stop_freq;
  uint32_t step_size = sweep_config.step_size;
  uint32_t delay_ms = sweep_config.delay_ms; // Delay between frequencies

#if ENABLE_DEBUG_PRINTS
  Serial.println("Sweep Config Values:");
  Serial.print("start_freq: ");
  Serial.println(curr_freq);
  Serial.print("stop_freq: ");
  Serial.println(stop_freq);
  Serial.print("step_size: ");
  Serial.println(step_size);
  Serial.print("delay_ms: ");
  Serial.println(delay_ms);
#endif

  // Flow:
  // Enable first port
  // Write frequency
  // Measure all 10 outputs
  // Record data

  float data[12] = {0};
  // Sweep according to config

  // Conduct sweep through each of the specified ports
  for(int port = 0; port < SP8T_NUM_PORTS; port++) {

    // Only sweep for select ports
    if(ports[port] == UNDEF_PORT) break;

    // Otherwise, enable the correct port
    sp8t_enablePort(ports[port]);
    data[0] = (float)(ports[port]);

    while(curr_freq <= stop_freq) {
      data[1] = (float)(curr_freq);

      ADF_write_freq(curr_freq);
      
      for(int i = 0; i < NUM_LOG_AMPS; i++) {
        int raw = analogRead(log_amp_pins[i]);

        // Convert raw ADC value to voltage
        float voltage = (raw * ADC_REF_VOLTAGE) / ADC_MAX_VALUE;
        data[i + 2] = voltage;

        // TODO: data[i + 2] -= calData;

        delay(LOG_AMP_READ_DELAY);
      }

      // TODO: Error handling
      SD_add_data(sweep_name, data);

      curr_freq += step_size;
      delay(delay_ms);
    }

    curr_freq = start_freq;
  }

  return STATUS_OK;
}

/**
 * @brief Read JSON doc and return a Config_t struct with that data
 * @param config - Data that will be used to populate the struct
 * @return: Config_t struct containing config information
 * TODO: Error handling
 */
static Config_t update_config_struct(const JsonDocument& config) {
    Config_t config_struct;

    // Extract list of ports
    JsonArrayConst sp8t_ports = config["sp8t_out_ports"].as<JsonArrayConst>();

    // TODO: Error handle below
    uint8_t idx = 0;
    for (JsonVariantConst port : sp8t_ports) {
      int p = port.as<int>();

      config_struct.sp8t_out_ports[idx++] = (sp8t_port_t)p;
    }

    // Fill unused ports with undefined
    for(int i = idx; i < 8; i++) {
      config_struct.sp8t_out_ports[i] = UNDEF_PORT;
    }

    // Populate struct values
    config_struct.start_freq = config["start_freq"];
    config_struct.stop_freq = config["stop_freq"];
    config_struct.step_size = config["step_size"];
    config_struct.delay_ms = config["delay_ms"];

    return config_struct;
}

/**
 * @brief Print a JSON doc to serial
 * @param doc - Json document to be serialized
 * @return: void
 */
static void print_json(const JsonDocument& doc) {
    serializeJsonPretty(doc, Serial);
#if ENABLE_DEBUG_PRINTS
    Serial.println(); // Remove when using CLI
#endif
}
