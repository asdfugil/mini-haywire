/* SPDX-License-Identifier: MIT */

#include <stdint.h>

#include "kboot.h"
#include "adt.h"
#include "assert.h"
#include "clkrstgen.h"
#include "exception.h"
#include "firmware.h"
#include "iodev.h"
#include "malloc.h"
#include "memory.h"
#include "types.h"
#include "usb.h"
#include "utils.h"
#include "xnuboot.h"

#include "libfdt/libfdt.h"

#define MAX_CHOSEN_PARAMS 16
#define MAX_MEM_REGIONS   8

void *dt = NULL;
static int dt_bufsize = 0;
static void *initrd_start = NULL;
static size_t initrd_size = 0;
static char *chosen_params[MAX_CHOSEN_PARAMS][2];

extern const char *const mini_version;

int dt_set_gpu(void *dt);

#define bail(...)                                                                                  \
    do {                                                                                           \
        printf(__VA_ARGS__);                                                                       \
        return -1;                                                                                 \
    } while (0)

#define bail_cleanup(...)                                                                          \
    do {                                                                                           \
        printf(__VA_ARGS__);                                                                       \
        ret = -1;                                                                                  \
        goto err;                                                                                  \
    } while (0)

void get_notchless_fb(u32 *fb_base, u32 *fb_height)
{
    *fb_base = cur_boot_args.video.base;
    *fb_height = cur_boot_args.video.height;

    return;
}

static int dt_set_rng_seed_adt(int node)
{
    int anode = adt_path_offset(adt, "/chosen");

    if (anode < 0)
        bail("ADT: /chosen not found\n");

    const uint8_t *random_seed;
    u32 seed_length;

    random_seed = adt_getprop(adt, anode, "random-seed", &seed_length);
    if (random_seed) {
        printf("ADT: %d bytes of random seed available\n", seed_length);

        if (seed_length >= sizeof(u64)) {
            u64 kaslr_seed;

            memcpy(&kaslr_seed, random_seed, sizeof(kaslr_seed));

            // Ideally we would throw away the kaslr_seed part of random_seed
            // and avoid reusing it. However, Linux wants 64 bytes of bootloader
            // random seed to consider its CRNG initialized, which is exactly
            // how much iBoot gives us. This probably doesn't matter, since
            // that entropy is going to get shuffled together and Linux makes
            // sure to clear the FDT randomness after using it anyway, but just
            // in case let's mix in a few bits from our own KASLR base to make
            // kaslr_seed unique.

            kaslr_seed ^= (uintptr_t)cur_boot_args.virt_base;

            if (fdt_setprop_u64(dt, node, "kaslr-seed", kaslr_seed))
                bail("FDT: couldn't set kaslr-seed\n");

            printf("FDT: KASLR seed initialized\n");
        } else {
            printf("ADT: not enough random data for kaslr-seed\n");
        }

        if (seed_length) {
            if (fdt_setprop(dt, node, "rng-seed", random_seed, seed_length))
                bail("FDT: couldn't set rng-seed\n");

            printf("FDT: Passing %d bytes of random seed\n", seed_length);
        }
    } else {
        printf("ADT: no random-seed available!\n");
    }

    return 0;
}

static int dt_set_fb(void)
{
    if (!cur_boot_args.video.base) {
        printf("FDT: Framebuffer unavailable, skipping framebuffer initialization");
        return 0;
    }

    int fb = fdt_path_offset(dt, "/chosen/framebuffer");

    if (fb < 0) {
        printf("FDT: No framebuffer found\n");
        return 0;
    }

    u32 fb_base, fb_height;
    get_notchless_fb(&fb_base, &fb_height);
    u32 fb_size = cur_boot_args.video.stride * fb_height;
    u32 fbreg[2] = {cpu_to_fdt32(fb_base), cpu_to_fdt32(fb_size)};
    char fbname[32];

    snprintf(fbname, sizeof(fbname), "framebuffer@%lx", fb_base);

    if (fdt_setprop(dt, fb, "reg", fbreg, sizeof(fbreg)))
        bail("FDT: couldn't set framebuffer.reg property\n");

    if (fdt_set_name(dt, fb, fbname))
        bail("FDT: couldn't set framebuffer name\n");

    if (fdt_setprop_u32(dt, fb, "width", cur_boot_args.video.width))
        bail("FDT: couldn't set framebuffer width\n");

    if (fdt_setprop_u32(dt, fb, "height", fb_height))
        bail("FDT: couldn't set framebuffer height\n");

    if (fdt_setprop_u32(dt, fb, "stride", cur_boot_args.video.stride))
        bail("FDT: couldn't set framebuffer stride\n");

    const char *format = NULL;

    switch (cur_boot_args.video.depth & 0xff) {
        case 32:
            format = "x8r8g8b8";
            break;
        case 30:
            format = "x2r10g10b10";
            break;
        case 16:
            format = "r5g6b5";
            break;
        default:
            printf("FDT: unsupported fb depth %u, not enabling\n", cur_boot_args.video.depth);
            return 0; // Do not error out, but don't set the FB
    }

    if (fdt_setprop_string(dt, fb, "format", format))
        bail("FDT: couldn't set framebuffer format\n");

    fdt_delprop(dt, fb, "status"); // may fail if it does not exist

    printf("FDT: %s base 0x%x size 0x%x\n", fbname, fb_base, fb_size);

    // We do not need to reserve the framebuffer, as it will be excluded from the usable RAM
    // range already.

    // save notch height in the dcp node if present
    if (cur_boot_args.video.height - fb_height) {
        int dcp = fdt_path_offset(dt, "dcp");
        if (dcp >= 0)
            if (fdt_appendprop_u32(dt, dcp, "apple,notch-height",
                                   cur_boot_args.video.height - fb_height))
                printf("FDT: couldn't set apple,notch-height\n");
    }

    return 0;
}

