/* SPDX-License-Identifier: MIT */

#include "firmware.h"
#include "adt.h"
#include "stdint.h"
#include "string.h"
#include "types.h"
#include "utils.h"

struct fw_version_info os_firmware;
struct fw_version_info system_firmware;

#define bail(...)                                                                                  \
    do {                                                                                           \
        printf(__VA_ARGS__);                                                                       \
        return -1;                                                                                 \
    } while (0)

const struct fw_version_info fw_versions[NUM_FW_VERSIONS] = {
    // clang-format off
    [V_UNKNOWN] = {V_UNKNOWN, "unknown",    {0},            1, "unknown"},
    // clang-format on
};

void firmware_parse_version(const char *s, u32 *out)
{
    memset(out, 0, sizeof(*out) * IBOOT_VER_COMP);

    for (int i = 0; i < IBOOT_VER_COMP; i++) {
        while (*s && !(*s >= '0' && *s <= '9'))
            s++;
        if (!*s)
            break;
        out[i] = atol(s);
        while (*s >= '0' && *s <= '9')
            s++;
    }
}

static void detect_firmware(struct fw_version_info *info, const char *ver)
{
    for (size_t i = 0; i < ARRAY_SIZE(fw_versions); i++) {
        if (!strcmp(fw_versions[i].iboot, ver)) {
            *info = fw_versions[i];
            return;
        }
    }

    *info = fw_versions[V_UNKNOWN];
    info->iboot = ver;
}

bool firmware_iboot_in_range(u32 min[IBOOT_VER_COMP], u32 max[IBOOT_VER_COMP],
                             u32 this[IBOOT_VER_COMP])
{
    int i;
    for (i = 0; i < IBOOT_VER_COMP; i++)
        if (this[i] != min[i])
            break;

    if (this[i] < min[i])
        return false;

    for (i = 0; i < IBOOT_VER_COMP; i++)
        if (this[i] != max[i])
            break;

    return this[i] < max[i];
}

int firmware_init(void)
{
    int node = adt_path_offset(adt, "/chosen");

    if (node < 0) {
        printf("ADT: no /chosen found\n");
        return -1;
    }

    u32 len;
    const char *p = adt_getprop(adt, node, "firmware-version", &len);
    if (p && len && p[len - 1] == 0) {
        detect_firmware(&os_firmware, p);
        // for purposes of haywire these two are the same
        detect_firmware(&system_firmware, p);
        printf("OS FW version: %s (%s)\n", os_firmware.string, os_firmware.iboot);
    } else {
        printf("ADT: failed to find firmware-version\n");
        return -1;
    }

    return 0;
}
