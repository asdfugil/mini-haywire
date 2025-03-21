/* SPDX-License-Identifier: MIT */

#ifndef __FIRMWARE_H__
#define __FIRMWARE_H__

#include "types.h"

/* macOS */
enum fw_version {
    V_UNKNOWN = 0,
    NUM_FW_VERSIONS,
};

#define FW_MIN V_UNKNOWN
#define FW_MAX V_UNKNOWN

#define OS_VER_COMP    4
#define IBOOT_VER_COMP 5

struct fw_version_info {
    enum fw_version version;
    const char *string;
    u32 num[OS_VER_COMP];
    size_t num_length;
    const char *iboot;
};

extern struct fw_version_info os_firmware;
extern struct fw_version_info system_firmware;
extern const struct fw_version_info fw_versions[NUM_FW_VERSIONS];

int firmware_init(void);
int firmware_set_fdt(void *fdt, int node, const char *prop, const struct fw_version_info *ver);
bool firmware_iboot_in_range(u32 min[IBOOT_VER_COMP], u32 max[IBOOT_VER_COMP],
                             u32 this[IBOOT_VER_COMP]);
bool firmware_sfw_in_range(enum fw_version lower_bound, enum fw_version upper_bound);
void firmware_parse_version(const char *s, u32 *out);

#endif
