#ifndef SYSREG_ACCESS_H
#define SYSREG_ACCESS_H

#include "types.h"

static inline u32 read_tpidrprw(void)
{
    u32 reg;
    __asm__ volatile("mrc\tp15, 0, %0, c13, c0, 4" : "=r"(reg));
    return reg;
}

static inline void write_tpidrprw(u32 reg)
{
    __asm__ volatile("mcr\tp15, 0, %0, c13, c0, 4" ::"r"(reg));
}

static inline u32 read_midr(void)
{
    u32 reg;
    __asm__ volatile("mrc\tp15, 0, %0, c0, c0, 0" : "=r"(reg));
    return reg;
}

static inline u32 read_ttbr0(void)
{
    u32 reg;
    __asm__ volatile("mrc\tp15, 0, %0, c2, c0, 0" : "=r"(reg));
    return reg;
}

static inline u32 read_dacr(void)
{
    u32 reg;
    __asm__ volatile("mrc\tp15, 0, %0, c3, c0, 0" : "=r"(reg));
    return reg;
}

static inline void write_ttbr0(u32 reg)
{
    __asm__ volatile("mcr\tp15, 0, %0, c2, c0, 0" ::"r"(reg));
}

static inline void write_dacr(u32 reg)
{
    __asm__ volatile("mcr\tp15, 0, %0, c3, c0, 0" ::"r"(reg));
}

static inline void write_ttbcr(u32 reg)
{
    __asm__ volatile("mcr\tp15, 0, %0, c2, c0, 2" ::"r"(reg));
}

static inline u32 read_sctlr(void)
{
    u32 reg;
    __asm__ volatile("mrc\tp15, 0, %0, c1, c0, 0" : "=r"(reg));
    return reg;
}

static inline void write_sctlr(u32 reg)
{
    __asm__ volatile("mcr\tp15, 0, %0, c1, c0, 0" ::"r"(reg));
}

static inline u32 read_vbar(void)
{
    u32 reg;
    __asm__ volatile("mrc\tp15, 0, %0, c12, c0, 0" : "=r"(reg));
    return reg;
}

static inline void write_vbar(u32 reg)
{
    __asm__ volatile("mcr\tp15, 0, %0, c12, c0, 0" ::"r"(reg));
}

#endif
