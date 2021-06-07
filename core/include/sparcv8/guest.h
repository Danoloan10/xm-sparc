/*
 * $FILE: guest.h
 *
 * Guest shared info
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_ARCH_GUEST_H_
#define _XM_ARCH_GUEST_H_

#ifdef _XM_KERNEL_
#include <arch/atomic.h>
#include <arch/processor.h>
#else
#include <xm_inc/arch/atomic.h>
#include <xm_inc/arch/processor.h>
#endif

#ifdef CONFIG_AT697F
#define XM_VT_HW_IO_IRQ4_TRAP_NR          (10) // TrapNr 0x1A
#define XM_VT_HW_IO_IRQ5_TRAP_NR          (12) // TrapNr 0x1C
#define XM_VT_HW_IO_IRQ6_TRAP_NR          (13) // TrapNr 0x1D
#define XM_VT_HW_IO_IRQ7_TRAP_NR          (15) // TrapNr 0x1F
// Secondary interrupt controller irq lines:
#define XM_VT_HW_IT2_SDRAM                (16) // TrapNr 0x30
#define XM_VT_HW_IT2_EADR                 (17) 
#define XM_VT_HW_IT2_1HZ                  (18)
#define XM_VT_HW_IT2_32HZ                 (19)
#define XM_VT_HW_IT2_IO0                  (20)
#define XM_VT_HW_IT2_IO1                  (21)
#define XM_VT_HW_IT2_IO2                  (22)
#define XM_VT_HW_IT2_IO3                  (23)
#define XM_VT_HW_IT2_UART_TX              (24)
#define XM_VT_HW_IT2_UART_RX              (25)
#define XM_VT_HW_IT2_IOTOP                (26)
#define XM_VT_HW_IT2_OSLNK_RX             (27)
#define XM_VT_HW_IT2_OSLNK_TX             (28)
#define XM_VT_HW_IT2_SPW_RX               (29)
#define XM_VT_HW_IT2_SPW_TX               (30) // TrapNr 0x3E
#endif

// XXX: this structure is visible from the guest
/*  <track id="doc-Partition-Control-Table-Arch"> */
struct pctArch {
    xmAddress_t tbr;
#ifdef CONFIG_MMU
#define _ARCH_PTDL1_REG ptdL1
    volatile xm_u32_t faultStatusReg;
    volatile xm_u32_t faultAddressReg;
#endif
};
/*  </track id="doc-Partition-Control-Table-Arch"> */

#endif
