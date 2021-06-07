/*
 * $FILE: spw_irq.c
 *
 *
 * $VERSION$
 *
 * Author: María Soler Heredia <msolher@ai2.upv.es> 
 *         Javier O. Coronel <jacopa@ai2.upv.es>
 *
 * $LICENSE:
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <kdevice.h>
#ifdef CONFIG_SPW_RTC
#include <drivers/spw_irq.h>
#include <irqs.h>
#include <sched.h>
#include <arch/guest.h>
#include <spinlock.h>
#include <arch/processor.h>
#include <arch/xm_def.h>
#include <objects/hm.h>

#define MASK_ALL_IT2_IRQ 	0xFFFFFFFF
#define MASK_SPW_IFX_IRQ 	0xFFFFFFFF        /*Interrupts of SPW interface X (0 or 1)*/

#define ITMP_REG 0x0
#define ITP_REG 0x4
#define ITC_REG 0xC
#define IMASK_MASK 0x0000FFFE

#define SPW_BASE0		0x80060000
#define SPW_BASE1               0x80070000
#define SPW_MASK		0x10
#define SPW_CLEAR		0x1c
#define SPW_PENDING_LOW		0x14
#define SPW_PENDING_HIGH	0x08

/*Interruption used in the processor to receive interrupts from the IT2*/
#define SPW_CTRL_IT_PIRQ 	XM_VT_HW_IO_IRQ4_TRAP_NR  /*Line 10*/

#define SPW_IRQ0 	XM_VT_HW_IO_IRQ6_TRAP_NR	/*Line 13*/
#define SPW_IRQ1        XM_VT_HW_PCI_TRAP_NR		/*Line 14*/

#ifdef CONFIG_PLAN_EXTSYNC
RESERVE_HWIRQ(SPW_IRQ0);
RESERVE_HWIRQ(SPW_IRQ1);
#endif
RESERVE_HWIRQ(XM_VT_HW_IO_IRQ4_TRAP_NR);
RESERVE_IOPORTS(RTC_CTRL_IT2, 3);


static void SpwIrq2Handler(cpuCtxt_t *ctxt, void *irqData);

//static spinLock_t spw_picLock=SPINLOCK_INIT;
volatile xm_u32_t spw_it_mask=0;
volatile xmAddress_t SPW_LINK_IFX=0;  /*Address of the SPW link 0 or 1 (X)*/

static void SpwPicEnableIrq(xm_u32_t irq){
    StoreIoReg(RTC_CTRL_IT2+RTC_IT2_MASK, LoadIoReg(RTC_CTRL_IT2+RTC_IT2_MASK)|(1<<(irq-CPU_NO_HWIRQS)));
    spw_it_mask|=(1<<(irq-CPU_NO_HWIRQS));
}

static void SpwPicDisableIrq(xm_u32_t irq){
    StoreIoReg(RTC_CTRL_IT2+RTC_IT2_MASK, LoadIoReg(RTC_CTRL_IT2+RTC_IT2_MASK)&~(1<<(irq-CPU_NO_HWIRQS)));
    spw_it_mask&= ~(1<<(irq-CPU_NO_HWIRQS));
}

#ifdef CONFIG_PLAN_EXTSYNC

static void SpwPicEnableIrqSpwIT(xm_u32_t irq){
    StoreIoReg(LEON_PIC_BASE+ITC_REG,1<<irq); /*Clear the SPW iterrupt in the processor-IT. It should not be necessary but it is performed in order to ensure that the interrupt is cleared*/
    StoreIoReg(LEON_PIC_BASE+ITMP_REG, LoadIoReg(LEON_PIC_BASE+ITMP_REG)|((1<<irq)&IMASK_MASK));  /*Unmask SPW interrupt in the IT*/
}

static void SpwPicDisableIrqSpwIT(xm_u32_t irq){
    StoreIoReg(LEON_PIC_BASE+ITMP_REG, LoadIoReg(LEON_PIC_BASE+ITMP_REG)&~((1<<irq)&IMASK_MASK)); /*Mask SPW interrupt in the IT*/
    StoreIoReg(SPW_LINK_IFX+SPW_LISCR,SPW_RX_TIME_CODE_BIT); /*Clear RX_TIME_CODE*/
}
#endif

