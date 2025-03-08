/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef ARM_CPU_REGS_H
#define ARM_CPU_REGS_H

#include "types.h"

#define SCTLR_EnIA    BIT(31)
#define SCTLR_EnIB    BIT(30)
#define SCTLR_LSMAOE  BIT(29)
#define SCTLR_nTLSMD  BIT(28)
#define SCTLR_EnDA    BIT(27)
#define SCTLR_UCI     BIT(26)
#define SCTLR_EE      BIT(25)
#define SCTLR_VE      BIT(24)
#define SCTLR_SPAN    BIT(23)
#define SCTLR_EIS     BIT(22)
#define SCTLR_IESB    BIT(21)
#define SCTLR_TSCXT   BIT(20)
#define SCTLR_WXN     BIT(19)
#define SCTLR_nTWE    BIT(18)
#define SCTLR_nTWI    BIT(16)
#define SCTLR_UCT     BIT(15)
#define SCTLR_DZE     BIT(14)
#define SCTLR_V       BIT(13)
#define SCTLR_I       BIT(12)
#define SCTLR_Z       BIT(11)
#define SCTLR_EnRCTX  BIT(10)
#define SCTLR_UMA     BIT(9)
#define SCTLR_SED     BIT(8)
#define SCTLR_ITD     BIT(7)
#define SCTLR_nAA     BIT(6)
#define SCTLR_CP15BEN BIT(5)
#define SCTLR_SA0     BIT(4)
#define SCTLR_SA      BIT(3)
#define SCTLR_C       BIT(2)
#define SCTLR_A       BIT(1)
#define SCTLR_M       BIT(0)

#define CPSR_A BIT(8)
#define CPSR_I BIT(7)
#define CPSR_F BIT(6)

/*
 * Copyright (c) 2016-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* ID_MMFR4 definitions */
#define ID_MMFR4_CNP_SHIFT  (12)
#define ID_MMFR4_CNP_LENGTH (4)
#define ID_MMFR4_CNP_MASK   (0xf)

#define ID_MMFR4_CCIDX_SHIFT  (24)
#define ID_MMFR4_CCIDX_LENGTH (4)
#define ID_MMFR4_CCIDX_MASK   (0xf)

/*
 * CTR definitions
 */
#define CTR_CWG_SHIFT      (24)
#define CTR_CWG_MASK       (0xf)
#define CTR_ERG_SHIFT      (20)
#define CTR_ERG_MASK       (0xf)
#define CTR_DMINLINE_SHIFT (16)
#define CTR_DMINLINE_WIDTH (4)
#define CTR_DMINLINE_MASK  (((1) << 4) - (1))
#define CTR_L1IP_SHIFT     (14)
#define CTR_L1IP_MASK      (0x3)
#define CTR_IMINLINE_SHIFT (0)
#define CTR_IMINLINE_MASK  (0xf)

