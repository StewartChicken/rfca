
#ifndef _COMMON_DEFS_H
#define _COMMON_DEFS_H

// ===========================
// ===== Pin Definitions =====

// ADF5356
#define ADF_LE                      10 // CS pin (repurposed)
#define ADF_DATA                    11 // MOSI for SPI
#define ADF_MISO                    12 // Not Used (Registers are write only)
#define ADF_CLK                     13 

// SD card
#define SD_CS                       BUILTIN_SDCARD  // For Teensy 4.1 SDIO slot

// Log amp
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

// SP8T
#define SP8T_PIN_ENABLE             2
#define SP8T_PIN_LS                 3
#define SP8T_PIN_V3                 4
#define SP8T_PIN_V2                 5
#define SP8T_PIN_V1                 6 

// === End Pin Definitions ===
// ===========================

// LogAmp Config
#define NUM_LOG_AMPS                10
#define LOG_AMP_READ_DELAY          10 // ms
#define ADC_REF_VOLTAGE             3.3  // Teensy uses a 3.3 V reference for analog inputs
#define ADC_MAX_VALUE               1023 // default 10-bit resolution

// SP8T Bit Masks
#define SP8T_NUM_PORTS              8
#define SP8T_V1_MSK                 0x1 // 0x1 = 0b 0001
#define SP8T_V2_MSK                 0x2 // 0x2 = 0b 0010
#define SP8T_V3_MSK                 0x4 // 0x4 = 0b 0100


// DEBUG (dev)
#define ENABLE_DEBUG_PRINTS 0 // This needs to be 0 when using the CLI to communicate with the Teensy

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

// Data Structures
typedef struct {
    sp8t_port_t sp8t_out_ports[8];  // 1-8, inclusive
    uint32_t start_freq;        // MHz
    uint32_t stop_freq;         //  MHz
    uint32_t step_size;         // MHz
    uint32_t delay_ms;          // ms
} Config_t;

// TODO: make this (maybe just make it a 2D array? uint32_t loss[8][10], 8 sp8t for 10 log amp, 80 total values)
typedef struct {

} Calibrate_t;


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

    STATUS_ERR_UNKNOWN              = 0XFF,
} status_t;

const char* status_to_str(status_t s)
{
    switch (s) {
        case STATUS_OK:                     return "OK";

        case STATUS_ERR_SD_INIT_FAIL:       return "SD Initialization Failed";
        case STATUS_ERR_SD_OPEN_FAIL:       return "SD Data Open Failed";
        case STATUS_ERR_SD_READ_FAIL:       return "SD Data Read Failed";
        case STATUS_ERR_SD_WRITE_FAIL:      return "SD Data Write Failed";
        case STATUS_ERR_SD_DIR_CREATE_FAIL: return "SD Directory Creation Failed";
        case STATUS_ERR_SD_REMOVE_FAIL:     return "SD Data Removal Failed";

        case STATUS_ERR_UNKNOWN:            return "Unknown Error Code";

        default:                            return "Unknown Error Code";

    }
}

#endif // _COMMON_DEFS_H