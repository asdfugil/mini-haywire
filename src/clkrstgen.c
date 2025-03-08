/* SPDX-License-Identifier: MIT */
#include "adt.h"
#include "clkrstgen.h"
#include "string.h"
#include "types.h"
#include "utils.h"

// device power state register
#define CLKRSTGEN_PS0 0x200
#define CLKRSTGEN_PS1 0x204
#define CLKRSTGEN_PS2 0x208
#define CLKRSTGEN_PS3 0x20c
#define CLKRSTGEN_PS4 0x210

#define CLKRSTGEN_DEVICE_ID GENMASK(7, 0)

static uint32_t clkrstgen_base;

static const struct clkrstgen_device *clkrstgen_devices = NULL;
static u32 clkrstgen_devices_len = 0;

static const u8 *clkrstgen_devices_4;
static u32 clkrstgen_devices_len_4 = 0;

static int clkrstgen_path[8];
static int clkrstgen_offset;

int clkrstgen_toggle(const struct clkrstgen_device *dev, bool enable)
{
    if (enable) {
        clear32(clkrstgen_base + CLKRSTGEN_PS0, dev->ps0);
        clear32(clkrstgen_base + CLKRSTGEN_PS1, dev->ps1);
        clear32(clkrstgen_base + CLKRSTGEN_PS2, dev->ps2);
        clear32(clkrstgen_base + CLKRSTGEN_PS3, dev->ps3);
        clear32(clkrstgen_base + CLKRSTGEN_PS4, dev->ps4);
    } else {
        set32(clkrstgen_base + CLKRSTGEN_PS0, dev->ps0);
        set32(clkrstgen_base + CLKRSTGEN_PS1, dev->ps1);
        set32(clkrstgen_base + CLKRSTGEN_PS2, dev->ps2);
        set32(clkrstgen_base + CLKRSTGEN_PS3, dev->ps3);
        set32(clkrstgen_base + CLKRSTGEN_PS4, dev->ps4);
    }

    return 0;
}

int clkrstgen_power_on(const char *name)
{
    const struct clkrstgen_device *dev = NULL;

    for (unsigned int i = 0; i < clkrstgen_devices_len; ++i) {
        if (strncmp(clkrstgen_devices[i].name, name, 0x10) == 0) {
            dev = &clkrstgen_devices[i];
            break;
        }
    }

    if (!dev)
        return -1;

    return clkrstgen_toggle(dev, true);
}

int clkrstgen_power_off(const char *name)
{
    const struct clkrstgen_device *dev = NULL;

    for (unsigned int i = 0; i < clkrstgen_devices_len; ++i) {
        if (strncmp(clkrstgen_devices[i].name, name, 0x10) == 0) {
            dev = &clkrstgen_devices[i];
            break;
        }
    }

    if (!dev)
        return -1;

    return clkrstgen_toggle(dev, false);
}

static int clkrstgen_find_device(u8 id, const struct clkrstgen_device **device)
{
    for (size_t i = 0; i < clkrstgen_devices_len; ++i) {
        const struct clkrstgen_device *i_device = &clkrstgen_devices[i];
        if (i_device->id != id)
            continue;

        *device = i_device;
        return 0;
    }

    return -1;
}

static int clkrstgen_adt_find_devices(const char *path, const u32 **devices, u32 *n_devices)
{
    int node_offset = adt_path_offset(adt, path);
    if (node_offset < 0) {
        printf("clkrstgen: Error getting node %s\n", path);
        return -1;
    }

    *devices = adt_getprop(adt, node_offset, "clock-gates", n_devices);
    if (*devices == NULL || *n_devices == 0) {
        printf("clkrstgen: Error getting %s clock-gates.\n", path);
        return -1;
    }

    *n_devices /= 4;

    return 0;
}

static int clkrstgen_adt_devices_set_mode(const char *path, bool target_mode)
{
    const u32 *devices;
    u32 n_devices;
    int ret = 0;

    if (clkrstgen_adt_find_devices(path, &devices, &n_devices) < 0)
        return -1;

    for (u32 i = 0; i < n_devices; ++i) {
        u8 device = FIELD_GET(CLKRSTGEN_DEVICE_ID, devices[i]);
        const struct clkrstgen_device *clkdev;

        if (device == 0)
            continue;

        if (clkrstgen_find_device(device, &clkdev) < 0) {
            ret = -1;
            continue;
        }

        if (clkrstgen_toggle(clkdev, target_mode)) {
            ret = -1;
            continue;
        }
    }

    return ret;
}

int clkrstgen_adt_power_enable(const char *path)
{
    return clkrstgen_adt_devices_set_mode(path, true);
}

int clkrstgen_adt_power_disable(const char *path)
{
    return clkrstgen_adt_devices_set_mode(path, false);
}

int clkrstgen_init(void)
{
    int node = adt_path_offset(adt, "/arm-io");
    if (node < 0) {
        printf("clkrstgen: Error getting /arm-io node\n");
        return -1;
    }

    clkrstgen_offset = adt_path_offset_trace(adt, "/arm-io/clkrstgen", clkrstgen_path);
    if (clkrstgen_offset < 0) {
        printf("clkrstgen: Error getting /arm-io/clkrstgen node\n");
        return -1;
    }

    clkrstgen_devices_4 =
        adt_getprop(adt, clkrstgen_offset, "device-clocks", &clkrstgen_devices_len_4);
    if (clkrstgen_devices_4 == NULL || clkrstgen_devices_len_4 < 4) {
        printf("clkrstgen: Error getting /arm-io/clkrstgen device-clocks.\n");
        return -1;
    }

    if (adt_get_reg(adt, clkrstgen_path, "reg", 0, &clkrstgen_base, NULL) < 0) {
        printf("clkrstgen: Error getting /arm-io/clkrstgen reg.\n");
        return -1;
    }

    clkrstgen_devices = (const struct clkrstgen_device *)(clkrstgen_devices_4 + 4);
    clkrstgen_devices_len = (clkrstgen_devices_len_4 - 4) / sizeof(const struct clkrstgen_device);

#if 1
    write32(clkrstgen_base + CLKRSTGEN_PS0, 0);
    write32(clkrstgen_base + CLKRSTGEN_PS1, 0);
    write32(clkrstgen_base + CLKRSTGEN_PS2, 0);
    write32(clkrstgen_base + CLKRSTGEN_PS3, 0);
    write32(clkrstgen_base + CLKRSTGEN_PS4, 0);
#endif

    printf("clkrstgen: Initialized, %d devices found.\n", clkrstgen_devices_len);

    return 0;
}
