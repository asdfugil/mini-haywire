#ifndef SMP_H
#define SMP_H

#include "utils.h"

static inline int smp_id(void)
{
    return read_tpidrprw();
}

void smp_start_secondaries(void);
void smp_stop_secondaries(void);

#endif
