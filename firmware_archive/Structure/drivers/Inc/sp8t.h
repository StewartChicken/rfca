#ifndef _SP8T_H
#define _SP8T_H

// Data
#include "../../common_defs.h"

#define PIN_ENABLE    2
#define PIN_LS        3
#define PIN_V3        4
#define PIN_V2        5
#define PIN_V1        6 

#define v1_msk 0x1 // 0x1 = 0b 0001
#define v2_msk 0x2 // 0x2 = 0b 0010
#define v3_msk 0x4 // 0x4 = 0b 0100

void sp8t_init(void);
void sp8t_enablePort(uint8_t);

#endif // _SP8T_8