xm_s32_t irqTab2SpwHwIrq[RTC_SPW_NO_HWIRQS]={
	SPW_INTERRUPT_LEVEL_1,
	SPW_INTERRUPT_LEVEL_2,
	SPW_INTERRUPT_LEVEL_3,
	SPW_INTERRUPT_LEVEL_4,
	SPW_INTERRUPT_LEVEL_5,
	SPW_INTERRUPT_LEVEL_6,
	SPW_INTERRUPT_LEVEL_7,
	SPW_INTERRUPT_LEVEL_8,
	SPW_INTERRUPT_LEVEL_9,
	SPW_INTERRUPT_LEVEL_10,
	SPW_INTERRUPT_LEVEL_11,
	SPW_INTERRUPT_LEVEL_12,
	SPW_INTERRUPT_LEVEL_13,
	SPW_INTERRUPT_LEVEL_14, // IT2_SPW0 // 29
	SPW_INTERRUPT_LEVEL_15,  
	SPW_INTERRUPT_LEVEL_16  // IT2_SPW1 // 31
};

extern xm_s32_t irq2Vector[CONFIG_NO_HWIRQS];

int irqExtSync=-1;

#ifdef CONFIG_PLAN_EXTSYNC
static void InitSpwIf(void) {
  xm_u32_t pendif;
  pendif=LoadIoReg(SPW_LINK_IFX+SPW_PISR); /*Read pending interrupts*/
  pendif=LoadIoReg(SPW_LINK_IFX+SPW_LISR); /*Read link interrupts status register*/
  StoreIoReg(SPW_LINK_IFX+SPW_LISCR,MASK_SPW_IFX_IRQ); /*Clear spurious interrpts*/
  StoreIoReg(SPW_LINK_IFX+SPW_IMR,0); /*Mask all interrupts*/
}
#endif

static xm_s32_t InitSpwIT2(void) {
  int irq;
  eprintf("Init SPW-RTC IT2-CTRL\n");
  /*Configuration of I/O pins*/
  
  /*All interruptions Spw-IT2 are masked*/
  StoreIoReg(RTC_CTRL_IT2+RTC_IT2_MASK,~MASK_ALL_IT2_IRQ);
  
  /*Interruption in SPW-IT2 are cleared*/
  StoreIoReg(RTC_CTRL_IT2+RTC_IT2_CLEAR, MASK_ALL_IT2_IRQ);


    for (irq=CPU_NO_HWIRQS; irq<CONFIG_NO_HWIRQS; irq++) {
	hwIrqCtrl[irq]=(hwIrqCtrl_t){
	    .Enable=SpwPicEnableIrq,
	    .Disable=SpwPicDisableIrq,
	    .Ack=SpwPicDisableIrq,
	    .End=SpwPicEnableIrq,
	};
        irq2Vector[irq]=irqTab2SpwHwIrq[irq-CPU_NO_HWIRQS];  /*The table irq2Vector is completed */
    }
    
#ifdef CONFIG_PLAN_EXTSYNC
  irqExtSync=xmcSchedCyclicPlanTab[0].extSync;
  if ((irqExtSync==29)||(irqExtSync==13)){
    SPW_LINK_IFX=SPW_LINK_IF0;
  }
  else if ((irqExtSync==31)||(irqExtSync==14)){
    SPW_LINK_IFX=SPW_LINK_IF1;
  }
  
  if (SPW_LINK_IFX)
     InitSpwIf();
  
  if ((irqExtSync==29)||(irqExtSync==31)){
     StoreIoReg(SPW_LINK_IFX+SPW_IMR,LoadIoReg(SPW_LINK_IFX+SPW_IMR)|(SPW_RX_TIME_CODE_BIT)); /*Unmask source external syncronization RX_TIME_CODE*/
  }

  if ((irqExtSync==13)||(irqExtSync==14)){
     StoreIoReg(SPW_LINK_IFX+SPW_IMR,LoadIoReg(SPW_LINK_IFX+SPW_IMR)|(SPW_RX_TIME_CODE_BIT)); /*Unmask source external syncronization RX_TIME_CODE*/
    	hwIrqCtrl[irqExtSync]=(hwIrqCtrl_t){
	    .Enable=SpwPicEnableIrqSpwIT,
	    .Disable=SpwPicDisableIrqSpwIT,
	    .Ack=SpwPicDisableIrqSpwIT,
	    .End=SpwPicEnableIrqSpwIT,
	};
  }
#endif
  
  return 0;
}

static kDevice_t spwReg={
    .subId=0,
};

static const kDevice_t *GetSpw(xm_u32_t subId) {
    return &spwReg;
}

