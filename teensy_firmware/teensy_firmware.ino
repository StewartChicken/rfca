


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

// Calibration data stored on SD
static JsonDocument global_cal_doc;

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

  // Retrieve any saved calibration data from the SD card
  global_status = SD_get_cal(global_cal_doc);

  // Power control.  These pins need to be written high before the switch logic or synth control
  pinMode(ADM_POS_3v3, OUTPUT);
  pinMode(ADM_NEG_3v3, OUTPUT);
  digitalWrite(ADM_POS_3v3, HIGH); 
  delay(1000); // For safety
  digitalWrite(ADM_NEG_3v3, HIGH);
  delay(1000); // For safety 

  // For LogAmps
  analogReadResolution(ADC_RESOLUTION);  

  // GPIO config for sp8t mux
  sp8t_init();
  delay(1000); // For safety

  // Communication with ADF5356
  ADF_spi_init();
  delay(1000); // For safety
  
  
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

        // First we pull the config data from the SD card
        JsonDocument config_doc;
        cmd_status = SD_get_config(config_doc);

        // Assuming no error is thrown, proceed to update the struct
        sweep_config = update_config_struct(config_doc);

        // Construct the response to the CLI
        // TODO: Response is constructed assuming no error is thrown. Add the case where the error is thrown.
        response["status"] = status_to_str(cmd_status);
        response["data"]["config_params"] = config_doc; // This is the config data the SD card contains

    }
    else if(strcmp(cmd, "calibrate") == 0) {
        
        JsonArray ports = data.as<JsonArray>(); // Contains port information [out, in]
        
        uint8_t out_port     = ports[0];  // Output of SP8T (1-8)
        uint8_t in_port      = ports[1];  // Input of Log-Amp (1-10)
        int8_t set           = ports[2];  // If -1, proceed w/ cal.  Otherwise, hardcode cal value to 'set'

        // CLI sends output and input ports to cal, the key accesses the correct one
        char key[20];
        snprintf(key, sizeof(key), "out%d_in%d", out_port, in_port);

        // First we pull the data from the SD card
        JsonDocument cal_doc;
        cmd_status = SD_get_cal(cal_doc);
        JsonObject cal = cal_doc.as<JsonObject>();
        JsonArray frequencies = cal[key].to<JsonArray>(); // This is the current cal data

        // Reset before updating
        frequencies.clear();

        // Proceed w/ typical cal
        if(set == -1) {
          sp8t_enablePort(out_port); // We're calibrating a single port

          // To transmit progress to CLI during cal process
          JsonDocument progress;
          progress["type"] = "progress";
          progress["cmd"] = "calibrate";
          progress["status"] = status_to_str(STATUS_OK);

          // Cal process
          uint32_t curr_freq = 800;  // Start at 800 MHz
          uint32_t step = 100;       // 100 MHz steps
          while(curr_freq <= 6800) { // Loop through 6.8 GHz (61 total steps)

            // Send progress report
            progress["data"]["frequency"] = curr_freq;
            serializeJson(progress, Serial);
            Serial.println();

            ADF_write_freq(curr_freq);
            delay(2); // 2ms delay for sanity

            int raw = analogRead(log_amp_pins[in_port - 1]); // log_amp_pins indexed (0-9)

            // Convert raw ADC value to voltage
            float voltage = (raw * ADC_REF_VOLTAGE) / ADC_MAX_VALUE;
            frequencies.add(voltage); // Thru loss
            //frequencies.add(curr_freq); // Dev test

            curr_freq += step;
          }

          cmd_status = SD_update_cal(cal); // Write to SD
          cmd_status = SD_get_cal(global_cal_doc); // Read from SD and write to global
          
          response["status"] = status_to_str(cmd_status);
          response["data"]["cal_data"] = cal; // This is the config data the SD card contains
      }
      else { // If set != -1, we set hard coded values for cal data
        
        // Cal process
        uint32_t curr_freq = 800;  // Start at 800 MHz
        uint32_t step = 100;       // 100 MHz steps
        while(curr_freq <= 6800) { // Loop through 6.8 GHz (61 total steps)
          frequencies.add(set); // Thru loss
          curr_freq += step;
        }

        cmd_status = SD_update_cal(cal); // Write to SD
        cmd_status = SD_get_cal(global_cal_doc); // Read from SD and write to global
        
        response["status"] = status_to_str(cmd_status);
        response["data"]["cal_data"] = cal; // This is the config data the SD card contains
      }

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

    // Dev command, write specified frequency and read power on LA0
    else if(strcmp(cmd, "freq") == 0) { // {cmd: "freq", data: 3500} // Set output to 3500 MHz
      const uint32_t freq = data.as<uint32_t>();
#if ENABLE_DEBUG_PRINTS
      Serial.println("Got 'freq' command, initializing frequency");
      Serial.print("Frequency: ");
      Serial.println(freq);
#endif
      ADF_write_freq(freq);
      delay(500);

      // Slope and Intercept depend on frequency in GHz
      float slope = LOG_AMP_SLOPE_3*pow(((float)freq / 1000.0), 3) 
                    + LOG_AMP_SLOPE_2*pow(((float)freq / 1000.0), 2) 
                    + LOG_AMP_SLOPE_1*((float)freq / 1000.0) 
                    + LOG_AMP_SLOPE_0;

      float intercept = LOG_AMP_INTERCEPT_4*pow(((float)freq / 1000.0), 4) 
                        + LOG_AMP_INTERCEPT_3*pow(((float)freq / 1000.0), 3) 
                        + LOG_AMP_INTERCEPT_2*pow(((float)freq / 1000.0), 2) 
                        + LOG_AMP_INTERCEPT_1*((float)freq / 1000.0) 
                        + LOG_AMP_INTERCEPT_0;

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

#if ENABLE_DEBUG_PRINTS
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

    } // END OF else if(strcmp(cmd, "freq") == 0)

    // Dev command, enable specific SP8T port
    else if(strcmp(cmd, "port") == 0) { // {cmd: "port", data: 1} // Open port 1 (close the rest)
      const uint32_t port = data.as<uint32_t>();
#if ENABLE_DEBUG_PRINTS
      Serial.println("Enabling Port: ");
      Serial.println(port);
#endif
      sp8t_enablePort((sp8t_port_t)port);
#if ENABLE_DEBUG_PRINTS
      Serial.println("Done enabling port");
#endif
    } // END OF else if(strcmp(cmd, "port") == 0)

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

  JsonDocument progress;

  progress["type"] = "progress";
  progress["cmd"] = "sweep";
  progress["status"] = status_to_str(STATUS_OK);

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

        // Slope and Intercept depend on frequency in GHz
        float slope = LOG_AMP_SLOPE_3*pow(((float)curr_freq / 1000.0), 3) 
                      + LOG_AMP_SLOPE_2*pow(((float)curr_freq / 1000.0), 2) 
                      + LOG_AMP_SLOPE_1*((float)curr_freq / 1000.0) 
                      + LOG_AMP_SLOPE_0;

        float intercept = LOG_AMP_INTERCEPT_4*pow(((float)curr_freq / 1000.0), 4) 
                          + LOG_AMP_INTERCEPT_3*pow(((float)curr_freq / 1000.0), 3) 
                          + LOG_AMP_INTERCEPT_2*pow(((float)curr_freq / 1000.0), 2) 
                          + LOG_AMP_INTERCEPT_1*((float)curr_freq / 1000.0) 
                          + LOG_AMP_INTERCEPT_0;

        int raw = analogRead(log_amp_pins[i]);
        float voltage = (raw * (float)ADC_REF_VOLTAGE) / (float)ADC_MAX_VALUE; // Convert raw ADC value to voltage (V)

#if ENABLE_CALIBRATION
        voltage += get_cal_delta(port, i, curr_freq); // output port, input port, current frequency
#endif

        // Power (dB) = (Measured Voltage (mV) / Slope) + Intercept
        // ^^^ This comes from the ADL5902 Log-Ammp datasheet
        float power = (voltage * 1000.0) / slope + intercept;

        data[i + 2] = voltage;
        //data[i + 2] = power;
        delay(LOG_AMP_READ_DELAY);
        
      }

      // Adds the new row of data to the corresponding csv
      // TODO: Error handling
      SD_add_data(sweep_name, data);

      // Send progress report
      progress["data"]["frequency"] = curr_freq;
      serializeJson(progress, Serial);
      Serial.println();

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
 * @brief Return the cal data for a given in/out port pair and a frequency
 * @param out SP8T output port
 * @param in  LogAmp input port
 * @param frequency In MHz
 * @return: float - cal info (voltage offset)
 * TODO: Error handling
 */
