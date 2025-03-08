/* SPDX-License-Identifier: MIT */

#ifndef KBOOT_H
#define KBOOT_H

#include "types.h"

#define DT_ALIGN 16384

void kboot_set_initrd(void *start, size_t size);
int kboot_set_chosen(const char *name, const char *value);
int kboot_prepare_dt(void *fdt);
int kboot_boot(void *kernel);

extern void *dt;

#endif
