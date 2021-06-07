/*
 * $FILE: leon_pic.c
 *
 * LEON I/O interrupt controller as defined in Datasheet LEON AT697E
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <assert.h>
#include <kdevice.h>
#include <irqs.h>
#include <processor.h>
#include <arch/processor.h>
#include <arch/xm_def.h>

#if defined(CONFIG_LEON2) || defined(CONFIG_LEON3) || defined(CONFIG_LEON4) || defined (CONFIG_LEON3FT)

#if defined(CONFIG_LEON3) || (CONFIG_LEON4) || (CONFIG_LEON3FT)
RESERVE_IOPORTS(LEON_PIC_BASE, 10);
#define ITMP_REG 0x40
RESERVE_IOPORTS(LEON_PIC_BASE+ITMP_REG, 1);
#else
RESERVE_IOPORTS(LEON_PIC_BASE, 4);
#define ITMP_REG 0x0
#endif
  #define ILEVEL_MASK 0xFFFE0000
    #define ILEVEL_PCI_BIT (1<<30)
    #define ILEVEL_DSU_BIT (1<<27)
    #define ILEVEL_TIMER2_BIT (1<25)
    #define ILEVEL_TIMER1_BIT (1<<24)
    #define ILEVEL_IO3_BIT (1<<23)
    #define ILEVEL_IO2_BIT (1<<22)
    #define ILEVEL_IO1_BIT (1<<21)
    #define ILEVEL_IO0_BIT (1<<20)
    #define ILEVEL_UART1_BIT (1<<19)
    #define ILEVEL_UART2_BIT (1<<18)
    #define ILEVEL_AMBA_BIT (1<<17)
  #define IMASK_MASK 0x0000FFFE
    #define IMASK_PCI_BIT (1<<14)
    #define IMASK_DSU_BIT (1<<11)
    #define IMASK_TIMER2_BIT (1<<9)
    #define IMASK_TIMER1_BIT (1<<8)
    #define IMASK_IO3_BIT (1<<7)
    #define IMASK_IO2_BIT (1<<6)
    #define IMASK_IO1_BIT (1<<5)
    #define IMASK_IO0_BIT (1<<4)
    #define IMASK_UART1_BIT (1<<3)
    #define IMASK_UART2_BIT (1<<2)
    #define IMASK_AMBA_BIT (1<<1)

#define ITP_REG 0x4
  #define IPEND_MASK 0x0000FFFE
    #define IPEND_PCI_BIT (1<<14)
    #define IPEND_DSU_BIT (1<<11)
    #define IPEND_TIMER2_BIT (1<<9)
    #define IPEND_TIMER1_BIT (1<<8)
    #define IPEND_IO3_BIT (1<<7)
    #define IPEND_IO2_BIT (1<<6)
    #define IPEND_IO1_BIT (1<<5)
    #define IPEND_IO0_BIT (1<<4)
    #define IPEND_UART1_BIT (1<<3)
    #define IPEND_UART2_BIT (1<<2)
    #define IPEND_AMBA_BIT (1<<1)

#if defined(CONFIG_LEON3) || defined(CONFIG_LEON3FT)
#define ITF_REG 0x80
#else
#define ITF_REG 0x8
#endif
  #define IFORCE_MASK 0x0000FFFE
    #define IFORCE_PCI_BIT (1<<14)
    #define IFORCE_DSU_BIT (1<<11)
    #define IFORCE_TIMER2_BIT (1<<9)
    #define IFORCE_TIMER1_BIT (1<<8)
    #define IFORCE_IO3_BIT (1<<7)
    #define IFORCE_IO2_BIT (1<<6)
    #define IFORCE_IO1_BIT (1<<5)
    #define IFORCE_IO0_BIT (1<<4)
    #define IFORCE_UART1_BIT (1<<3)
    #define IFORCE_UART2_BIT (1<<2)
    #define IFORCE_AMBA_BIT (1<<1)

#define ITC_REG 0xC
  #define ICLEAR_MASK 0x0000FFFE
    #define ICLEAR_PCI_BIT (1<<14)
    #define ICLEAR_DSU_BIT (1<<11)
    #define ICLEAR_TIMER2_BIT (1<<9)
    #define ICLEAR_TIMER1_BIT (1<<8)
    #define ICLEAR_IO3_BIT (1<<7)
    #define ICLEAR_IO2_BIT (1<<6)
    #define ICLEAR_IO1_BIT (1<<5)
    #define ICLEAR_IO0_BIT (1<<4)
    #define ICLEAR_UART1_BIT (1<<3)
    #define ICLEAR_UART2_BIT (1<<2)
    #define ICLEAR_AMBA_BIT (1<<1)

#define ITS_REG 0x10
#define ITB_REG 0x14

static inline xm_u32_t GetPICCPUBase(void) {
    return LEON_PIC_BASE+__GetCpuHwId()*4;
}

xm_u32_t HwIrqGetMask(void) {
    return ~LoadIoReg(GetPICCPUBase()+ITMP_REG);
}

void HwIrqSetMask(xm_u32_t mask) {
    StoreIoReg(GetPICCPUBase()+ITMP_REG, (~mask)&IMASK_MASK);
}

xm_s32_t IsHwIrqPending(xm_s32_t irq) {
       return LoadIoReg(GetPICCPUBase()+ITP_REG)&(1<<irq);
}

void PicForceIrq(xm_u32_t irq) {
    StoreIoReg(GetPICCPUBase()+ITF_REG, (1<<irq));
}

void PicClearIrq(xm_u32_t irq) {
    StoreIoReg(LEON_PIC_BASE+ITC_REG, (1<<irq));
}

void PicEnableIrq(xm_u32_t irq) {
    StoreIoReg(GetPICCPUBase()+ITMP_REG, LoadIoReg(GetPICCPUBase()+ITMP_REG)|((1<<irq)&IMASK_MASK));
}

void PicDisableIrq(xm_u32_t irq) {
    StoreIoReg(GetPICCPUBase()+ITMP_REG, LoadIoReg(GetPICCPUBase()+ITMP_REG)&~((1<<irq)&IMASK_MASK));
}

#ifdef CONFIG_SMP_SUPPORT
xm_u8_t SparcGetNoCpus(void) {
    return (LoadIoReg(GetPICCPUBase()+ITS_REG)>>28)+1;
}

void WakeUpCpu(xm_s32_t cpu) {
    StoreIoReg(GetPICCPUBase()+ITS_REG, 1<<cpu);
}

void PicForceIrqCpu(xm_u8_t dst, xm_u32_t irq) {
    StoreIoReg((LEON_PIC_BASE+dst*4)+ITF_REG, (1<<irq));
}

void PicBroadcastIrq(xm_u32_t irq) {
    xm_s32_t e;
    for (e=0; e<CONFIG_NO_CPUS; e++) 
        if (e!=GET_CPU_ID())
            PicForceIrqCpu(e, irq);
}
#endif

void InitPic(void) {
    xm_s32_t irq;

    // Masking all HW IRQs, all irqs have the same priority level (0)
    StoreIoReg(GetPICCPUBase()+ITMP_REG, 0);

    for (irq=0; irq<CONFIG_NO_HWIRQS; irq++) {
        hwIrqCtrl[irq].Enable=PicEnableIrq;        
        hwIrqCtrl[irq].Disable=PicDisableIrq;
        hwIrqCtrl[irq].Ack=PicDisableIrq;
        hwIrqCtrl[irq].End=PicEnableIrq;
        hwIrqCtrl[irq].Force=PicForceIrq;
        hwIrqCtrl[irq].Clear=PicClearIrq;
    }
}

#endif
