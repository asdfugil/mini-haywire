/* SPDX-License-Identifier: MIT */

#include "../build/build_cfg.h"
#include "../build/build_tag.h"

#include <stdarg.h>
#include <stdint.h>

#include "adt.h"
#include "clkrstgen.h"
#include "exception.h"
#include "firmware.h"
#include "heapblock.h"
#include "memory.h"
#include "payload.h"
#include "string.h"
#include "timer.h"
#include "types.h"
#include "uart.h"
#include "uartproxy.h"
#include "usb.h"
#include "utils.h"
#include "vsprintf.h"
#include "wdt.h"
#include "xnuboot.h"

const char version_tag[] = "##mini_ver##" BUILD_TAG;
const char *const mini_version = version_tag + 12;

u32 board_id = ~0, chip_id = ~0;
struct vector_args next_stage;

void get_device_info(void)
{
    const char *model = (const char *)adt_getprop(adt, 0, "model", NULL);
    const char *target = (const char *)adt_getprop(adt, 0, "target-type", NULL);

    printf("Device info:\n");

    if (model)
        printf("  Model: %s\n", model);

    if (target)
        printf("  Target: %s\n", target);

    int chosen = adt_path_offset(adt, "/chosen");
    if (chosen > 0) {
        if (ADT_GETPROP(adt, chosen, "board-id", &board_id) < 0)
            printf("Failed to find board-id\n");
        if (ADT_GETPROP(adt, chosen, "chip-id", &chip_id) < 0)
            printf("Failed to find chip-id\n");

        size_t serial_len;
        const char *serial = adt_getprop(adt, 0, "serial-number", &serial_len);
        if (serial && serial[0] == '\0' && serial_len == 32) {
            size_t ecid_len;
            const u32 *ecid;
            if ((ecid = adt_getprop(adt, chosen, "unique-chip-id", &ecid_len))) {
                if (ecid_len == 8) {
                    char new_serial[32];
                    snprintf(new_serial, 32, "%X%X", ecid[1], ecid[0]);
                    if (adt_setprop(adt, 0, "serial-number", new_serial, 32) < 0) {
                        printf("Failed fixing up serial number with ECID\n");
                    }
                }
            }
        }

        printf("  Board-ID: 0x%x\n", board_id);
        printf("  Chip-ID: 0x%x\n", chip_id);
    } else {
        printf("No chosen node!\n");
    }

    printf("\n");
}

void run_actions(void)
{
    int anode = adt_path_offset(adt, "/chosen/memory-map");
    if (anode < 0)
        goto run_proxy;

    printf("Checking for payloads...\n");

    u32 ramdisk[2];
    if (ADT_GETPROP_ARRAY(adt, anode, "RAMDisk", ramdisk) < 0)
        goto run_proxy;

    if (payload_run((void *)(ramdisk[0]), (void *)(ramdisk[0] + ramdisk[1])) == 0) {
        printf("Valid payload found\n");
        return;
    }

run_proxy:
    printf("No valid payload found\n");

    usb_init();
    usb_iodev_init();

    printf("Running proxy...\n");

    uartproxy_run(NULL);
}

void mini_main(void)
{
    printf("\n\nmini %s\n", mini_version);
    printf("Copyright Nick Chan, fenfenS and contributors\n");
    printf("Special thanks to the Asahi Linux Contributors\n");
    printf("Licensed under the MIT license\n\n");
    printf("Running in %s Mode\n\n", m_table[mrs(cpsr) & 0x1f]);

    firmware_init();
    heapblock_init();

    wdt_disable();
    mmu_init();
    clkrstgen_init();
    timer_init();

    printf("Initialization complete.\n");

    run_actions();

    if (!next_stage.entry)
        panic("Nothing to do!\n");

    printf("Preparing to run next stage at %p...\n", next_stage.entry);

    exception_shutdown();
    usb_iodev_shutdown();
    mmu_shutdown();

    printf("Vectoring to next stage...\n");

    next_stage.entry(next_stage.args[0], next_stage.args[1], next_stage.args[2],
                     next_stage.args[3]);

    panic("Next stage returned!\n");
}
