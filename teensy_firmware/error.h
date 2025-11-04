#ifndef _ERROR_H
#define _ERROR_H

// Define possible errors and their codes
typedef enum {
    STATUS_OK                                   = 0x00;

    // SD Errors: 0x10 - 0x1F
    STATUS_ERR_SD_INIT_FAIL                     = 0x10;
    STATUS_ERR_SD_OPEN_FAIL                     = 0x11;
    STATUS_ERR_SD_READ_FAIL                     = 0x12;
    STATUS_ERR_SD_WRITE_FAIL                    = 0x13;
    STATUS_ERR_SD_DIR_CREATE_FAIL               = 0x14;
    STATUS_ERR_SD_REMOVE_FAIL                   = 0x15;

    // JSON Errors: 0x20 - 0x2F
    STATUS_ERR_JSON_DESERIALIZE_FAIL            = 0x20;

    STATUS_ERR_UNKNOWN                          = 0xFF;


} status_t;

// Given a status code, return a descriptive string for logging
const char* status_to_str(status_t s)
{
    switch (s) {
        case STATUS_OK:                             return "OK";
        case STATUS_ERR_SD_INIT_FAILED:             return "SD initialization failed";

        // etc
        default:                                    return "Unknown code";
    }
}

#endif // _ERROR_H