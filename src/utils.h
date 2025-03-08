#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

#include "sysreg_access.h"
#include "types.h"

#define SPINLOCK_ALIGN 64
#define printf(...)    debug_printf(__VA_ARGS__)

typedef struct {
    s32 lock;
    int count;
} spinlock_t ALIGNED(SPINLOCK_ALIGN);

extern u32 board_id, chip_id;
extern size_t *_effective_size;
extern char _base[];
extern char _end[];
extern char _rodata_end[];

extern volatile enum exc_guard_t exc_guard;
extern volatile int exc_count;

void flush_and_reboot(void);

#define panic(fmt, ...)                                                                            \
    do {                                                                                           \
        debug_printf(fmt, ##__VA_ARGS__);                                                          \
        flush_and_reboot();                                                                        \
    } while (0)

typedef u32(generic_func)(u32, u32, u32, u32);

struct vector_args {
    generic_func *entry;
    u32 args[4];
    bool restore_logo;
};

extern struct vector_args next_stage;

#define PAGE_SIZE 4096

// 18.1.6 vs 18.1.1 diff
// clang-format off
#define SPINLOCK_INIT       {-1, 0}
// clang-format on
#define DECLARE_SPINLOCK(n) spinlock_t n = SPINLOCK_INIT;

#define ALIGN_UP(x, a)   (((x) + ((a) - 1)) & ~((a) - 1))
#define ALIGN_DOWN(x, a) ((x) & ~((a) - 1))
#define min(a, b)        (((a) < (b)) ? (a) : (b))
#define max(a, b)        (((a) > (b)) ? (a) : (b))

#define _concat(a, _1, b, ...) a##b

#define _sr_tkn_S(_0, _1, op0, op1, CRn, CRm, op2) s##op0##_##op1##_c##CRn##_c##CRm##_##op2

#define _sr_tkn(a) a

#define sr_tkn(...) _concat(_sr_tkn, __VA_ARGS__, )(__VA_ARGS__)

#define __mrs(reg)                                                                                 \
    ({                                                                                             \
        u32 val;                                                                                   \
        __asm__ volatile("mrs\t%0, " #reg : "=r"(val));                                            \
        val;                                                                                       \
    })
#define _mrs(reg) __mrs(reg)

#define __msr(reg, val)                                                                            \
    ({                                                                                             \
        u32 __val = (u32)val;                                                                      \
        __asm__ volatile("msr\t" #reg ", %0" : : "r"(__val));                                      \
    })
#define _msr(reg, val) __msr(reg, val)

#define mrs(reg)      _mrs(sr_tkn(reg))
#define msr(reg, val) _msr(sr_tkn(reg), val)

#define reg_clr(reg, bits)      _msr(sr_tkn(reg), _mrs(sr_tkn(reg)) & ~(bits))
#define reg_set(reg, bits)      _msr(sr_tkn(reg), _mrs(sr_tkn(reg)) | bits)
#define reg_mask(reg, clr, set) _msr(sr_tkn(reg), (_mrs(sr_tkn(reg)) & ~(clr)) | set)

#define ARRAY_SIZE(s) (sizeof(s) / sizeof((s)[0]))

#define sysop(op)        __asm__ volatile(op ::: "memory")
#define cacheop(op, val) ({ __asm__ volatile(op : : "r"(val) : "memory"); })

#define dma_mb()  sysop("dmb")
#define dma_rmb() sysop("dmb")
#define dma_wmb() sysop("dmb")

#define UNUSED(x) (void)(x)

void spin_init(spinlock_t *lock);
void spin_lock(spinlock_t *lock);
void spin_unlock(spinlock_t *lock);
int snprintf(char *buffer, size_t size, const char *fmt, ...);

void reboot(void) __attribute__((noreturn));
extern uintptr_t top_of_memory_alloc(size_t size);

int debug_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void udelay(u32 d);
u64 ticks_to_msecs(u64 ticks);
u64 ticks_to_usecs(u64 ticks);
#define mdelay(m) udelay((m) * 1000);

#define ic_ialluis() sysop("mcr p15, 0, r0, c7, c1, 0")       // icialluis
#define ic_iallu()   sysop("mcr p15, 0, r0, c7, c5, 0")       // iciallu
#define ic_ivau(p)   cacheop("mcr p15, 0, %0, c7, c5, 1", p)  // icimvau
#define dc_ivac(p)   cacheop("mcr p15, 0, %0, c7, c6, 1", p)  // dcimvac
#define dc_isw(p)    cacheop("mcr p15, 0, %0, c7, c6, 2", p)  // dcisw
#define dc_csw(p)    cacheop("mcr p15, 0, %0, c7, c10, 2", p) // dccsw
#define dc_cisw(p)   cacheop("mcr p15, 0, %0, c7, c14, 2", p) // dccisw
#define dc_cvac(p)   cacheop("mcr p15, 0, %0, c7, c10, 1", p) // dccmvac
#define dc_cvau(p)   cacheop("mcr p15, 0, %0, c7, c11, 1", p) // dccmvau

#define DCSW_OP_DCISW  0x0
#define DCSW_OP_DCCISW 0x1
#define DCSW_OP_DCCSW  0x2
void dcsw_op_all(u64 op_type);

#define dc_civac(p) cacheop("mcr p15, 0, %0, c7, c14, 1", p) // dccimvac

static inline uint64_t read64(uint32_t addr)
{
    return *(volatile uint64_t *)addr;
}

static inline void write64(uint32_t addr, uint64_t val)
{
    *(volatile uint64_t *)addr = val;
}

static inline uint32_t read32(uint32_t addr)
{
    return *(volatile uint32_t *)addr;
}

static inline void write32(uint32_t addr, uint32_t val)
{
    *(volatile uint32_t *)addr = val;
}

static inline uint16_t read16(uint32_t addr)
{
    return *(volatile uint16_t *)addr;
}

static inline void write16(uint32_t addr, uint16_t val)
{
    *(volatile uint16_t *)addr = val;
}

static inline uint16_t read8(uint32_t addr)
{
    return *(volatile uint8_t *)addr;
}

static inline void write8(uint32_t addr, uint8_t val)
{
    *(volatile uint8_t *)addr = val;
}

static inline u32 get_page_size(void)
{
    return PAGE_SIZE;
}

static inline u64 clear64(uint32_t addr, uint64_t val)
{
    u64 reg = read64(addr);
    u64 ret = reg & (~val);
    write64(addr, ret);
    return ret;
}

static inline u64 set64(uint32_t addr, uint64_t val)
{
    u64 reg = read64(addr);
    u64 ret = reg | val;
    write64(addr, ret);
    return ret;
}

static inline u32 clear32(uint32_t addr, uint32_t val)
{
    u32 reg = read32(addr);
    u32 ret = reg & (~val);
    write32(addr, ret);
    return ret;
}

static inline u32 set32(uint32_t addr, uint32_t val)
{
    u32 reg = read32(addr);
    u32 ret = reg | val;
    write32(addr, ret);
    return ret;
}

static inline u16 clear16(uint32_t addr, uint16_t val)
{
    u16 reg = read16(addr);
    u16 ret = reg & (~val);
    write16(addr, ret);
    return ret;
}

static inline u16 set16(uint32_t addr, uint16_t val)
{
    u16 reg = read16(addr);
    u16 ret = reg | val;
    write16(addr, ret);
    return ret;
}

static inline u8 clear8(uint32_t addr, uint8_t val)
{
    u8 reg = read8(addr);
    u8 ret = reg & (~val);
    write8(addr, ret);
    return ret;
}

static inline u8 set8(uint32_t addr, uint8_t val)
{
    u8 reg = read8(addr);
    u8 ret = reg | val;
    write8(addr, ret);
    return ret;
}

static inline u64 writeread64(u32 addr, u64 data)
{
    write64(addr, data);
    return read64(addr);
}

static inline u32 writeread32(u32 addr, u32 data)
{
    write32(addr, data);
    return read32(addr);
}

static inline u16 writeread16(u64 addr, u16 data)
{
    write16(addr, data);
    return read16(addr);
}

static inline u8 writeread8(u64 addr, u8 data)
{
    write8(addr, data);
    return read8(addr);
}

static inline u64 mask64(u32 addr, u64 clear, u64 set)
{
    u64 val = read64(addr);
    val &= (~clear);
    val |= set;
    write64(addr, val);
    return val;
}

static inline u32 mask32(u32 addr, u32 clear, u32 set)
{
    u32 val = read32(addr);
    val &= (~clear);
    val |= set;
    write32(addr, val);
    return val;
}

static inline u16 mask16(u32 addr, u16 clear, u16 set)
{
    u16 val = read16(addr);
    val &= (~clear);
    val |= set;
    write16(addr, val);
    return val;
}

static inline u8 mask8(u32 addr, u8 clear, u8 set)
{
    u8 val = read8(addr);
    val &= (~clear);
    val |= set;
    write8(addr, val);
    return val;
}

static inline int poll32(u64 addr, u32 mask, u32 target, u32 timeout)
{
    while (--timeout > 0) {
        u32 value = read32(addr) & mask;
        if (value == target)
            return 0;
        udelay(1);
    }

    return -1;
}

static inline int poll64(u64 addr, u64 mask, u64 target, u32 timeout)
{
    while (--timeout > 0) {
        u32 value = read64(addr) & mask;
        if (value == target)
            return 0;
        udelay(1);
    }

    return -1;
}

/*
 * These functions are guaranteed to copy by reading from src and writing to dst
 * in <n>-bit units If size is not aligned, the remaining bytes are not copied
 */
void memset64(void *dst, u64 value, size_t size);
void memcpy64(void *dst, void *src, size_t size);
void memset32(void *dst, u32 value, size_t size);
void memcpy32(void *dst, void *src, size_t size);
void memset16(void *dst, u16 value, size_t size);
void memcpy16(void *dst, void *src, size_t size);
void memset8(void *dst, u8 value, size_t size);
void memcpy8(void *dst, void *src, size_t size);

static inline bool is_boot_cpu(void)
{
    return true;
}

#endif