static int dt_set_chosen(void)
{

    int node = fdt_path_offset(dt, "/chosen");
    if (node < 0)
        bail("FDT: /chosen node not found in devtree\n");

    for (int i = 0; i < MAX_CHOSEN_PARAMS; i++) {
        if (!chosen_params[i][0])
            break;

        const char *name = chosen_params[i][0];
        const char *value = chosen_params[i][1];
        if (fdt_setprop(dt, node, name, value, strlen(value) + 1) < 0)
            bail("FDT: couldn't set chosen.%s property\n", name);
        printf("FDT: %s = '%s'\n", name, value);
    }

    if (initrd_start && initrd_size) {
        if (fdt_setprop_u32(dt, node, "linux,initrd-start", (uintptr_t)initrd_start))
            bail("FDT: couldn't set chosen.linux,initrd-start property\n");

        u32 end = ((uintptr_t)initrd_start) + initrd_size;
        if (fdt_setprop_u32(dt, node, "linux,initrd-end", end))
            bail("FDT: couldn't set chosen.linux,initrd-end property\n");

        if (fdt_add_mem_rsv(dt, (uintptr_t)initrd_start, initrd_size))
            bail("FDT: couldn't add reservation for the initrd\n");

        printf("FDT: initrd at %p size 0x%zx\n", initrd_start, initrd_size);
    }

    if (dt_set_fb())
        return -1;

    node = fdt_path_offset(dt, "/chosen");
    if (node < 0)
        bail("FDT: /chosen node not found in devtree\n");

    if (fdt_setprop(dt, node, "asahi,iboot2-version", os_firmware.iboot,
                    strlen(os_firmware.iboot) + 1))
        bail("FDT: couldn't set asahi,iboot2-version\n");

    if (fdt_setprop(dt, node, "asahi,os-fw-version", os_firmware.string,
                    strlen(os_firmware.string) + 1))
        bail("FDT: couldn't set asahi,os-fw-version\n");

    if (fdt_setprop(dt, node, "asahi,m1n1-stage2-version", mini_version, strlen(mini_version) + 1))
        bail("FDT: couldn't set asahi,m1n1-stage2-version\n");

    return dt_set_rng_seed_adt(node);
}

