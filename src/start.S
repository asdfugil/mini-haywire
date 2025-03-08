#include "types.h"
#include "uart_regs.h"

.globl _start

.align 2

#define adr_l(reg, label) \
    movw		reg, #:lower16:label - (1f + 8); \
    movt		reg, #:upper16:label - (1f + 8); \
1:; \
    add		reg, reg, pc;

#define EARLY_UART_BASE 0x3DC00000

.globl apply_rela
.globl memset
.globl _base
.globl _rela_start
.globl _rela_end
.globl _bss_start
.globl _bss_end
.globl mini_start
.globl mini_stack
.globl debug_putc
.globl _vectors
.globl v_reset
.globl v_undf
.globl v_svc
.globl v_instr_abrt
.globl v_data_abrt
.globl v_reserved
.globl v_irq
.globl v_fiq

.type _vectors, %function
_vectors:
_start:
	ldr	pc, =v_reset
	ldr	pc, =v_undf
	ldr	pc, =v_svc
	ldr	pc, =v_instr_abrt
	ldr	pc, =v_data_abrt
	ldr	pc, =v_reserved
	ldr	pc, =v_irq
	ldr	pc, =v_fiq

.type debug_putc, %function
debug_putc:
#ifdef EARLY_UART_BASE
    ldr r1, =EARLY_UART_BASE

1:
    ldr r2, [r1, #UTRSTAT]
    tst r2, #2
    beq 1b
    str r0, [r1, #UTXH]
#endif
    bx lr

.type init_uart, %function
init_uart:
#ifdef EARLY_UART_BASE
    ldr r1, =EARLY_UART_BASE
    mov r0, #ULCON_CS8
    str r0, [r1, #ULCON]
    mov r0, #UCON_DEFAULT
    str r0, [r1, #UCON]
    mov r0, #0x0
    str r0, [r1, #UFCON]
    mov r0, #UMCOM_RTS_LOW
    str r0, [r1, #UMCON]
    mov r0, #0xc // 115200
    str r0, [r1, #UBRDIV]
#endif
    bx lr

v_reset:
    mov r4, r0
    bl init_uart
    mov r0, 'm'
    bl debug_putc

    // continue execution in real dram
    orr pc, pc, #0x8000000

    mrs r0, cpsr
    bic r0, r0, #0x1F

    orr r1, r0, #0x1B /* Undefined mode */
    msr cpsr_cxsf, r1
    adr_l(sp, _und_stack_bot)

    orr r1, r0, #0x17 /* Abort mode */
    msr cpsr_cxsf, r1
    adr_l(sp, _exc_stack_bot)

    orr r1, r0, #0x12 /* IRQ mode */
    msr cpsr_cxsf, r1
    adr_l(sp, _irq_stack_bot)

    orr r1, r0, #0x11 /* FIQ mode */
    msr cpsr_cxsf, r1
    adr_l(sp, _fiq_stack_bot)

    orr r1, r0, #0x13 /* Supervisor mode */
    msr cpsr_cxsf, r1
    adr_l(sp, _svc_stack_bot)

    adr_l(r0, _base)
    adr_l(r1, _rela_start)
    adr_l(r2, _rela_end)

    bl apply_rela

    mov r0, 'i'
    bl debug_putc

    adr_l(r0, _bss_start)
    adr_l(r2, _bss_end)
    sub r2, r2, r0
    mov r1, #0
    bl memset

    mov r0, 'n'
    bl debug_putc

    mov r0, 'i'
    bl debug_putc

    mov r0, r4
    adr_l(r1, _base)
    b mini_start

.globl reboot
.type debug_putc, %function
reboot:
    bl wdt_reboot
    b .
