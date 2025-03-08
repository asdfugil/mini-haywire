#ifndef TIMER_H
#define TIMER_H

#include "types.h"

int timer_init(void);
uint64_t get_ticks(void);
uint32_t get_hz(void);

#endif