/* System register defines The format is: coproc, opt1, CRn, CRm, opt2 */
#define SCR        p15, 0, c1, c1, 0
#define SCTLR      p15, 0, c1, c0, 0
#define ACTLR      p15, 0, c1, c0, 1
#define SDCR       p15, 0, c1, c3, 1
#define MPIDR      p15, 0, c0, c0, 5
#define MIDR       p15, 0, c0, c0, 0
#define HVBAR      p15, 4, c12, c0, 0
#define VBAR       p15, 0, c12, c0, 0
#define MVBAR      p15, 0, c12, c0, 1
#define NSACR      p15, 0, c1, c1, 2
#define CPACR      p15, 0, c1, c0, 2
#define DCCIMVAC   p15, 0, c7, c14, 1
#define DCCMVAC    p15, 0, c7, c10, 1
#define DCIMVAC    p15, 0, c7, c6, 1
#define DCCISW     p15, 0, c7, c14, 2
#define DCCSW      p15, 0, c7, c10, 2
#define DCISW      p15, 0, c7, c6, 2
#define CTR        p15, 0, c0, c0, 1
#define CNTFRQ     p15, 0, c14, c0, 0
#define ID_MMFR3   p15, 0, c0, c1, 7
#define ID_MMFR4   p15, 0, c0, c2, 6
#define ID_DFR0    p15, 0, c0, c1, 2
#define ID_DFR1    p15, 0, c0, c3, 5
#define ID_PFR0    p15, 0, c0, c1, 0
#define ID_PFR1    p15, 0, c0, c1, 1
#define ID_PFR2    p15, 0, c0, c3, 4
#define MAIR0      p15, 0, c10, c2, 0
#define MAIR1      p15, 0, c10, c2, 1
#define TTBCR      p15, 0, c2, c0, 2
#define TTBR0      p15, 0, c2, c0, 0
#define TTBR1      p15, 0, c2, c0, 1
#define TLBIALL    p15, 0, c8, c7, 0
#define TLBIALLH   p15, 4, c8, c7, 0
#define TLBIALLIS  p15, 0, c8, c3, 0
#define TLBIMVA    p15, 0, c8, c7, 1
#define TLBIMVAA   p15, 0, c8, c7, 3
#define TLBIMVAAIS p15, 0, c8, c3, 3
#define TLBIMVAHIS p15, 4, c8, c3, 1
#define BPIALLIS   p15, 0, c7, c1, 6
#define BPIALL     p15, 0, c7, c5, 6
#define ICIALLU    p15, 0, c7, c5, 0
#define HSCTLR     p15, 4, c1, c0, 0
#define HCR        p15, 4, c1, c1, 0
#define HCPTR      p15, 4, c1, c1, 2
#define HSTR       p15, 4, c1, c1, 3
#define CNTHCTL    p15, 4, c14, c1, 0
#define CNTKCTL    p15, 0, c14, c1, 0
#define VPIDR      p15, 4, c0, c0, 0
#define VMPIDR     p15, 4, c0, c0, 5
#define ISR        p15, 0, c12, c1, 0
#define CLIDR      p15, 1, c0, c0, 1
#define CSSELR     p15, 2, c0, c0, 0
#define CCSIDR     p15, 1, c0, c0, 0
#define CCSIDR2    p15, 1, c0, c0, 2
#define HTCR       p15, 4, c2, c0, 2
#define HMAIR0     p15, 4, c10, c2, 0
#define ATS1CPR    p15, 0, c7, c8, 0
#define ATS1HR     p15, 4, c7, c8, 0
#define DBGOSDLR   p14, 0, c1, c3, 4

/* Debug register defines. The format is: coproc, opt1, CRn, CRm, opt2 */
#define HDCR       p15, 4, c1, c1, 1
#define PMCR       p15, 0, c9, c12, 0
#define CNTHP_TVAL p15, 4, c14, c2, 0
#define CNTHP_CTL  p15, 4, c14, c2, 1

/* AArch32 coproc registers for 32bit MMU descriptor support */
#define PRRR p15, 0, c10, c2, 0
#define NMRR p15, 0, c10, c2, 1
#define DACR p15, 0, c3, c0, 0

/* GICv3 CPU Interface system register defines. The format is: coproc, opt1, CRn, CRm, opt2 */
#define ICC_IAR1    p15, 0, c12, c12, 0
#define ICC_IAR0    p15, 0, c12, c8, 0
#define ICC_EOIR1   p15, 0, c12, c12, 1
#define ICC_EOIR0   p15, 0, c12, c8, 1
#define ICC_HPPIR1  p15, 0, c12, c12, 2
#define ICC_HPPIR0  p15, 0, c12, c8, 2
#define ICC_BPR1    p15, 0, c12, c12, 3
#define ICC_BPR0    p15, 0, c12, c8, 3
#define ICC_DIR     p15, 0, c12, c11, 1
#define ICC_PMR     p15, 0, c4, c6, 0
#define ICC_RPR     p15, 0, c12, c11, 3
#define ICC_CTLR    p15, 0, c12, c12, 4
#define ICC_MCTLR   p15, 6, c12, c12, 4
#define ICC_SRE     p15, 0, c12, c12, 5
#define ICC_HSRE    p15, 4, c12, c9, 5
#define ICC_MSRE    p15, 6, c12, c12, 5
#define ICC_IGRPEN0 p15, 0, c12, c12, 6
#define ICC_IGRPEN1 p15, 0, c12, c12, 7
#define ICC_MGRPEN1 p15, 6, c12, c12, 7

/* 64 bit system register defines The format is: coproc, opt1, CRm */
#define TTBR0_64      p15, 0, c2
#define TTBR1_64      p15, 1, c2
#define CNTVOFF_64    p15, 4, c14
#define VTTBR_64      p15, 6, c2
#define CNTPCT_64     p15, 0, c14
#define HTTBR_64      p15, 4, c2
#define CNTHP_CVAL_64 p15, 6, c14
#define PAR_64        p15, 0, c7

#define MIDR_REV_LOW  GENMASK(3, 0)
#define MIDR_PART     GENMASK(15, 4)
#define MIDR_REV_HIGH GENMASK(23, 20)

#endif