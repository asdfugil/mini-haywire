/* SPDX-License-Identifier: MIT */

#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

#define EXC_TYPE_UNDEFINED         0
#define EXC_TYPE_SVC               1
#define EXC_TYPE_INSTRUCTION_ABORT 2
#define EXC_TYPE_DATA_ABORT        3
#define EXC_TYPE_IRQ               4
#define EXC_TYPE_FIQ               5

#ifndef __ASSEMBLER__
void exception_initialize(void);
void exception_shutdown(void);

enum exc_guard_t {
    GUARD_OFF = 0,
    GUARD_SKIP,
    GUARD_MARK,
    GUARD_RETURN,
    GUARD_TYPE_MASK = 0xff,
    GUARD_SILENT = 0x100,
};

static char *exc_table[EXC_TYPE_FIQ + 1] = {
    [EXC_TYPE_UNDEFINED] = "Undefined",
    [EXC_TYPE_SVC] = "Supervisor Call",
    [EXC_TYPE_INSTRUCTION_ABORT] = "Instruction Abort",
    [EXC_TYPE_DATA_ABORT] = "Data Abort",
    [EXC_TYPE_IRQ] = "IRQ",
    [EXC_TYPE_FIQ] = "FIQ",
};

static char *m_table[0x20] = {
    [0x10] = "User",       //
    [0x11] = "FIQ",        //
    [0x12] = "IRQ",        //
    [0x13] = "Supervisor", //
    [0x16] = "Monitor",    //
    [0x17] = "Abort",      //
    [0x1a] = "Hyp",        //
    [0x1b] = "Undefined",  //
    [0x1f] = "System",     //
};
#endif

#endif