#include "chickens.h"
#include "arm_cpu_regs.h"
#include "midr.h"
#include "uart.h"
#include "utils.h"

const char *init_cpu(void)
{
    const char *cpu = "Unknown";

    u32 midr = read_midr();

    int part = FIELD_GET(MIDR_PART, midr);
    int rev = (FIELD_GET(MIDR_REV_HIGH, midr) << 4) | FIELD_GET(MIDR_REV_LOW, midr);

    printf("  CPU part: 0x%x rev: 0x%x\n", part, rev);

    switch (part) {
        case MIDR_PART_CORTEX_A5:
            cpu = "ARM Cortex-A5";
            break;
        default:
            uart_puts("  Unknown CPU type");
            break;
    }

    return cpu;
}
