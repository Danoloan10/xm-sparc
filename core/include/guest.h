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

#ifndef _XM_GUEST_H_
#define _XM_GUEST_H_

#define XM_HYPERVISOR_ID 0xff
#define UNCHECKED_ID 0xff
#define KTHREAD_MAGIC 0xa5a55a5a

/* <track id="xm-interrupt-list"> */
#define XM_VT_HW_FIRST             (0)
#define XM_VT_HW_LAST              (31)
#define XM_VT_HW_MAX               (32)

#ifdef CONFIG_LEON3FT

#define XM_VT_HW_INTERNAL_BUS_TRAP_NR (1+XM_VT_HW_FIRST)
#define XM_VT_HW_UART1_TRAP_NR        (2+XM_VT_HW_FIRST)
#define XM_VT_HW_IO_IRQ0_TRAP_NR      (3+XM_VT_HW_FIRST)
#define XM_VT_HW_IO_IRQ1_TRAP_NR      (4+XM_VT_HW_FIRST)
#define XM_VT_HW_TIMER_LATCH_TRAP_NR  (7+XM_VT_HW_FIRST)
#define XM_VT_HW_TIMER1_TRAP_NR       (8+XM_VT_HW_FIRST)
#define XM_VT_HW_TIMER2_TRAP_NR       (9+XM_VT_HW_FIRST)
#define XM_VT_HW_TIMER3_TRAP_NR       (10+XM_VT_HW_FIRST)
#define XM_VT_HW_TIMER4_TRAP_NR       (11+XM_VT_HW_FIRST)
#define XM_VT_HW_PCI_TRAP_NR          (14+XM_VT_HW_FIRST)
#define XM_VT_HW_UART2_TRAP_NR        (17+XM_VT_HW_FIRST)
#define XM_VT_HW_UART3_TRAP_NR        (18+XM_VT_HW_FIRST)
#define XM_VT_HW_UART4_TRAP_NR        (19+XM_VT_HW_FIRST)
#define XM_VT_HW_UART5_TRAP_NR        (20+XM_VT_HW_FIRST)
#define XM_VT_HW_UART6_TRAP_NR        (21+XM_VT_HW_FIRST)

#else

#define XM_VT_HW_INTERNAL_BUS_TRAP_NR (1+XM_VT_HW_FIRST)
#define XM_VT_HW_UART2_TRAP_NR        (2+XM_VT_HW_FIRST)
#define XM_VT_HW_UART1_TRAP_NR        (3+XM_VT_HW_FIRST)
#define XM_VT_HW_IO_IRQ0_TRAP_NR      (4+XM_VT_HW_FIRST)
#define XM_VT_HW_IO_IRQ1_TRAP_NR      (5+XM_VT_HW_FIRST)
#define XM_VT_HW_IO_IRQ2_TRAP_NR      (6+XM_VT_HW_FIRST)
#define XM_VT_HW_IO_IRQ3_TRAP_NR      (7+XM_VT_HW_FIRST)
#define XM_VT_HW_TIMER1_TRAP_NR       (8+XM_VT_HW_FIRST)
#define XM_VT_HW_TIMER2_TRAP_NR       (9+XM_VT_HW_FIRST)
#define XM_VT_HW_DSU_TRAP_NR          (11+XM_VT_HW_FIRST)
#define XM_VT_HW_PCI_TRAP_NR          (14+XM_VT_HW_FIRST)

#endif

#define XM_VT_EXT_FIRST            (0)
#define XM_VT_EXT_LAST             (31)
#define XM_VT_EXT_MAX              (32)

#define XM_VT_EXT_HW_TIMER          (0+XM_VT_EXT_FIRST)
#define XM_VT_EXT_EXEC_TIMER        (1+XM_VT_EXT_FIRST)
/*                                  (2+XM_VT_EXT_FIRST) not used*/
#define XM_VT_EXT_SHUTDOWN          (3+XM_VT_EXT_FIRST)
#define XM_VT_EXT_SAMPLING_PORT     (4+XM_VT_EXT_FIRST)
#define XM_VT_EXT_QUEUING_PORT      (5+XM_VT_EXT_FIRST)

#define XM_VT_EXT_CYCLIC_SLOT_START (8+XM_VT_EXT_FIRST)

#define XM_VT_EXT_MEM_PROTECT       (16+XM_VT_EXT_FIRST)

/* Inter-Partition Virtual Interrupts */
#define XM_MAX_IPVI                CONFIG_MAX_NO_IPVI
#define XM_VT_EXT_IPVI0            (24+XM_VT_EXT_FIRST)
#define XM_VT_EXT_IPVI1            (25+XM_VT_EXT_FIRST)
#define XM_VT_EXT_IPVI2            (26+XM_VT_EXT_FIRST)
#define XM_VT_EXT_IPVI3            (27+XM_VT_EXT_FIRST)
#define XM_VT_EXT_IPVI4            (28+XM_VT_EXT_FIRST)
#define XM_VT_EXT_IPVI5            (29+XM_VT_EXT_FIRST)
#define XM_VT_EXT_IPVI6            (30+XM_VT_EXT_FIRST)
#define XM_VT_EXT_IPVI7            (31+XM_VT_EXT_FIRST)

/* </track id="xm-interrupt-list"> */

#define XM_EXT_TRAPS ((1<<(XM_VT_EXT_MEM_PROTECT-XM_VT_EXT_FIRST)))

#ifndef __ASSEMBLY__

