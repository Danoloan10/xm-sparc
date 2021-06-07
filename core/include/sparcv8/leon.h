/*
 * $FILE: leon.h
 *
 * LEON definitions
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

/*
  Changelog:
  - [05/08/2015:SPR-010915-01] 1553 registers updated.
  - [02/09/2015:SPR-020915-01] MISRA rules.9.1 violation fixed.
  - [17/11/2015:SPR-221015-01] update for leon edac support
  - [10/01/2016:SPR-111215-04] LEON_MMU_FLUSH ASI fixed
*/

#ifndef _XM_ARCH_LEON_H_
#define _XM_ARCH_LEON_H_

#define DEFAULT_PORT LEON_UART1_BASE
#define LEON_PRESCALER_KHZ (1000) // 1 Mhz

#ifdef CONFIG_LEON2
#define LEON_IOREGS 0x80000000

#define LEON_MEMORY_CFG_BASE (LEON_IOREGS+0x0)
#define LEON_MEMORY_WPR_BASE (LEON_IOREGS+0x1C)
#define LEON_PCR_BASE (LEON_IOREGS+0x24)
#define LEON_FAILAR_BASE (LEON_IOREGS+0xC)
#define LEON_FAILSR_BASE (LEON_IOREGS+0x10)
#define LEON_CCR_BASE (LEON_IOREGS+0x14)
#define LEON_IDLE_BASE (LEON_IOREGS+0x18)
#define LEON_WATCHDOG_BASE (LEON_IOREGS+0x4C)
#define LEON_TIMER1_BASE (LEON_IOREGS+0x40)
#define LEON_TIMER2_BASE (LEON_IOREGS+0x50)
#define LEON_TIMER_CFG_BASE (LEON_IOREGS+0x60)
#define LEON_UART1_BASE (LEON_IOREGS+0x70)
#define LEON_UART2_BASE (LEON_IOREGS+0x80)
#define LEON_PIC_BASE (LEON_IOREGS+0x90)
#define LEON_GPIO_BASE (LEON_IOREGS+0xA0)

#define LEON_WPR1_OFFSET 0x0
#define LEON_WPR2_OFFSET 0x4

#define IODIR_REG 0x4

#define LEON_IFLUSH_ASI 0x05
#define LEON_DFLUSH_ASI 0x06
#define LEON_MMU_FLUSH 0x18 

#elif CONFIG_LEON3
#define LEON_IOREGS 0x80000000

#define LEON_IO_AREA 0xfff00000
#define LEON_MEMORY_CFG_BASE (LEON_IOREGS+0x0)
#define LEON_MEMORY_WPR_BASE (LEON_IOREGS+0x1C)
#define LEON_TIMER_CFG_BASE (LEON_IOREGS+0x300)
#define LEON_TIMER1_BASE (LEON_IOREGS+0x310)
#define LEON_TIMER2_BASE (LEON_IOREGS+0x320)
#define LEON_UART1_BASE (LEON_IOREGS+0x100)
#define LEON_UART2_BASE (LEON_IOREGS+0x100100)
#define LEON_PIC_BASE (LEON_IOREGS+0x200)

#define LEON_GRGPREG_BASE (LEON_IOREGS+0x600)
#define LEON_CLKGATE_BASE (LEON_IOREGS+0xD00)

#define LEON_CCR_BASE 0x2

#define LEON_MMU_FLUSH 0x18 

#elif CONFIG_LEON3FT
#define LEON_IOREGS 0x80000000

#define LEON_IO_AREA 0xfff00000

#define LEON_CFG (LEON_IOREGS+0x100000)

#define LEON_MEMORY_CFG_BASE (LEON_IOREGS+0x0)
#define LEON_TIMER_CFG_BASE (LEON_IOREGS+0x300)
#define LEON_TIMER1_BASE (LEON_IOREGS+0x310)
#define LEON_TIMER2_BASE (LEON_IOREGS+0x320)
#define LEON_TIMER3_BASE (LEON_IOREGS+0x330)
#define LEON_TIMER4_BASE (LEON_IOREGS+0x340)
#define LEON_UART1_BASE (LEON_IOREGS+0x100)
#define LEON_UART2_BASE (LEON_IOREGS+0x100100)
#define LEON_TIMER_LATCH_BASE (LEON_IOREGS+0x100610)
#define LEON_PIC_BASE (LEON_IOREGS+0x200)

#define LEON_GRGPREG_BASE (LEON_IOREGS+0x600)
#define LEON_CLKGATE_BASE (LEON_IOREGS+0xD00)

#define LEON_CCR_BASE 0x2

#define LEON_IFLUSH_ASI 0x10
#define LEON_DFLUSH_ASI 0x11
/* Updated to the ASI required by GR712RC technical document */
#define LEON_MMU_FLUSH 0x18

