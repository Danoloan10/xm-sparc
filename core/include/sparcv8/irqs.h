/*
 * $FILE: irqs.h
 *
 * IRQS
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_ARCH_IRQS_H_
#define _XM_ARCH_IRQS_H_

#ifdef _XM_KERNEL_
#include <arch/xm_def.h>
#else
#include <xm_inc/arch/xm_def.h>
#endif

#if defined(CONFIG_LEON3FT)
#define NO_TRAPS 30
#else
#define NO_TRAPS 20
#endif

#ifndef __ASSEMBLY__
struct trapHandler {
    xmAddress_t pc;
};
#endif

#ifdef _XM_KERNEL_

#ifndef __ASSEMBLY__

typedef struct _cpuCtxt {
    struct _cpuCtxt *prev;
    xmWord_t unused;
    xmWord_t y;
    xmWord_t g1;
    xmWord_t g2;
    xmWord_t g3;
    xmWord_t g4;
    xmWord_t g5;
    xmWord_t g6;
    xmWord_t g7;
    xmWord_t nPc;
    xmWord_t irqNr;
    xmWord_t psr;
    xmWord_t pc;
} cpuCtxt_t;

#define CpuCtxt2HmCpuCtxt(cpuCtxt, hmCpuCtxt) do { \
    (hmCpuCtxt)->pc=(cpuCtxt)->pc; \
} while(0)

#define GET_ECODE(ctxt) 0
#define GET_CTXT_PC(ctxt) (ctxt)->pc
#define SET_CTXT_PC(ctxt, _pc) do { \
    (ctxt)->pc=(_pc); \
    (ctxt)->nPc=(_pc)+4; \
} while(0)

#define PrintHmCpuCtxt(ctxt) \
    kprintf("[HM] PC:0x%lx NPC:0x%lx PSR:0x%lx\n", (ctxt)->pc, (ctxt)->nPc, (ctxt)->psr)

#ifndef PSR_PS_BIT
#define PSR_PS_BIT 0x00000040
#endif

#include <guest.h>

static inline void InitPCtrlTabIrqs(xm_u32_t *iFlags) {
    (*iFlags)=PSR_PIL_MASK|PSR_ET_BIT;
}

static inline xm_s32_t ArePCtrlTabIrqsSet(xm_u32_t iFlags) {
    return (!(iFlags&PSR_ET_BIT)||((iFlags&PSR_PIL_MASK)==PSR_PIL_MASK))?0:1;
}

static inline void DisablePCtrlTabIrqs(xm_u32_t *iFlags) {
    (*iFlags)&=~PSR_ET_BIT;
}

static inline void MaskPCtrlTabIrq(xm_u32_t *mask, xm_u32_t bitmap) {
    // doing nothing
}

static inline xm_s32_t IsSvIrqCtxt(cpuCtxt_t *ctxt){
    return (ctxt->psr&PSR_PS_BIT);
}

static inline xm_s32_t ArchEmulTrapIrq(cpuCtxt_t *ctxt, partitionControlTable_t *partCtrlTab, xm_s32_t irqNr) {
    return partCtrlTab->trap2Vector[irqNr];
}


static inline xm_s32_t ArchEmulHwIrq(cpuCtxt_t *ctxt, partitionControlTable_t *partCtrlTab, xm_s32_t irqNr) {
    return partCtrlTab->hwIrq2Vector[irqNr];
}


static inline xm_s32_t ArchEmulExtIrq(cpuCtxt_t *ctxt, partitionControlTable_t *partCtrlTab, xm_s32_t irqNr) {
    return partCtrlTab->extIrq2Vector[irqNr];
}

extern void PicForceIrq(xm_u32_t irq);

extern void PicClearIrq(xm_u32_t irq);

extern void PicEnableIrq(xm_u32_t irq);

extern void PicDisableIrq(xm_u32_t irq);


#endif

/* stack offsets */ 
#define REG_WND_FRAME 0x40

#define L0_OFFS 0x0
#define L1_OFFS 0x4
#define L2_OFFS 0x8
#define L3_OFFS 0xc
#define L4_OFFS 0x10
#define L5_OFFS 0x14
#define L6_OFFS 0x18
#define L7_OFFS 0x1c

#define I0_OFFS 0x20
#define I1_OFFS 0x24
#define I2_OFFS 0x28
#define I3_OFFS 0x2c
#define I4_OFFS 0x30
#define I5_OFFS 0x34
#define I6_OFFS 0x38
#define I7_OFFS 0x3c

#define MIN_STACK_FRAME 0x80 // REG_WND_FRAME+0x20

