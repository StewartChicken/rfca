
#ifndef _COMMON_DEFS_H
#define _COMMON_DEFS_H


// ============================================================
// Logorithmic Amplifiers
// ============================================================

// Log Amp Pins
#define LOG_AMP_0                   A0
#define LOG_AMP_1                   A1
#define LOG_AMP_2                   A2
#define LOG_AMP_3                   A3
#define LOG_AMP_4                   A4
#define LOG_AMP_5                   A5
#define LOG_AMP_6                   A6
#define LOG_AMP_7                   A7
#define LOG_AMP_8                   A8
#define LOG_AMP_9                   A9

// For indexing
static const uint8_t log_amp_pins[10] = {
    LOG_AMP_0,
    LOG_AMP_1,
    LOG_AMP_2,
    LOG_AMP_3,
    LOG_AMP_4,
    LOG_AMP_5,
    LOG_AMP_6,
    LOG_AMP_7,
    LOG_AMP_8,
    LOG_AMP_9
};

// LogAmp Config
#define NUM_LOG_AMPS                10
#define LOG_AMP_READ_DELAY          5   // ms
#define ADC_REF_VOLTAGE             3.3  // Teensy uses a 3.3 V reference for analog inputs
#define ADC_RESOLUTION              10   // 10-bit, max 12?
#define ADC_MAX_VALUE               1023 // default 10-bit resolution

// LogAmp Power curve parameters
#define LOG_AMP_SLOPE_3             0.0221
#define LOG_AMP_SLOPE_2            -0.4935
#define LOG_AMP_SLOPE_1             0.2023
#define LOG_AMP_SLOPE_0             53.819
#define LOG_AMP_INTERCEPT_4         0.0932
#define LOG_AMP_INTERCEPT_3        -0.8662
#define LOG_AMP_INTERCEPT_2         2.6089
#define LOG_AMP_INTERCEPT_1        -2.8576
#define LOG_AMP_INTERCEPT_0        -61.804



// ============================================================
// ADF5356 Frequency Synthesizer 
// ============================================================

// ADF5356 Pins
#define ADF5356_LE                  36 // SPI0 CS  
#define ADF5356_DATA                11 // SPI0 MOSI 
#define ADF5356_MISO                12 // SPI0 MISO Not Used (Registers are write only)
#define ADF5356_CLK                 13 // SPI0 CLK



// ============================================================
// Power Board 
// ============================================================

// Power EN pin definitions
#define ADM_POS_3v3                 8
#define ADM_NEG_3v3                 9



// ============================================================
// SP8T Switch Board
// ============================================================

// SP8T Pins
#define SP8T_PIN_LS                 2
#define SP8T_PIN_ENABLE             3
#define SP8T_PIN_V1                 4
#define SP8T_PIN_V2                 5
#define SP8T_PIN_V3                 6 

// SP8T Config & Bit masks
#define SP8T_NUM_PORTS              8
#define SP8T_V1_MSK                 0x1 // 0x1 = 0b 0001
#define SP8T_V2_MSK                 0x2 // 0x2 = 0b 0010
#define SP8T_V3_MSK                 0x4 // 0x4 = 0b 0100

// Data type
typedef enum {
    UNDEF_PORT  = -1,
    SP8T_PORT_1 = 1,
    SP8T_PORT_2,
    SP8T_PORT_3,
    SP8T_PORT_4,
    SP8T_PORT_5,
    SP8T_PORT_6,
    SP8T_PORT_7,
    SP8T_PORT_8,
} sp8t_port_t;


// ============================================================
// SD Card
// ============================================================

// The config file and sweep data will be stored at these paths on the SD card
static const char* CONFIG_PATH = "/config.json";
static const char* CAL_PATH = "/cal.json";
static const char* DATA_PATH = "/data";

// Max length of file path is 256 bytes = 256 characters
#define MAX_FILE_PATH_LENGTH  256
#define CSV_COL_NUM           12 // This shouldn't be changed unless adding more data to the .csv
#define HEADER_BYTES          59 // The header is 59 bytes
#define DATA_ROW_BYTES        107 // The data rows are 107 bytes

// ============================================================
// Status/Error handling
// ============================================================

// Error handling
typedef enum {
    STATUS_OK                       = 0x00,

    // SD Errors: 0x10 - 0x1F
    STATUS_ERR_SD_INIT_FAIL         = 0x10,
    STATUS_ERR_SD_OPEN_FAIL         = 0X11,
    STATUS_ERR_SD_READ_FAIL         = 0X12,
    STATUS_ERR_SD_WRITE_FAIL        = 0X13,
    STATUS_ERR_SD_DIR_CREATE_FAIL   = 0X14,
    STATUS_ERR_SD_REMOVE_FAIL       = 0X15,
    STATUS_ERR_SD_PATH_TOO_LONG     = 0x16,

    // ADF5356 Errors: 0x20 - 0x2F
    STATUS_ERR_ADF_FREQUENCY_BOUND  = 0x20,

    STATUS_ERR_UNKNOWN              = 0XFF,
} status_t;

const char* status_to_str(status_t s)
{
    switch (s) {
        case STATUS_OK:                     return "OK";

        // SD Card
        case STATUS_ERR_SD_INIT_FAIL:       return "SD Initialization Failed";
        case STATUS_ERR_SD_OPEN_FAIL:       return "SD Data Open Failed";
        case STATUS_ERR_SD_READ_FAIL:       return "SD Data Read Failed";
        case STATUS_ERR_SD_WRITE_FAIL:      return "SD Data Write Failed";
        case STATUS_ERR_SD_DIR_CREATE_FAIL: return "SD Directory Creation Failed";
        case STATUS_ERR_SD_REMOVE_FAIL:     return "SD Data Removal Failed";
        case STATUS_ERR_SD_PATH_TOO_LONG:   return "SD File Path Too Long";

        // ADF5356
        case STATUS_ERR_ADF_FREQUENCY_BOUND:return "ADF Frequency out of bounds, must be within (800, 6800) MHz";

        case STATUS_ERR_UNKNOWN:            return "Unknown Error Code";

        default:                            return "Unknown Error Code";

    }
}



// ============================================================
// Global Parameters/Data structures
// ============================================================

// DEBUG config (dev) flags
#define ENABLE_DEBUG_PRINTS 0 // 0 DEFAULT | This needs to be 0 when using the CLI to communicate with the Teensy
#define ENABLE_CALIBRATION  1 // 1 DEFAULT | Determines if calibration data is accounted for when recording loss measurements
#define USE_QUAD_LOG_AMP    0 // 0 DEFAULT | Determines which regression approach is used to compute power output of log amp
#define RECORD_VOLTAGE      0 // 0 DEFAULT | Determines whether the raw voltage or power conversion is recorded to the SD card

// Sweep parameters
typedef struct {
    sp8t_port_t sp8t_out_ports[8];  // 1-8, inclusive
    uint32_t start_freq;            // MHz
    uint32_t stop_freq;             //  MHz
    uint32_t step_size;             // MHz
    uint32_t delay_ms;              // ms
} Config_t;


#endif // _COMMON_DEFS_H