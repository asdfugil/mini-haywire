#include <stdarg.h>
#include <stdint.h>

#include "adt.h"
#include "chickens.h"
#include "exception.h"
#include "smp.h"
#include "string.h"
#include "types.h"
#include "uart.h"
#include "utils.h"
#include "vsprintf.h"
#include "xnuboot.h"

struct rela_entry {
    uint32_t off, info;
};

struct boot_args cur_boot_args;
uint32_t boot_args_addr;
struct rela_entry *_rela_start, *_rela_end;
void *adt;

extern char _bss_start[0];
extern char _bss_end[0];
extern void get_device_info(void);

#define R_ARM_RELATIVE 23

void debug_putc(char c);

void apply_rela(uint32_t base, struct rela_entry *rela_start, struct rela_entry *rela_end)
{
    struct rela_entry *e = rela_start;

    while (e < rela_end) {
        uint8_t type = e->info & 0xff;

        switch (type) {
            case R_ARM_RELATIVE:
                *(u32 *)(base + e->off) += base;
                break;
            default:
                debug_putc('R');
                debug_putc('!');
                while (1)
                    ;
        }
        e++;
    }
}

u64 boot_flags, mem_size_actual;
void dump_boot_args(struct boot_args *ba)
{
    printf("  revision:     %d\n", ba->revision);
    printf("  version:      %d\n", ba->version);
    printf("  virt_base:    0x%x\n", ba->virt_base);
    printf("  phys_base:    0x%x\n", ba->phys_base);
    printf("  mem_size:     0x%x\n", ba->mem_size);
    printf("  top_of_kdata: 0x%x\n", ba->top_of_kernel_data);
    printf("  video:\n");
    printf("    base:       0x%x\n", ba->video.base);
    printf("    display:    0x%x\n", ba->video.display);
    printf("    stride:     0x%x\n", ba->video.stride);
    printf("    width:      %u\n", ba->video.width);
    printf("    height:     %u\n", ba->video.height);
    printf("    depth:      %ubpp\n", ba->video.depth & 0xff);
    printf("    density:    %u\n", ba->video.depth >> 16);
    printf("  machine_type: %d\n", ba->machine_type);
    printf("  devtree:      %p\n", ba->devtree);
    printf("  devtree_size: 0x%x\n", ba->devtree_size);
    int node = adt_path_offset(adt, "/chosen");

    if (node < 0) {
        printf("ADT: no /chosen found\n");
        return;
    }

    printf("  cmdline:      %s\n", ba->cmdline);
    printf("  boot_flags:   0x%x\n", ba->boot_flags);
    boot_flags = ba->boot_flags;
}

void mini_main(void);
void mini_start(struct boot_args *boot_args, void *base)
{
    UNUSED(base);
    boot_args_addr = (u32)boot_args;

    memcpy(&cur_boot_args, boot_args, sizeof(cur_boot_args));

    // framebuffer is useless
    cur_boot_args.mem_size =
        (cur_boot_args.video.base + cur_boot_args.video.stride * cur_boot_args.video.height -
         cur_boot_args.phys_base);

    // move adt to top of memory because the zImage decompressor probably
    // will overwrite it
    void *adt_new = (void *)top_of_memory_alloc(cur_boot_args.devtree_size);
    memcpy(
        adt_new,
        (void *)(((u32)cur_boot_args.devtree) - cur_boot_args.virt_base + cur_boot_args.phys_base),
        cur_boot_args.devtree_size);

    adt = adt_new;

    if (uart_init())
        debug_putc('!');

    debug_putc('\n');

    uart_printf("Initializting\n");
    get_device_info();

    write_tpidrprw(0);

    printf("CPU init (MIDR: 0x%x smp_id:0x%x)...\n", read_midr(), smp_id());
    const char *type = init_cpu();
    printf("  CPU: %s\n\n", type);

    printf("boot_args at %p\n", boot_args);

    dump_boot_args(&cur_boot_args);

    exception_initialize();
    mini_main();

    return;
}