float get_cal_delta(uint8_t out, uint8_t in, uint32_t frequency) {

  uint32_t freq = frequency;

  // Convert 'frequency' to an index
  // 800 MHz  = 0
  // 900 MHz  = 1
  // 1000 MHz = 2
  // ...
  // 6700 MHz = 59
  // 6800 MHz = 60
  uint8_t index;
  
  // Clamp
  if (freq < 800)  freq = 800;
  if (freq > 6800) freq = 6800;

  // Get index 
  uint32_t rounded = ((freq + 50) / 100) * 100;
  index = (rounded - 800) / 100;

  // cal = (1, in) - [(1, 1) - (out, 1)]
  char in_key[20];
  snprintf(in_key, sizeof(in_key), "out1_in%d", in);
  char out_key[20];
  snprintf(out_key, sizeof(out_key), "out%d_in1", out);
  char one_key[20] = "out1_in1";
  
  //JsonArray frequencies = global_cal_doc[key].to<JsonArray>();
  float in_loss = global_cal_doc[in_key].as<JsonArray>()[index].as<float>();
  float out_loss = global_cal_doc[out_key].as<JsonArray>()[index].as<float>();
  float one_loss = global_cal_doc[one_key].as<JsonArray>()[index].as<float>();

  return in_loss - (one_loss - out_loss);
}

/**
 * @brief Print a JSON doc to serial. DEV use
 * @param doc - Json document to be serialized
 * @return: void
 */
static void print_json(const JsonDocument& doc) {
    serializeJsonPretty(doc, Serial);
#if ENABLE_DEBUG_PRINTS
    Serial.println(); // Remove when using CLI
#endif
}