static int dt_set_memory(void)
{
    int anode = adt_path_offset(adt, "/chosen");

    if (anode < 0)
        bail("ADT: /chosen not found\n");

    uintptr_t dram_base = 0x8000000, dram_size = 0x10000000;

    // Tell the kernel our usable memory range. We cannot declare all of DRAM, and just reserve the
    // bottom and top, because the kernel would still map it (and just not use it), which breaks
    // ioremap (e.g. simplefb).

    uintptr_t dram_min = cur_boot_args.phys_base;
    uintptr_t dram_max = cur_boot_args.phys_base + cur_boot_args.mem_size;

    printf("FDT: DRAM at 0x%x size 0x%x\n", dram_base, dram_size);
    printf("FDT: Usable memory is 0x%x..0x%x (0x%x)\n", dram_min, dram_max, dram_max - dram_min);

    struct {
        fdt32_t start;
        fdt32_t size;
    } memreg[MAX_MEM_REGIONS] = {{cpu_to_fdt32(dram_min), cpu_to_fdt32(dram_max - dram_min)}};
    int num_regions = 1;

    int resv_node = fdt_path_offset(dt, "/reserved-memory");
    if (resv_node < 0) {
        printf("FDT: '/reserved-memory' not found\n");
    } else {
        int node;

        /* Find all reserved memory nodes */
        fdt_for_each_subnode(node, dt, resv_node)
        {
            const char *name = fdt_get_name(dt, node, NULL);
            const char *status = fdt_getprop(dt, node, "status", NULL);
            if (status && !strcmp(status, "disabled"))
                continue;

            if (fdt_getprop(dt, node, "no-map", NULL))
                continue;

            const fdt32_t *reg = fdt_getprop(dt, node, "reg", NULL);
            if (!reg)
                continue;

            u32 resv_start = fdt32_to_cpu(reg[0]);
            u32 resv_len = fdt32_to_cpu(reg[1]);
            u32 resv_end = resv_start + resv_len;

            if (resv_start < dram_min) {
                if (resv_end > dram_min) {
                    bail("FDT: reserved-memory node %s intersects start of RAM (%x..%x "
                         "%x..%x)\n",
                         name, resv_start, resv_end, dram_min, dram_max);
                }
            } else if (resv_end > dram_max) {
                if (resv_start < dram_max) {
                    bail("FDT: reserved-memory node %s intersects end of RAM (%x..%x %x..%x)\n",
                         name, resv_start, resv_end, dram_min, dram_max);
                }
            } else {
                continue;
            }

            if ((resv_start | resv_len) & (get_page_size() - 1)) {
                bail("FDT: reserved-memory node %s not page-aligned, ignoring (%x..%x)\n", name,
                     resv_start, resv_end);
                continue;
            }

            printf("FDT: Adding reserved-memory node %s (%x..%x) to RAM map\n", name, resv_start,
                   resv_end);

            if (num_regions >= MAX_MEM_REGIONS) {
                bail("FDT: Out of memory regions for reserved-memory\n");
            }

            memreg[num_regions].start = cpu_to_fdt32(resv_start);
            memreg[num_regions++].size = cpu_to_fdt32(resv_len);
        }
    }

    if (initrd_start && initrd_size) {
        if (num_regions >= MAX_MEM_REGIONS)
            bail("FDT: Out of memory regions for initrd\n");

        size_t aligned_initrd_size = ALIGN_UP(initrd_size, PAGE_SIZE);

        if ((uintptr_t)initrd_start & (PAGE_SIZE - 1))
            bail("FDT: initrd %p...0x%x is not page aligned\n", initrd_start,
                 (uintptr_t)initrd_start + initrd_size);

        printf("FDT: Adding initrd %p...0x%x to RAM map\n", initrd_start,
               (uintptr_t)initrd_start + aligned_initrd_size);

        memreg[num_regions].start = cpu_to_fdt32((uintptr_t)initrd_start);
        memreg[num_regions++].size = cpu_to_fdt32(aligned_initrd_size);
    }

    if (num_regions >= MAX_MEM_REGIONS)
        bail("FDT: Out of memory regions for FDT\n");

    if (((uintptr_t)dt | dt_bufsize) & (PAGE_SIZE - 1))
        bail("FDT: FDT %p...0x%x is not page aligned\n", dt, (uintptr_t)initrd_start + initrd_size);

    printf("FDT: Adding FDT %p...0x%x to RAM map\n", dt, (uintptr_t)dt + dt_bufsize);

    memreg[num_regions].start = cpu_to_fdt32((uintptr_t)initrd_start);
    memreg[num_regions++].size = cpu_to_fdt32(initrd_size);

    int node = fdt_path_offset(dt, "/memory");
    if (node < 0)
        bail("FDT: /memory node not found in devtree\n");

    if (fdt_setprop(dt, node, "reg", memreg, sizeof(memreg[0]) * num_regions))
        bail("FDT: couldn't set memory.reg property\n");

    return 0;
}

static int dt_set_serial_number(void)
{

    int fdt_root = fdt_path_offset(dt, "/");
    int adt_root = adt_path_offset(adt, "/");

    if (fdt_root < 0)
        bail("FDT: could not open a handle to FDT root.\n");
    if (adt_root < 0)
        bail("ADT: could not open a handle to ADT root.\n");

    u32 sn_len;
    const char *serial_number = adt_getprop(adt, adt_root, "serial-number", &sn_len);
    if (fdt_setprop_string(dt, fdt_root, "serial-number", serial_number))
        bail("FDT: unable to set device serial number!\n");
    printf("FDT: reporting device serial number: %s\n", serial_number);

    return 0;
}

