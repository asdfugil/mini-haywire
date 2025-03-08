#include "memory.h"
#include "adt.h"
#include "arm_cpu_regs.h"
#include "assert.h"
#include "string.h"
#include "types.h"
#include "utils.h"
#include "xnuboot.h"

#define CACHE_LINE_SIZE 64

#define CACHE_RANGE_OP(func, op)                                                                   \
    void func(void *addr, size_t length)                                                           \
    {                                                                                              \
        u64 p = (u64)addr;                                                                         \
        u64 end = p + length;                                                                      \
        while (p < end) {                                                                          \
            cacheop(op, p);                                                                        \
            p += CACHE_LINE_SIZE;                                                                  \
        }                                                                                          \
    }

CACHE_RANGE_OP(ic_ivau_range, "mcr p15, 0, %0, c7, c5, 1")
CACHE_RANGE_OP(dc_ivac_range, "mcr p15, 0, %0, c7, c6, 1")
CACHE_RANGE_OP(dc_cvac_range, "mcr p15, 0, %0, c7, c10, 1")
CACHE_RANGE_OP(dc_cvau_range, "mcr p15, 0, %0, c7, c11, 1")
CACHE_RANGE_OP(dc_civac_range, "mcr p15, 0, %0, c7, c14, 1")

static u32 mmu_add_ap(u32 section, u32 perms)
{
    if (perms & BIT(0))
        section |= BIT(10);
    else
        section &= ~BIT(10);

    if (perms & BIT(1))
        section |= BIT(11);
    else
        section &= ~BIT(11);

    if (perms & BIT(2))
        section |= BIT(15);
    else
        section &= ~BIT(15);

    return section;
}

void mmu_add_mapping(u32 from, u32 to, size_t size, u32 perms, u32 attr)
{
    assert(to == ALIGN_UP(to, SZ_1M));
    assert(from == ALIGN_UP(from, SZ_1M));
    assert(size == ALIGN_UP(size, SZ_1M));

    attr &= ~3;
    attr = mmu_add_ap(attr, perms);
    attr |= ATTR_SECTION | ATTR_NS;
    attr |= FIELD_PREP(ATTR_DOMAIN, 0);

    u32 index = size >> 20;
    u32 from_off = from >> 20;
    u32 to_off = to >> 20;
    while (index--)
        __pgtables[from_off++] = ((to_off++) << 20) | attr;
}

void mmu_init(void)
{
    int node = adt_path_offset(adt, "/arm-io");
    if (node < 0) {
        printf("MMU: ARM-IO node not found!\n");
        return;
    }
    u32 ranges_len;
    const u32 *ranges = adt_getprop(adt, node, "ranges", &ranges_len);
    if (!ranges) {
        printf("MMU: Failed to get ranges property!\n");
        return;
    }

    memset(__pgtables, '\0', sizeof(__pgtables));

    int range_cnt = ranges_len / 12;
    while (range_cnt--) {
        u32 bus = ranges[1];
        u32 size = ranges[2];

        mmu_add_mapping(bus, bus, size, AP_RW_ALL, ATTR_BUFFERABLE);
        ranges += 3;
    }

    mmu_add_mapping(cur_boot_args.phys_base, cur_boot_args.phys_base,
                    ALIGN_UP(cur_boot_args.mem_size, BIT(24)), AP_RW_ALL,
                    ATTR_BUFFERABLE | ATTR_CACHEABLE | ATTR_SHARABLE);
    write_dacr(0x55555555); // enable client access for all domains
    write_ttbr0((u32)__pgtables);
    write_ttbcr(0);

    // ensure page tables are flushed
    dc_cvac_range(__pgtables, sizeof(__pgtables));

    u32 sctlr = read_sctlr();
    u32 sctlr2 = sctlr & ~(SCTLR_A);
    sctlr2 |= (SCTLR_Z | SCTLR_M | SCTLR_I /*| SCTLR_C*/);
    printf("MMU: SCTLR %x -> %x\n", sctlr, sctlr2);
    write_sctlr(sctlr2);
    printf("MMU: Running with caches and translation enabled\n");
}

void mmu_shutdown(void)
{
    u32 sctlr = read_sctlr();
    u32 sctlr2 = sctlr & ~(SCTLR_A | SCTLR_I | SCTLR_C | SCTLR_M);

    dcsw_op_all(DCSW_OP_DCCISW);
    write_sctlr(sctlr2);

    printf("MMU: Shutdown\n");
}

u32 mmu_disable(void)
{
    u32 sctlr_old = read_sctlr();
    if (!(sctlr_old & SCTLR_M))
        return sctlr_old;

    dcsw_op_all(DCSW_OP_DCCISW);
    write_sctlr(sctlr_old & ~(SCTLR_I | SCTLR_C | SCTLR_M));

    return sctlr_old;
}

void mmu_restore(u32 state)
{
    write_sctlr(state);
}
