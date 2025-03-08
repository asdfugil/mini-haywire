/* SPDX-License-Identifier: MIT */

#include <stdint.h>

#include "proxy.h"
#include "assert.h"
#include "exception.h"
#include "heapblock.h"
#include "iodev.h"
#include "kboot.h"
#include "malloc.h"
#include "memory.h"
#include "smp.h"
#include "string.h"
#include "types.h"
#include "uart.h"
#include "uartproxy.h"
#include "utils.h"
#include "xnuboot.h"

#include "minilzlib/minlzma.h"
#include "tinf/tinf.h"

int proxy_process(ProxyRequest *request, ProxyReply *reply)
{
    enum exc_guard_t guard_save = exc_guard;

    reply->opcode = request->opcode;
    reply->status = S_OK;
    reply->retval = 0;
    switch (request->opcode) {
        case P_NOP:
            break;
        case P_EXIT:
            if (request->args[0])
                return request->args[0];
            return 1;
        case P_CALL: {
            generic_func *f = (generic_func *)request->args[0];
            reply->retval =
                f(request->args[1], request->args[2], request->args[3], request->args[4]);
            break;
        }
        case P_GET_BOOTARGS:
            reply->retval = boot_args_addr;
            break;
        case P_GET_BASE:
            reply->retval = (u64)_base;
            break;
        case P_SET_BAUD: {
            int cnt = request->args[1];
            printf("Changing baud rate to %llu...\n", request->args[0]);
            uart_setbaud(request->args[0]);
            while (cnt--) {
                uart_putbyte(request->args[2]);
                uart_putbyte(request->args[2] >> 8);
                uart_putbyte(request->args[2] >> 16);
                uart_putbyte(request->args[2] >> 24);
            }
            break;
        }
        case P_UDELAY:
            udelay(request->args[0]);
            break;
        case P_SET_EXC_GUARD:
            exc_count = 0;
            guard_save = request->args[0];
            break;
        case P_GET_EXC_COUNT:
            reply->retval = exc_count;
            exc_count = 0;
            break;
        case P_EL0_CALL:
        case P_EL1_CALL:
        case P_VECTOR:
        case P_GL1_CALL:
        case P_GL2_CALL:
        case P_GET_SIMD_STATE:
        case P_PUT_SIMD_STATE:
            reply->status = S_BADCMD;
            break;
        case P_REBOOT:
            reboot();
            break;
        case P_SLEEP:
        case P_EL3_CALL:
            reply->status = S_BADCMD;
        case P_WRITE64:
            exc_guard = GUARD_SKIP;
            write64(request->args[0], request->args[1]);
            break;
        case P_WRITE32:
            exc_guard = GUARD_SKIP;
            write32(request->args[0], request->args[1]);
            break;
        case P_WRITE16:
            exc_guard = GUARD_SKIP;
            write16(request->args[0], request->args[1]);
            break;
        case P_WRITE8:
            exc_guard = GUARD_SKIP;
            write8(request->args[0], request->args[1]);
            break;

        case P_READ64:
            exc_guard = GUARD_MARK;
            reply->retval = read64(request->args[0]);
            break;
        case P_READ32:
            exc_guard = GUARD_MARK;
            reply->retval = read32(request->args[0]);
            break;
        case P_READ16:
            exc_guard = GUARD_MARK;
            reply->retval = read16(request->args[0]);
            break;
        case P_READ8:
            exc_guard = GUARD_MARK;
            reply->retval = read8(request->args[0]);
            break;

        case P_SET64:
            exc_guard = GUARD_MARK;
            reply->retval = set64(request->args[0], request->args[1]);
            break;
        case P_SET32:
            exc_guard = GUARD_MARK;
            reply->retval = set32(request->args[0], request->args[1]);
            break;
        case P_SET16:
            exc_guard = GUARD_MARK;
            reply->retval = set16(request->args[0], request->args[1]);
            break;
        case P_SET8:
            exc_guard = GUARD_MARK;
            reply->retval = set8(request->args[0], request->args[1]);
            break;

        case P_CLEAR64:
            exc_guard = GUARD_MARK;
            reply->retval = clear64(request->args[0], request->args[1]);
            break;
        case P_CLEAR32:
            exc_guard = GUARD_MARK;
            reply->retval = clear32(request->args[0], request->args[1]);
            break;
        case P_CLEAR16:
            exc_guard = GUARD_MARK;
            reply->retval = clear16(request->args[0], request->args[1]);
            break;
        case P_CLEAR8:
            exc_guard = GUARD_MARK;
            reply->retval = clear8(request->args[0], request->args[1]);
            break;

        case P_MASK64:
            exc_guard = GUARD_MARK;
            reply->retval = mask64(request->args[0], request->args[1], request->args[2]);
            break;
        case P_MASK32:
            exc_guard = GUARD_MARK;
            reply->retval = mask32(request->args[0], request->args[1], request->args[2]);
            break;
        case P_MASK16:
            exc_guard = GUARD_MARK;
            reply->retval = mask16(request->args[0], request->args[1], request->args[2]);
            break;
        case P_MASK8:
            exc_guard = GUARD_MARK;
            reply->retval = mask8(request->args[0], request->args[1], request->args[2]);
            break;

        case P_WRITEREAD64:
            exc_guard = GUARD_MARK;
            reply->retval = writeread64(request->args[0], request->args[1]);
            break;
        case P_WRITEREAD32:
            exc_guard = GUARD_MARK;
            reply->retval = writeread32(request->args[0], request->args[1]);
            break;
        case P_WRITEREAD16:
            exc_guard = GUARD_MARK;
            reply->retval = writeread16(request->args[0], request->args[1]);
            break;
        case P_WRITEREAD8:
            exc_guard = GUARD_MARK;
            reply->retval = writeread8(request->args[0], request->args[1]);
            break;

        case P_MEMCPY64:
            exc_guard = GUARD_RETURN;
            memcpy64((void *)request->args[0], (void *)request->args[1], request->args[2]);
            break;
        case P_MEMCPY32:
            exc_guard = GUARD_RETURN;
            memcpy32((void *)request->args[0], (void *)request->args[1], request->args[2]);
            break;
        case P_MEMCPY16:
            exc_guard = GUARD_RETURN;
            memcpy16((void *)request->args[0], (void *)request->args[1], request->args[2]);
            break;
        case P_MEMCPY8:
            exc_guard = GUARD_RETURN;
            memcpy8((void *)request->args[0], (void *)request->args[1], request->args[2]);
            break;

        case P_MEMSET64:
            exc_guard = GUARD_RETURN;
            memset64((void *)request->args[0], request->args[1], request->args[2]);
            break;
        case P_MEMSET32:
            exc_guard = GUARD_RETURN;
            memset32((void *)request->args[0], request->args[1], request->args[2]);
            break;
        case P_MEMSET16:
            exc_guard = GUARD_RETURN;
            memset16((void *)request->args[0], request->args[1], request->args[2]);
            break;
        case P_MEMSET8:
            exc_guard = GUARD_RETURN;
            memset8((void *)request->args[0], request->args[1], request->args[2]);
            break;

        case P_IC_IALLUIS:
            ic_ialluis();
            break;
        case P_IC_IALLU:
            ic_iallu();
            break;
        case P_IC_IVAU:
            ic_ivau_range((void *)request->args[0], request->args[1]);
            break;
        case P_DC_IVAC:
            dc_ivac_range((void *)request->args[0], request->args[1]);
            break;
        case P_DC_ISW:
            dc_isw((void *)request->args[0]);
            break;
        case P_DC_CSW:
            dc_csw((void *)request->args[0]);
            break;
        case P_DC_CISW:
            dc_cisw((void *)request->args[0]);
            break;
        case P_DC_ZVA:
            reply->status = S_BADCMD;
            break;
        case P_DC_CVAC:
            dc_cvac_range((void *)request->args[0], request->args[1]);
            break;
        case P_DC_CVAU:
            dc_cvau_range((void *)request->args[0], request->args[1]);
            break;
        case P_DC_CIVAC:
            dc_civac_range((void *)request->args[0], request->args[1]);
            break;
        case P_MMU_SHUTDOWN:
            mmu_shutdown();
            break;
        case P_MMU_INIT:
            mmu_init();
            break;
        case P_MMU_DISABLE:
            reply->retval = mmu_disable();
            break;
        case P_MMU_RESTORE:
            mmu_restore(request->args[0]);
            break;
        case P_MMU_INIT_SECONDARY:
            break;

        case P_XZDEC: {
            uint32_t destlen, srclen;
            destlen = request->args[3];
            srclen = request->args[1];
            if (XzDecode((void *)request->args[0], &srclen, (void *)request->args[2], &destlen))
                reply->retval = destlen;
            else
                reply->retval = ~0L;
            break;
        }
        case P_GZDEC: {
            unsigned int destlen, srclen;
            destlen = request->args[3];
            srclen = request->args[1];
            size_t ret = tinf_gzip_uncompress((void *)request->args[2], &destlen,
                                              (void *)request->args[0], &srclen);
            if (ret != TINF_OK)
                reply->retval = ret;
            else
                reply->retval = destlen;
            break;
        }

        case P_SMP_START_SECONDARIES:
            smp_start_secondaries();
            break;
        case P_SMP_STOP_SECONDARIES:
            smp_stop_secondaries();
            break;
        case P_SMP_CALL:
        case P_SMP_CALL_SYNC:
        case P_SMP_WAIT:
        case P_SMP_SET_WFE_MODE:
        case P_SMP_IS_ALIVE:
        case P_SMP_CALL_EL1:
        case P_SMP_CALL_EL1_SYNC:
        case P_SMP_CALL_EL0:
        case P_SMP_CALL_EL0_SYNC:
            reply->status = S_BADCMD;
            break;

        case P_HEAPBLOCK_ALLOC:
            reply->retval = (u64)heapblock_alloc(request->args[0]);
            break;
        case P_MALLOC:
            reply->retval = (u64)malloc(request->args[0]);
            break;
        case P_TOP_OF_MEMORY_ALLOC:
            reply->retval = (u64)top_of_memory_alloc(request->args[0]);
            break;
        case P_MEMALIGN:
            reply->retval = (u64)memalign(request->args[0], request->args[1]);
            break;
        case P_FREE:
            free((void *)request->args[0]);
            break;

        case P_KBOOT_BOOT:
            if (kboot_boot((void *)request->args[0]) == 0)
                return 1;
        case P_KBOOT_SET_CHOSEN:
            reply->retval = kboot_set_chosen((void *)request->args[0], (void *)request->args[1]);
            break;
        case P_KBOOT_SET_INITRD:
            kboot_set_initrd((void *)request->args[0], request->args[1]);
            break;
        case P_KBOOT_PREPARE_DT:
            reply->retval = kboot_prepare_dt((void *)request->args[0]);
            break;
        case P_KBOOT_GET_DT:
            reply->retval = (u64)dt;
            break;
        case P_PMGR_POWER_ENABLE:
        case P_PMGR_POWER_DISABLE:
        case P_PMGR_ADT_POWER_ENABLE:
        case P_PMGR_ADT_POWER_DISABLE:
        case P_PMGR_RESET:
            reply->status = S_BADCMD;
            break;

        case P_IODEV_SET_USAGE:
            iodev_set_usage(request->args[0], request->args[1]);
            break;
        case P_IODEV_CAN_READ:
            reply->retval = iodev_can_read(request->args[0]);
            break;
        case P_IODEV_CAN_WRITE:
            reply->retval = iodev_can_write(request->args[0]);
            break;
        case P_IODEV_READ:
            reply->retval =
                iodev_read(request->args[0], (void *)request->args[1], request->args[2]);
            break;
        case P_IODEV_WRITE:
            reply->retval =
                iodev_write(request->args[0], (void *)request->args[1], request->args[2]);
            break;
        case P_IODEV_WHOAMI:
            reply->retval = uartproxy_iodev;
            break;

        case P_USB_IODEV_VUART_SETUP:
        case P_TUNABLES_APPLY_GLOBAL:
        case P_TUNABLES_APPLY_LOCAL:
        case P_TUNABLES_APPLY_LOCAL_ADDR:
        case P_DART_INIT:
        case P_DART_SHUTDOWN:
        case P_DART_MAP:
        case P_DART_UNMAP:
        case P_HV_INIT:
        case P_HV_MAP:
        case P_HV_START:
        case P_HV_TRANSLATE:
        case P_HV_PT_WALK:
        case P_HV_MAP_VUART:
        case P_HV_MAP_VIRTIO:
        case P_VIRTIO_PUT_BUFFER:
        case P_HV_TRACE_IRQ:
        case P_HV_WDT_START:
        case P_HV_START_SECONDARY:
        case P_HV_SWITCH_CPU:
        case P_HV_SET_TIME_STEALING:
        case P_HV_PIN_CPU:
        case P_HV_WRITE_HCR:
        case P_HV_EXIT_CPU:
        case P_HV_ADD_TIME:
        case P_FB_INIT:
        case P_FB_SHUTDOWN:
        case P_FB_BLIT:
        case P_FB_UNBLIT:
        case P_FB_FILL:
        case P_FB_CLEAR:
        case P_FB_DISPLAY_LOGO:
        case P_FB_RESTORE_LOGO:
        case P_FB_IMPROVE_LOGO:
        case P_PCIE_INIT:
        case P_PCIE_SHUTDOWN:
        case P_NVME_INIT:
        case P_NVME_SHUTDOWN:
        case P_NVME_READ:
        case P_NVME_FLUSH:
        case P_MCC_GET_CARVEOUTS:
        case P_DISPLAY_INIT:
        case P_DISPLAY_CONFIGURE:
        case P_DISPLAY_SHUTDOWN:
        case P_DISPLAY_START_DCP:
        case P_DISPLAY_IS_EXTERNAL:
        case P_DAPF_INIT_ALL:
        case P_DAPF_INIT:
        case P_CPUFREQ_INIT:
        case P_CPUFREQ_PREPARE_BAUD:
        default:
            reply->status = S_BADCMD;
            break;
    }
    sysop("dsb sy");
    sysop("isb");
    exc_guard = guard_save;
    return 0;
}
