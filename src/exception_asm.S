#include "exception.h"

.globl debug_putc
.globl v_reset
.globl v_undf
.globl v_svc
.globl v_instr_abrt
.globl v_data_abrt
.globl v_reserved
.globl v_irq
.globl v_fiq

.globl exc_handler

.align 2

.macro save_context
	stmfd sp!, {lr}
	stmfd sp, {r0-lr}^
	sub   sp, sp, #60
	mov r0, sp
    mrs   r1, spsr
    stmfd sp!, {r1}
.endm

.macro restore_context
	ldmfd sp!, {r0}
	msr spsr, r0
	ldmfd sp, {r0-lr}^
	add sp, sp, #60
	ldmfd sp!, {pc}^
.endm

v_undf:
	save_context
	mov r2, #EXC_TYPE_UNDEFINED
	bl exc_handler
	restore_context
v_svc:
	save_context
	mov r2, #EXC_TYPE_SVC
	bl exc_handler
	restore_context
v_instr_abrt:
	save_context
	mov r2, #EXC_TYPE_INSTRUCTION_ABORT
	bl exc_handler
	restore_context
v_data_abrt:
	save_context
	mov r2, #EXC_TYPE_DATA_ABORT
	bl exc_handler
	restore_context
v_reserved:
	b .
v_irq:
	save_context
	mov r2, #EXC_TYPE_IRQ
	bl exc_handler
	restore_context
v_fiq:
	save_context
	mov r2, #EXC_TYPE_FIQ
	bl exc_handler
	restore_context
