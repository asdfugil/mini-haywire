/* SPDX-License-Identifier: MIT */

#ifndef XNUBOOT_H
#define XNUBOOT_H

#include "types.h"

#define CMDLINE_LENGTH_RV1 256

struct boot_video {
    u32 base;
    u32 display;
    u32 stride;
    u32 width;
    u32 height;
    u32 depth;
} PACKED;

struct boot_args {
    u16 revision;
    u16 version;
    u32 virt_base;
    u32 phys_base;
    u32 mem_size;
    u32 top_of_kernel_data;
    struct boot_video video;
    u32 machine_type;
    void *devtree;
    u32 devtree_size;
    char cmdline[CMDLINE_LENGTH_RV1];
    u32 boot_flags;
} PACKED;

extern u32 boot_args_addr;
extern struct boot_args cur_boot_args;

#endif