#endif  //_XM_KERNEL_

/* <track id="hardware-exception-list"> */
#define DATA_STORE_ERROR 0x2b // 0
#define INSTRUCTION_ACCESS_MMU_MISS 0x3c // 1
#define INSTRUCTION_ACCESS_ERROR 0x21 // 2
#define R_REGISTER_ACCESS_ERROR 0x20 // 3
#define INSTRUCTION_ACCESS_EXCEPTION 0x1 // 4
#define PRIVILEGED_INSTRUCTION 0x03 // 5
#define ILLEGAL_INSTRUCTION 0x2 // 6
#define FP_DISABLED 0x4 // 7
#define CP_DISABLED 0x24 // 8
#define UNIMPLEMENTED_FLUSH 0x25 // 9
#define WATCHPOINT_DETECTED 0xb // 10
//#define WINDOW_OVERFLOW 0x5
//#define WINDOW_UNDERFLOW 0x6
#define MEM_ADDRESS_NOT_ALIGNED 0x7 // 11
#define FP_EXCEPTION 0x8 // 12
#define CP_EXCEPTION 0x28 // 13
#define DATA_ACCESS_ERROR 0x29 // 14
#define DATA_ACCESS_MMU_MISS 0x2c // 15
#define DATA_ACCESS_EXCEPTION 0x9 // 16
#define TAG_OVERFLOW 0xa // 17
#define DIVISION_BY_ZERO 0x2a // 18
/* </track id="hardware-exception-list"> */

/* <track id="interrupt-to-trap-list"> */
#define INTERRUPT_LEVEL_15 0x1f // UNUSED // 15
#define INTERRUPT_LEVEL_14 0x1e // PCI // 14
#define INTERRUPT_LEVEL_13 0x1d // UNUSED // 13
#define INTERRUPT_LEVEL_12 0x1c // UNUSED // 12
#define INTERRUPT_LEVEL_11 0x1b // DSU // 11
#define INTERRUPT_LEVEL_10 0x1a // UNUSED // 10
#define INTERRUPT_LEVEL_9 0x19 // TIMER2 // 9
#define INTERRUPT_LEVEL_8 0x18 // TIMER1 // 8
#define INTERRUPT_LEVEL_7 0x17 // IO3 // 7
#define INTERRUPT_LEVEL_6 0x16 // IO2 // 6
#define INTERRUPT_LEVEL_5 0x15 // IO1 // 5
#define INTERRUPT_LEVEL_4 0x14 // IO0 // 4
#define INTERRUPT_LEVEL_3 0x13 // UART1 // 3  //LEON3 -> UART2
#define INTERRUPT_LEVEL_2 0x12 // UART2 // 2  //LEON3 -> UART1
#define INTERRUPT_LEVEL_1 0x11 // AMBA // 1
/* </track id="interrupt-to-trap-list"> */

/* <track id="hardware-interrupt-list"> */
#if defined(CONFIG_LEON2)|| defined(CONFIG_LEON3)

#define INTERNAL_BUS_TRAP_NR 1
#ifdef CONFIG_LEON2
#define UART2_TRAP_NR 2
#define UART1_TRAP_NR 3
#endif
#ifdef CONFIG_LEON3
#define UART2_TRAP_NR 3
#define UART1_TRAP_NR 2
#endif
#define IO_IRQ0_TRAP_NR 4
#define IO_IRQ1_TRAP_NR 5
#define IO_IRQ2_TRAP_NR 6
#define IO_IRQ3_TRAP_NR 7
#define TIMER1_TRAP_NR 8
#define TIMER2_TRAP_NR 9
#define DSU_TRAP_NR 11
#define PCI_TRAP_NR 14

#elif defined(CONFIG_LEON3FT)

  #define INTERNAL_BUS_TRAP_NR 1
  #define UART1_TRAP_NR 2
  #define IO_IRQ0_TRAP_NR 3
  #define IO_IRQ1_TRAP_NR 4
  #define UART2_TRAP_NR 17
  #define UART3_TRAP_NR 18
  #define UART4_TRAP_NR 19
  #define UART5_TRAP_NR 20
  #define UART6_TRAP_NR 21
  #define TIMER_LATCH_TRAP_NR 7	
  #define TIMER1_TRAP_NR 8
  #define TIMER2_TRAP_NR 9
  #define TIMER3_TRAP_NR 10
  #define TIMER4_TRAP_NR 11
  #define PCI_TRAP_NR 14	   

#endif
/* </track id="hardware-interrupt-list"> */

#endif