#ifdef _XM_KERNEL_
#include <arch/atomic.h>
#include <arch/guest.h>
#include <arch/irqs.h>
#include <xmconf.h>
#include <objects/hm.h>
#else
#include <xm_inc/arch/atomic.h>
#include <xm_inc/arch/guest.h>
#include <xm_inc/arch/irqs.h>
#include <xm_inc/xmconf.h>
#include <xm_inc/objects/hm.h>
#endif

//@ \void{<track id="xmPhysicalMemMap">}
struct xmPhysicalMemMap {
    xm_s8_t name[CONFIG_ID_STRING_LENGTH];
    xmAddress_t startAddr;
    xmAddress_t mappedAt;
    xmSize_t size;
#define XM_MEM_AREA_UNMAPPED        (1<<0)
#define XM_MEM_AREA_READONLY        (1<<1)
#define XM_MEM_AREA_UNCACHEABLE     (1<<2)
#define XM_MEM_AREA_ROM             (1<<3)
#define XM_MEM_AREA_FLAG0           (1<<4)
#define XM_MEM_AREA_FLAG1           (1<<5)
#define XM_MEM_AREA_FLAG2           (1<<6)
#define XM_MEM_AREA_FLAG3           (1<<7)
#define XM_MEM_AREA_TAGGED          (1<<8)
    xm_u32_t flags;
};
//@ \void{</track id="xmPhysicalMemMap">}
/* <track id="doc-Partition-Control-Table"> */
typedef struct {
    xm_u32_t magic;
    xm_u32_t xmVersion; // XM version
    xm_u32_t xmAbiVersion; // XM's abi version
    xm_u32_t xmApiVersion; // XM's api version
    xm_u32_t resetCounter;
    xm_u32_t resetStatus;
    xm_u32_t cpuKhz;
    xmId_t id;
    // Copy of kthread->ctrl.flags
    xm_u32_t flags;    
#define PARTITION_SYSTEM_F (1<<0) // 1:SYSTEM
#define PARTITION_FP_F (1<<1) // Floating point enabled
#define PARTITION_HALTED_F (1<<2)  // 1:HALTED
#define PARTITION_SUSPENDED_F (1<<3) // 1:SUSPENDED
#define PARTITION_READY_F (1<<4) // 1:READY
//#define PARTITION_FLUSH_DCACHE_F (1<<5)
//#define PARTITION_FLUSH_ICACHE_F (1<<6)
#define PARTITION_DCACHE_ENABLED_F (1<<7)
#define PARTITION_ICACHE_ENABLED_F (1<<8)
    xm_u32_t imgStart;
    xm_u32_t hwIrqs; // Hw interrupts belonging to the partition
    xm_s32_t noPhysicalMemAreas;
    xm_s32_t noCommPorts;
    xm_u8_t name[CONFIG_ID_STRING_LENGTH];
    xm_u32_t iFlags; // As defined by the ARCH (ET+PIL in sparc)
    xm_u32_t hwIrqsPend; // pending hw irqs
    xm_u32_t hwIrqsMask; // masked hw irqs
    
    xm_u32_t extIrqsPend; // pending extended irqs
    xm_u32_t extIrqsMask; // masked extended irqs
    struct pctArch arch;
    struct {
	xm_u32_t noSlot:16, releasePoint:1, reserved:15;
	xm_u32_t id;
	xm_u32_t slotDuration;
    } schedInfo;
    xm_u16_t trap2Vector[NO_TRAPS];
    xm_u16_t hwIrq2Vector[CONFIG_NO_HWIRQS];
    xm_u16_t extIrq2Vector[XM_VT_EXT_MAX];
} partitionControlTable_t;
/* </track id="doc-Partition-Control-Table">  */

static inline void xmSetBit(xmWord_t bm[], xm_u32_t bp, xm_s32_t maxBits) {
    xm_u32_t e=bp>>XM_LOG2_WORD_SZ, b=bp&((1<<XM_LOG2_WORD_SZ)-1);
    if (bp>=maxBits)
        return;
    bm[e]|=(1<<b);
}

static inline void xmClearBit(xmWord_t bm[], xm_u32_t bp, xm_s32_t maxBits) {
    xm_u32_t e=bp>>XM_LOG2_WORD_SZ, b=bp&((1<<XM_LOG2_WORD_SZ)-1);
    if (bp>=maxBits)
        return;
    bm[e]&=~(1<<b);
}

static inline void xmClearBitmap(xmWord_t bm[], xm_s32_t maxBits) {
    xm_u32_t e;
    for (e=0; e<((maxBits&((1<<(XM_LOG2_WORD_SZ))-1))?
                 (maxBits>>XM_LOG2_WORD_SZ)+1:
                 (maxBits>>XM_LOG2_WORD_SZ)); e++)
        bm[e]=0;
}

static inline void xmSetBitmap(xmWord_t bm[], xm_s32_t maxBits) {
    xm_u32_t e;
    
    for (e=0; e<((maxBits&((1<<(XM_LOG2_WORD_SZ))-1))?
                 (maxBits>>XM_LOG2_WORD_SZ)+1:
                 (maxBits>>XM_LOG2_WORD_SZ)); e++)
        bm[e]=~0;
}

static inline xm_s32_t xmIsBitSet(xmWord_t bm[], xm_s32_t bp, xm_s32_t maxBits) {
    xm_u32_t e=bp>>XM_LOG2_WORD_SZ, b=bp&((1<<XM_LOG2_WORD_SZ)-1);
    if (bp>=maxBits)
        return -1;

    return (bm[e]&(1<<b))?1:0;
}

#endif

#endif
