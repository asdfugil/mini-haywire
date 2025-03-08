#include "exception.h"
#include "arm_cpu_regs.h"
#include "memory.h"
#include "uart.h"
#include "utils.h"

extern u32 _vectors[];

volatile enum exc_guard_t exc_guard = GUARD_OFF;
volatile int exc_count = 0;

void exc_handler(u32 *regs, u32 spsr, int type)
{
    printf("Exception: %s\n", exc_table[type]);
    u32 pc;

    switch (type) {
        case EXC_TYPE_UNDEFINED:
        case EXC_TYPE_SVC:
        case EXC_TYPE_INSTRUCTION_ABORT:
        case EXC_TYPE_DATA_ABORT:
            pc = regs[15] - 4;
            break;
        case EXC_TYPE_IRQ:
        case EXC_TYPE_FIQ:
            pc = regs[15] - 8;
            break;
        default:
            pc = regs[15];
            break;
    }

    printf("Registers:\n");
    printf(" r0: 0x%08x r1: 0x%08x  r2: 0x%08x  r3: 0x%08x\n", regs[0], regs[1], regs[2], regs[3]);
    printf(" r4: 0x%08x r5: 0x%08x  r6: 0x%08x  r7: 0x%8x\n", regs[4], regs[5], regs[6], regs[7]);
    printf(" r8: 0x%08x r9: 0x%08x r10: 0x%08x r11: 0x%08x\n", regs[8], regs[9], regs[10],
           regs[11]);
    printf("r12: 0x%08x sp: 0x%08x  lr: 0x%08x  pc: 0x%08x\n", regs[12], regs[13], regs[14], pc);
    printf("spsr: %08x (%s)  pc off: 0x%x\n", spsr, m_table[spsr & 0x1f], pc - (u32)_base);

    switch (type) {
        case EXC_TYPE_UNDEFINED:
        case EXC_TYPE_INSTRUCTION_ABORT:
        case EXC_TYPE_DATA_ABORT:
            printf("Unhandled exception, rebooting...\n");
            flush_and_reboot();
            break;
        default:
            break;
    }
}

void exception_initialize(void)
{
    write_vbar((u32)_vectors);

    u32 sctlr = read_sctlr();
    u32 sctlr2 = sctlr & ~(SCTLR_V | SCTLR_VE);
    write_sctlr(sctlr2);

    reg_clr(cpsr, CPSR_A | CPSR_I | CPSR_F);
}

void exception_shutdown(void)
{
    reg_set(cpsr, CPSR_A | CPSR_I | CPSR_F);
}
