
#ifndef _COMMON_DEFS_H
#define _COMMON_DEFS_H

// ===========================
// ===== Pin Definitions =====

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

// SP8T
#define PIN_ENABLE                  2
#define PIN_LS                      3
#define PIN_V3                      4
#define PIN_V2                      5
#define PIN_V1                      6 

// SD card
#define SD_CS                       BUILTIN_SDCARD  // For Teensy 4.1 SDIO slot

// ADF5356
#define ADF_LE                      10 // CS pin (repurposed)
#define ADF_DATA                    11 // MOSI for SPI
#define ADF_MISO                    12 // Not Used (Grounded)
#define ADF_CLK                     13 

// ===========================

// LogAmp Config
#define LOG_AMP_READ_DELAY          10 // ms
#define ADC_REF_VOLTAGE             3.3  // Teensy uses a 3.3 V reference for analog inputs
#define ADC_MAX_VALUE               1023 // default 10-bit resolution

// SP8T Bit Masks
#define v1_msk                      0x1 // 0x1 = 0b 0001
#define v2_msk                      0x2 // 0x2 = 0b 0010
#define v3_msk                      0x4 // 0x4 = 0b 0100

// ADF5356 Register Control Bit Values
#define REGISTER_0_CTRL             0x00
#define REGISTER_1_CTRL             0x01
#define REGISTER_2_CTRL             0x02
#define REGISTER_3_CTRL             0x03
#define REGISTER_4_CTRL             0x04
#define REGISTER_5_CTRL             0x05
#define REGISTER_6_CTRL             0x06
#define REGISTER_7_CTRL             0x07
#define REGISTER_8_CTRL             0x08
#define REGISTER_9_CTRL             0x09
#define REGISTER_10_CTRL            0x0A
#define REGISTER_11_CTRL            0x0B
#define REGISTER_12_CTRL            0x0C
#define REGISTER_13_CTRL            0x0D
#define REGISTER_CTRL_MSK           0x0F // Last four bits

// Data Structures
typedef struct {
    uint8_t sp8t_out_port;
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