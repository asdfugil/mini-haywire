#ifndef CLKRSTGEN_H
#define CLKRSTGEN_H

#include "types.h"

/* All the unknowns appears to be some sort of register mask */
struct clkrstgen_device {
    u32 unk0;
    u32 unk1;
    u32 ps0;
    u32 ps1;
    u32 ps2;
    u32 ps3;
    u32 ps4;
    u32 unk2;
    u32 unk3;
    u32 unk4;
    const char name[19];
    u8 id;
} PACKED;

int clkrstgen_init(void);
int clkrstgen_power_on(const char *name);
int clkrstgen_power_off(const char *name);
int clkrstgen_adt_power_enable(const char *path);
int clkrstgen_adt_power_disable(const char *path);

#endif
