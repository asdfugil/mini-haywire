#include "adt.h"
#include "clkrstgen.h"
#include "timer.h"
#include "utils.h"

uintptr_t timer_base = 0;

#define TIMER_64_HI  0x80
#define TIMER_64_LO  0x84
#define TIMER_64_CTL 0x88

#define TIMER_64_CTL_EN BIT(3)
#define TIMER_FREQ      24000000

int timer_init(void)
{
    int timer_offset;
    int timer_path[8];

    int node = adt_path_offset(adt, "/arm-io");

    if (node < 0) {
        printf("timer: Error getting /arm-io node\n");
        return -1;
    }

    timer_offset = adt_path_offset_trace(adt, "/arm-io/timer", timer_path);
    if (timer_offset < 0) {
        printf("timer: Error getting /arm-io/timer node\n");
        return -1;
    }

    if (adt_get_reg(adt, timer_path, "reg", 0, &timer_base, NULL) < 0) {
        printf("timer: Error getting /arm-io/timer reg.\n");
        return -1;
    }

#if 0
    if (clkrstgen_adt_power_enable("/arm-io/timer") < 0) {
        printf("timer: Error enabling /arm-io/timer clock.\n");
        return -1;
    }
#endif

    printf("Timer registers @ 0x%x\n", timer_base);
    write32(timer_base + TIMER_64_CTL, TIMER_64_CTL_EN);

    return 0;
}

uint64_t get_ticks(void)
{
    // report because caller may get stuck
    if (!timer_base) {
        printf("timer: timer not setup\n");
        return 0;
    }

    return (u64)((u64)read32(timer_base + TIMER_64_HI) << 32 | read32(timer_base + TIMER_64_LO));
}

uint32_t get_hz(void)
{
    return TIMER_FREQ;
}
