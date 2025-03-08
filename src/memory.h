#ifndef MEMORY_H
#define MEMORY_H

#include "arm_cpu_regs.h"
#include "types.h"
#include "utils.h"

// simple mapping by only using 1MB pages
extern u32 __pgtables[4096];

#define ATTR_PXN        BIT(0)
#define ATTR_SECTION    BIT(1)
#define ATTR_BUFFERABLE BIT(2)
#define ATTR_CACHEABLE  BIT(3)
#define ATTR_XN         BIT(4)
#define ATTR_DOMAIN     GENMASK(8, 5)
#define ATTR_IMP        BIT(9)
#define ATTR_TEX        GENMASK(14, 12)
#define ATTR_SHARABLE   BIT(16)
#define ATTR_NG         BIT(17)
#define ATTR_NS         BIT(19)

#define AP_NONE          0
#define AP_RW_PL1        0b1
#define AP_RW_PL1_RO_PL0 0b10
#define AP_RW_ALL        0b11
#define AP_RO_PL1        0b101
#define AP_RO_PL1_PL0    0b110
#define AP_RO_PL1_PL0_V7 0b111

void mmu_init(void);
void mmu_shutdown(void);
void mmu_shutdown(void);
u32 mmu_disable(void);
void mmu_restore(u32 state);

void ic_ivau_range(void *addr, size_t length);
void dc_ivac_range(void *addr, size_t length);
void dc_cvac_range(void *addr, size_t length);
void dc_cvau_range(void *addr, size_t length);
void dc_civac_range(void *addr, size_t length);

static inline bool mmu_active(void)
{
    return !!(read_sctlr() & SCTLR_M);
}

#endif