static void dt_set_uboot_dm_preloc(int node)
{
    // Tell U-Boot to bind this node early
    fdt_setprop_empty(dt, node, "u-boot,dm-pre-reloc");
    fdt_setprop_empty(dt, node, "bootph-all");

    // Make sure the power domains are bound early as well
    int pds_size;
    const fdt32_t *pds = fdt_getprop(dt, node, "power-domains", &pds_size);
    if (!pds)
        return;

    fdt32_t *phandles = calloc(pds_size, 1);
    if (!phandles) {
        printf("FDT: out of memory\n");
        return;
    }
    memcpy(phandles, pds, pds_size);

    for (int i = 0; i < pds_size / 4; i++) {
        node = fdt_node_offset_by_phandle(dt, fdt32_ld(&phandles[i]));
        if (node < 0)
            continue;
        dt_set_uboot_dm_preloc(node);

        // restore node offset after DT update
        node = fdt_node_offset_by_phandle(dt, fdt32_ld(&phandles[i]));
        if (node < 0)
            continue;

        // And make sure the PMGR node is bound early too
        node = fdt_parent_offset(dt, node);
        if (node < 0)
            continue;
        dt_set_uboot_dm_preloc(node);
    }

    free(phandles);
}

static int dt_set_uboot(void)
{
    // Make sure that U-Boot can initialize the serial port in its
    // pre-relocation phase by marking its node and the nodes of the
    // power domains it depends on with a "u-boot,dm-pre-reloc"
    // property.

    const char *path = fdt_get_alias(dt, "serial2");
    if (path == NULL)
        return 0;

    int node = fdt_path_offset(dt, path);
    if (node < 0)
        return 0;

    dt_set_uboot_dm_preloc(node);
    return 0;
}

static int dt_get_or_add_reserved_mem(const char *node_name, const char *compat, bool nomap,
                                      u64 paddr, size_t size)
{
    int ret;
    int resv_node = fdt_path_offset(dt, "/reserved-memory");
    if (resv_node < 0)
        bail("FDT: '/reserved-memory' not found\n");

    int node = fdt_subnode_offset(dt, resv_node, node_name);
    if (node < 0) {
        node = fdt_add_subnode(dt, resv_node, node_name);
        if (node < 0)
            bail("FDT: failed to add node '%s' to  '/reserved-memory'\n", node_name);

        uint32_t phandle;
        ret = fdt_generate_phandle(dt, &phandle);
        if (ret)
            bail("FDT: failed to generate phandle: %d\n", ret);

        ret = fdt_setprop_u32(dt, node, "phandle", phandle);
        if (ret != 0)
            bail("FDT: couldn't set '%s.phandle' property: %d\n", node_name, ret);
    }

    u32 reg[2] = {cpu_to_fdt32(paddr), cpu_to_fdt32(size)};
    ret = fdt_setprop(dt, node, "reg", reg, sizeof(reg));
    if (ret != 0)
        bail("FDT: couldn't set '%s.reg' property: %d\n", node_name, ret);

    ret = fdt_setprop_string(dt, node, "compatible", compat);
    if (ret != 0)
        bail("FDT: couldn't set '%s.compatible' property: %d\n", node_name, ret);

    if (nomap) {
        ret = fdt_setprop_empty(dt, node, "no-map");
        if (ret != 0)
            bail("FDT: couldn't set '%s.no-map' property: %d\n", node_name, ret);
    }

    return node;
}

void kboot_set_initrd(void *start, size_t size)
{
    // top of memory again because of zImage decomperssion issues
    void *initrd = (void *)top_of_memory_alloc(size);
    memcpy(initrd, start, size);
    initrd_start = initrd;
    initrd_size = size;
    ;
}

int kboot_set_chosen(const char *name, const char *value)
{
    int i = 0;

    if (!name)
        return -1;

    for (i = 0; i < MAX_CHOSEN_PARAMS; i++) {
        if (!chosen_params[i][0]) {
            chosen_params[i][0] = calloc(strlen(name) + 1, 1);
            strcpy(chosen_params[i][0], name);
            break;
        }

        if (!strcmp(name, chosen_params[i][0])) {
            free(chosen_params[i][1]);
            chosen_params[i][1] = NULL;
            break;
        }
    }

    if (i >= MAX_CHOSEN_PARAMS)
        return -1;

    if (value) {
        chosen_params[i][1] = calloc(strlen(value) + 1, 1);
        strcpy(chosen_params[i][1], value);
    }

    return i;
}

#define LOGBUF_SIZE SZ_16K

struct {
    void *buffer;
    size_t wp;
} logbuf;

