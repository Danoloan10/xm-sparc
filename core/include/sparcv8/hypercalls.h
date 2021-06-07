/*
 * $FILE: hypercalls.h
 *
 * Processor-related hypercalls definition
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 *
 * Changelog:
 *
 * [dd/mm/yyyy]     [author]
 * [brief description]
 *
 * [30/03/2016]     Ángel Esquinas (aesquinas@fentiss.com)
 * CP-250216-01:
 *     Define __RELOAD_WD_NR
 *     Increment hypercalls number to 35.
 */

#ifndef _XM_ARCH_HYPERCALLS_H_
#define _XM_ARCH_HYPERCALLS_H_

//@ \void{<track id="hypercall-numbers">}
#define __HALT_PARTITION_NR 0
#define __SUSPEND_PARTITION_NR 1
#define __RESUME_PARTITION_NR 2
#define __RESET_PARTITION_NR 3
#define __SHUTDOWN_PARTITION_NR 4
#define __HALT_SYSTEM_NR 5
#define __RESET_SYSTEM_NR 6
#define __IDLE_SELF_NR 7

#define __GET_TIME_NR 8
#define __SET_TIMER_NR 9
#define __READ_OBJECT_NR 10
#define __WRITE_OBJECT_NR 11
#define __SEEK_OBJECT_NR 12
#define __CTRL_OBJECT_NR 13

#define __CLEAR_IRQ_MASK_NR 14
#define __SET_IRQ_MASK_NR 15
#define __FORCE_IRQS_NR 16
#define __CLEAR_IRQS_NR 17
#define __ROUTE_IRQ_NR 18

#define __SET_CACHE_STATE_NR 19

#define __SWITCH_SCHED_PLAN_NR 20
#define __GET_GID_BY_NAME_NR 21

#define sparc_inport_nr 22
#define sparc_outport_nr 23
#define sparc_write_tbr_nr 24

#define sparc_disable_sdp_nr 25
#define sparc_outport_sdp_nr 26
#define sparc_outport_msg_nr 27
#define reserved_0 28
#define reserved_1 29

#define __RESET_VCPU_NR 30
#define __HALT_VCPU_NR 31
#define __GET_VCPUID_NR 32
#define __RAISE_IPVI_NR 33
#define __RELOAD_WD_NR 34

//@ \void{</track id="hypercall-numbers">}

#define NR_HYPERCALLS 35

//@ \void{<track id="asm-hypercall-numbers">}
#define sparc_iret_nr 0
#define sparc_flush_regwin_nr 1
#define sparc_get_psr_nr 2
#define sparc_set_psr_nr 3
#define sparc_set_pil_nr 4
#define sparc_clear_pil_nr 5
#define sparc_ctrl_winflow_nr 6
//@ \void{</track id="asm-hypercall-numbers">}
/*
#define sparc_set_irqmask_nr 7
#define sparc_clear_irqmask_nr 8
#define sparc_clear_irqs_nr 9
*/
#define NR_ASM_HYPERCALLS 7

#ifndef __ASSEMBLY__

#define ASM_HYPERCALL_TAB(_ahc) \
    __asm__ (".section .ahypercallstab, \"a\"\n\t" \
	     ".align 4\n\t" \
	     ".long "#_ahc"\n\t" \
	     ".previous\n\t")

#ifdef CONFIG_SPW_RTC
struct ioOutPortMsg {
   xm_u32_t * msg;
   xm_s32_t size;
   xmAddress_t checkingAddr;
   xm_u32_t validationValue;
   xm_u32_t validationMask;
};
#endif

#endif

#endif
