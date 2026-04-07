
#ifndef _SP8T_H
#define _SP8T_H

// Data
#include "../../common_defs.h"

// Public API functions
void sp8t_init(void);
void sp8t_enablePort(const sp8t_port_t); // Port num (1-8)
void sp8t_reset(void);

#endif // _SP8T_8