static bool log_console_iodev_can_write(void *opaque)
{
    UNUSED(opaque);
    return !!logbuf.buffer;
}

static ssize_t log_console_iodev_write(void *opaque, const void *buf, size_t len)
{
    UNUSED(opaque);

    if (!logbuf.buffer)
        return 0;

    ssize_t wrote = 0;
    size_t remain = LOGBUF_SIZE - logbuf.wp;
    while (remain < len) {
        memcpy(logbuf.buffer + logbuf.wp, buf, remain);
        logbuf.wp = 0;
        wrote += remain;
        buf += remain;
        len -= remain;
        remain = LOGBUF_SIZE;
    }
    memcpy(logbuf.buffer + logbuf.wp, buf, len);
    wrote += len;
    logbuf.wp = (logbuf.wp + len) % LOGBUF_SIZE;

    return wrote;
}

const struct iodev_ops iodev_log_ops = {
    .can_write = log_console_iodev_can_write,
    .write = log_console_iodev_write,
};

struct iodev iodev_log = {
    .ops = &iodev_log_ops,
    .usage = USAGE_CONSOLE,
    .lock = SPINLOCK_INIT,
};

static int dt_setup_mtd_phram(void)
{
    char node_name[64];
    snprintf(node_name, sizeof(node_name), "flash@%lx", (uintptr_t)adt);

    int node = dt_get_or_add_reserved_mem(node_name, "phram", false, (uintptr_t)adt,
                                          ALIGN_UP(cur_boot_args.devtree_size, SZ_16K));

    if (node > 0) {
        int ret = fdt_setprop_string(dt, node, "label", "adt");
        if (ret)
            bail("FDT: failed to setup ADT MTD phram label\n");
    }

    // init memory backed iodev for console log
    logbuf.buffer = (void *)top_of_memory_alloc(LOGBUF_SIZE);
    if (!logbuf.buffer)
        bail("FDT: failed to allocate m1n1 log buffer\n");

    snprintf(node_name, sizeof(node_name), "flash@%lx", (uintptr_t)logbuf.buffer);
    node = dt_get_or_add_reserved_mem(node_name, "phram", false, (uintptr_t)logbuf.buffer, SZ_16K);

    if (node > 0) {
        int ret = fdt_setprop_string(dt, node, "label", "m1n1_stage2.log");
        if (ret)
            bail("FDT: failed to setup m1n1 log MTD phram label\n");
    }

    return 0;
}

int kboot_prepare_dt(void *fdt)
{
    if (dt) {
        free(dt);
        dt = NULL;
    }

    dt_bufsize = fdt_totalsize(fdt);
    assert(dt_bufsize);

    dt_bufsize += 4 * SZ_16K; // Add 64K of buffer for modifications
    dt_bufsize = ALIGN_UP(dt_bufsize, PAGE_SIZE);
    dt = (void *)top_of_memory_alloc(dt_bufsize);

    if (fdt_open_into(fdt, dt, dt_bufsize) < 0)
        bail("FDT: fdt_open_into() failed\n");

    if (fdt_add_mem_rsv(dt, (u32)dt, dt_bufsize))
        bail("FDT: couldn't add reservation for the devtree\n");

    if (fdt_add_mem_rsv(dt, (u32)_base, ((u32)_end) - ((u32)_base)))
        bail("FDT: couldn't add reservation for m1n1\n");

    /* setup console log buffer early to capture as much log as possible */
    dt_setup_mtd_phram();

    if (dt_set_chosen())
        return -1;
    if (dt_set_serial_number())
        return -1;
    if (dt_set_uboot())
        return -1;

    /*
     * Set the /memory node late since we might be allocating from the top of memory
     * in one of the above devicetree prep functions, and we want an up-to-date value
     * for the usable memory span to make it into the devicetree.
     */
    if (dt_set_memory())
        return -1;

    if (fdt_pack(dt))
        bail("FDT: fdt_pack() failed\n");

    u32 dt_remain = dt_bufsize - fdt_totalsize(dt);
    if (dt_remain < SZ_16K)
        printf("FDT: free dt buffer space low, %u bytes left\n", dt_remain);

    printf("FDT prepared at %p\n", dt);

    return 0;
}

int kboot_boot(void *kernel)
{
    usb_init();

    printf("Preparing to boot kernel at %p with fdt at %p\n", kernel, dt);

    next_stage.entry = kernel;
    next_stage.args[0] = 0;
    next_stage.args[1] = ~0;
    next_stage.args[2] = (u32)dt;
    next_stage.args[3] = 0;
    next_stage.restore_logo = false;

    return 0;
}