#define LEON_AHB_BUS_ERROR_IRQ 1
#define LEON_AHB_STATUS (LEON_IOREGS+0xF00)
#define LEON_AHB_FAILING_ADDRESS (LEON_IOREGS+0xF04)

#elif CONFIG_LEON4

#define LEON_IO_AREA 0xfff00000
#define LEON_APBBRIDGE0 0xff900000
#define LEON_UART1_BASE (LEON_APBBRIDGE0+0x0)
#define LEON_UART2_BASE (LEON_APBBRIDGE0+0x1000)

#define LEON_L2CACHE_BASE 0xf0000000
#define LEON_CCR_BASE 0x2

#define LEON_L2CACHE_COPY_BACK      (0<<1)
#define LEON_L2CACHE_WRITE_THROUGH  (1<<1)

#define LEON_TIMER_CFG_BASE (LEON_APBBRIDGE0+0x8000)

#define LEON_TIMER1_BASE (LEON_TIMER_CFG_BASE+0x10)
#define LEON_TIMER2_BASE (LEON_TIMER_CFG_BASE+0x20)

#define LEON_IFLUSH_ASI 0x10
#define LEON_DFLUSH_ASI 0x11
#define LEON_MMU_FLUSH 0x18 

#define LEON_APIC_BASE (LEON_APBBRIDGE0+0x4000)
#define LEON_PIC_BASE LEON_APIC_BASE

#define LEON_IOMMU_BASE 0xff840000

#define TIMER1_TRAP_NR 1
#define TIMER2_TRAP_NR 2

#else
#error Processor not valid
#endif

//#define LEON_MMU_FLUSH 0x18 //0x03
#define LEON_MMU_BYPASS 0x1c
#define LEON_MMU_ASI 0x19
#define LEON_MMU_CTRL_REG 0x0
#define LEON_MMU_CPR_REG 0x100
#define LEON_MMU_CTXT_REG 0x200
#define LEON_MMU_FSTAT_REG 0x300
#define LEON_MMU_FADDR_REG 0x400

#ifdef _XM_KERNEL_

#define CONFIG_REGISTER_WINDOWS 8

// LEON_MEMORY_CFG_BASE
#define IOP_BIT 19
#define IOP_MASK (1<<IOP_BIT)

#ifndef __ASSEMBLY__

static inline void StoreIoReg(xmAddress_t pAddr, xm_u32_t value) {
#if defined(CONFIG_MMU)
    __asm__ __volatile__("sta %0, [%1] %2\n\t"::"r"(value),
			 "r"(pAddr), "i"(LEON_MMU_BYPASS):"memory");
#else
    __asm__ __volatile__("st %0, [%1]\n\t"::"r"(value), "r"(pAddr):"memory");
#endif
}


static inline xm_u32_t LoadIoReg(xmAddress_t pAddr) {
    xm_u32_t retVal = 0U;
#if defined(CONFIG_MMU)
    __asm__ __volatile__("lda [%1] %2, %0\n\t": "=r"(retVal): "r"(pAddr), "i"(LEON_MMU_BYPASS));
#else
    __asm__ __volatile__("ld [%1], %0\n\t": "=r"(retVal): "r"(pAddr));
#endif
    return retVal;
}

#ifdef CONFIG_MMU
static inline xm_u32_t ReadByPassMmuWord(void *pAddr) {
    xm_u32_t retVal = 0U;
    __asm__ __volatile__("lda [%1] %2, %0\n\t": "=r"(retVal): "r"(pAddr), "i"(LEON_MMU_BYPASS));
    return retVal;
}

static inline void WriteByPassMmuWord(void *pAddr, xm_u32_t val) {
    __asm__ __volatile__("sta %0, [%1] %2\n\t"::"r"(val),
			 "r"(pAddr), "i"(LEON_MMU_BYPASS):"memory");
}

static inline xm_u16_t ReadByPassMmuHalfWord(void *pAddr) {
    xm_u16_t retVal = 0U;
    __asm__ __volatile__("lduha [%1] %2, %0\n\t": "=r"(retVal): "r"(pAddr), "i"(LEON_MMU_BYPASS));
    return retVal;
}

static inline void WriteByPassMmuHalfWord(void *pAddr, xm_u16_t val) {
    __asm__ __volatile__("stha %0, [%1] %2\n\t"::"r"(val),
			 "r"(pAddr), "i"(LEON_MMU_BYPASS):"memory");
}
#else

static inline xm_u32_t ReadByPassMmuWord(void *pAddr) {
    return *(xm_u32_t *)pAddr;
}

static inline void WriteByPassMmuWord(void *pAddr, xm_u32_t val) {
    *(xm_u32_t *)pAddr=val;
}

#endif

#endif
#endif
#endif