xm_s32_t SpwInit(void){
    InitSpwIT2();
    
    SetIrqHandler(SPW_CTRL_IT_PIRQ, SpwIrq2Handler, 0);
    
    StoreIoReg(LEON_PIC_BASE+ITC_REG,1<<SPW_CTRL_IT_PIRQ); /*Clear the I/O iterrupt in the processor*/

#ifndef CONFIG_PLAN_EXTSYNC
    HwEnableIrq(SPW_CTRL_IT_PIRQ); /*Enable the interrupt in the processor*/
#endif
    GetKDevTab[XM_DEV_SPW_ID]=GetSpw;
    return 0;
}

// static void SpwRaiseEvent(cpuCtxt_t *ctxt, xm_u8_t id) {
//     localSched_t *sched=GET_LOCAL_SCHED();
//     xm_u32_t payload;
//     xm_u32_t sId=sched->cKThread->ctrl.g->cfg->id;
//     
//     if (IsSvIrqCtxt(ctxt))
// 	if (sched->cKThread->ctrl.g==NULL)
// 	    sId=XM_HYPERVISOR_ID;
//     payload=GET_CTXT_PC(ctxt);
//     HmRaiseEvent(sId, id, payload);
// }



static void SpwIrq2Handler(cpuCtxt_t *ctxt, void *irqData) {
    cpuCtxt_t l_ctxt=*ctxt;
    xm_u32_t ctrl_it;

    /*Interrupts pending in IT2*/
    ctrl_it=LoadIoReg(RTC_CTRL_IT2+RTC_IT2_PEND);

#ifdef CONFIG_PLAN_EXTSYNC
      if (ctrl_it & IT2_PEND_SPW0_BIT & spw_it_mask){
	 LoadIoReg(SPW_LINK_IFX+SPW_PISR); /*Read pending interrupts*/
         l_ctxt.irqNr=RTC_IT2_SPW0_IRQ+CPU_NO_HWIRQS;
         if (irqHandlerTab[l_ctxt.irqNr].handler)
	    (*(irqHandlerTab[l_ctxt.irqNr].handler))(&l_ctxt, irqHandlerTab[l_ctxt.irqNr].data);
 	 StoreIoReg(SPW_LINK_IFX+SPW_LISCR,SPW_RX_TIME_CODE_BIT); /*Clear RX_TIME_CODE*/
      }
      else if (ctrl_it & IT2_PEND_SPW1_BIT & spw_it_mask){
         l_ctxt.irqNr=RTC_IT2_SPW1_IRQ+CPU_NO_HWIRQS;
         if (irqHandlerTab[l_ctxt.irqNr].handler)
	    (*(irqHandlerTab[l_ctxt.irqNr].handler))(&l_ctxt, irqHandlerTab[l_ctxt.irqNr].data);
 	 StoreIoReg(SPW_LINK_IFX+SPW_LISCR,SPW_RX_TIME_CODE_BIT); /*Clear RX_TIME_CODE*/
      }
#else
    int irq;
    for (irq=0;irq<RTC_SPW_NO_HWIRQS;irq++){
	if ((ctrl_it & (1<<irq)) & spw_it_mask){
            l_ctxt.irqNr=irq+CPU_NO_HWIRQS;
            if (irqHandlerTab[l_ctxt.irqNr].handler)
	    	(*(irqHandlerTab[l_ctxt.irqNr].handler))(&l_ctxt, irqHandlerTab[l_ctxt.irqNr].data);
	}
    }

#endif

    StoreIoReg(RTC_CTRL_IT2+RTC_IT2_CLEAR,(spw_it_mask&ctrl_it)); /*Interrupts clear in IT2*/
    StoreIoReg(LEON_PIC_BASE+ITC_REG,1<<SPW_CTRL_IT_PIRQ); /*Clear the I/O iterrupt in the processor*/
    
}

// static void ShutdownSpw(void) {
//     HwDisableIrq(SPW_CTRL_IT_PIRQ); /*Disable the interrupt in the processor*/
//     StoreIoReg(RTC_CTRL_IT2+RTC_IT2_MASK,~MASK_ALL_IT2_IRQ); /*All interruptions Spw-IT2 are masked*/
//     if (SPW_LINK_IFX)
//        StoreIoReg(SPW_LINK_IFX+SPW_IMR,0); /*Mask all interrupts*/
// }

REGISTER_KDEV_SETUP(SpwInit);

#endif
