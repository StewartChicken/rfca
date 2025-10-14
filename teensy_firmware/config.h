#ifndef _CONFIG_H
#define _CONFIG_H

#pragma once

// Change once these are known
#define CONFIG1_DEFAULT 1
#define CONFIG2_DEFAULT 2
#define CONFIG3_DEFAULT 3
#define CONFIG4_DEFAULT 4

typedef struct {
    uint32_t config1;
    uint32_t config2;
    uint32_t config3;
    uint32_t config4;
} Config_t;

#endif // _CONFIG_H