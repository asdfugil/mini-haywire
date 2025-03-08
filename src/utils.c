#include <stdarg.h>

#include "utils.h"
#include "assert.h"
#include "iodev.h"
#include "smp.h"
#include "timer.h"
#include "uart.h"
#include "vsprintf.h"
#include "xnuboot.h"

void spin_init(spinlock_t *lock)
{
    lock->lock = -1;
    lock->count = 0;
}

void spin_lock(spinlock_t *lock)
{
    assert(lock->lock == -1 || lock->lock == smp_id());
    lock->lock = smp_id();
    lock->count++;
}

void spin_unlock(spinlock_t *lock)
{
    lock->count--;
    assert(lock->count >= 0);
}

int debug_printf(const char *fmt, ...)
{
    va_list args;
    char buffer[512];
    int i;

    va_start(args, fmt);
    i = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    iodev_console_write(buffer, min(i, (int)(sizeof(buffer) - 1)));

    return i;
}

void memset64(void *dst, u64 value, size_t size)
{
    size_t count = size / 8;
    volatile u64 *buf = (volatile u64 *)dst;

    for (size_t i = 0; i < count; i++)
        buf[i] = value;
}

void memcpy64(void *dst, void *src, size_t size)
{
    size_t count = size / 8;
    volatile u64 *buf = (volatile u64 *)dst;
    volatile u64 *source = (volatile u64 *)src;

    for (size_t i = 0; i < count; i++)
        buf[i] = source[i];
}

void memset32(void *dst, u32 value, size_t size)
{
    size_t count = size / 4;
    volatile u32 *buf = (volatile u32 *)dst;

    for (size_t i = 0; i < count; i++)
        buf[i] = value;
}

void memcpy32(void *dst, void *src, size_t size)
{
    size_t count = size / 4;
    volatile u32 *buf = (volatile u32 *)dst;
    volatile u32 *source = (volatile u32 *)src;

    for (size_t i = 0; i < count; i++)
        buf[i] = source[i];
}

void memset16(void *dst, u16 value, size_t size)
{
    size_t count = size / 2;
    volatile u16 *buf = (volatile u16 *)dst;

    for (size_t i = 0; i < count; i++)
        buf[i] = value;
}

void memcpy16(void *dst, void *src, size_t size)
{
    size_t count = size / 2;
    volatile u16 *buf = (volatile u16 *)dst;
    volatile u16 *source = (volatile u16 *)src;

    for (size_t i = 0; i < count; i++)
        buf[i] = source[i];
}

void memset8(void *dst, u8 value, size_t size)
{
    volatile u8 *buf = (volatile u8 *)dst;

    for (size_t i = 0; i < size; i++)
        buf[i] = value;
}

void memcpy8(void *dst, void *src, size_t size)
{
    volatile u16 *buf = (volatile u16 *)dst;
    volatile u16 *source = (volatile u16 *)src;

    for (size_t i = 0; i < size; i++)
        buf[i] = source[i];
}

void flush_and_reboot(void)
{
    iodev_console_flush();
    reboot();
}

void __assert_fail(const char *assertion, const char *file, unsigned int line, const char *function)
{
    printf("Assertion failed: '%s' on %s:%d:%s\n", assertion, file, line, function);
    flush_and_reboot();
}

void udelay(u32 d)
{
    u64 delay = ((u64)d) * get_hz() / 1000000;
    u64 val = get_ticks();
    while ((get_ticks() - val) < delay)
        ;
    sysop("isb");
}

u64 ticks_to_msecs(u64 ticks)
{
    // NOTE: only accurate if freq is even kHz
    return ticks / (get_hz() / 1000);
}

u64 ticks_to_usecs(u64 ticks)
{
    // NOTE: only accurate if freq is even MHz
    return ticks / (get_hz() / 1000000);
}

int snprintf(char *buffer, size_t size, const char *fmt, ...)
{
    va_list args;
    int i;

    va_start(args, fmt);
    i = vsnprintf(buffer, size, fmt, args);
    va_end(args);
    return i;
}

// TODO: update mapping?
uintptr_t top_of_memory_alloc(size_t size)
{
    static bool guard_page_inserted = false;
    cur_boot_args.mem_size -= ALIGN_UP(size, get_page_size());
    uintptr_t ret = cur_boot_args.phys_base + cur_boot_args.mem_size;

    if (!guard_page_inserted) {
        cur_boot_args.mem_size -= get_page_size();
        guard_page_inserted = true;
    } else {
        // If the guard page was already there, move it down and allocate
        // above it -- this is accomplished by simply shifting the allocated
        // region by one page up.
        ret += get_page_size();
    }

    return ret;
}
