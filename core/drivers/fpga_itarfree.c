/*
 * $FILE: fpga_itarfree.c
 *
 *
 * $VERSION$
 *
 * Author: Javier O. Coronel <jacopa@ai2.upv.es>
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

/*
  Changelog
  - [02/09/2015:SPR-020915-02] 'continue' replaced with equivalent code
*/

#include <kdevice.h>
#ifdef CONFIG_DEV_FPGA_ITARFREE
#include <drivers/fpga_itarfree.h>
#include <irqs.h>
#include <sched.h>
#include <spinlock.h>
#include <arch/processor.h>
#include <arch/xm_def.h>
#include <objects/hm.h>

#define MASK_ALL_FPGAIRQ 	0x00007fff

/*Interruption used in the processor to receive interrupts from the FPGA*/
#define FPGA_CTRL_IT_PIRQ 	IO_IRQ0_TRAP_NR

#define IODIR_REG  0x4
#define MASK_PIOIN_FPGA_TO_CPU	      ~(1<<PIO_FPGA_TO_CPU)
#define MASK_PIOOUT_FPGA_TO_CPU       (1<<PIO_FPGA_TO_CPU)

#define IOT1_REG   0x8
    #define IOT1_PL0_BIT		5
    #define IOT1_LE0_BIT		6
    #define IOT1_EN0_BIT		7
    #define PL0_HIGH_POSITVE_EDGE	1<<IOT1_PL0_BIT
    #define PL0_LOW_NEGATIVE_EDGE	0<<IOT1_PL0_BIT
    #define LE0_EDGE_TRIGGERED		1<<IOT1_LE0_BIT
    #define LE0_LEVEL_SENSITIVE		0<<IOT1_LE0_BIT
    #define EN0_ACTIVE			1<<IOT1_EN0_BIT

#define ITC_REG 0xC

RESERVE_IOPORTS(FPGA_CTRL_IT+FPGA_IT_MASK, 3);
RESERVE_IOPORTS(LEON_GPIO_BASE|IODIR_REG, 2);
RESERVE_HWIRQ(FPGA_CTRL_IT_PIRQ);
RESERVE_HWIRQ(IT_SDRAM+CPU_NO_HWIRQS);
RESERVE_HWIRQ(IT_EADR+CPU_NO_HWIRQS);

//#define DEBUG_FPGA_ITARFREE  1

#if defined(CONFIG_DEV_LICE_INTERFACE) && defined(DEBUG_FPGA_ITARFREE)

#include <devices/lice_interface.h>

#define LICE_CODE_ENABLE_IRQ	0x0d
#define LICE_CODE_DISABLE_IRQ	0x0e
#define LICE_CODE_IRQ_HANDLER	0x0f
#define LICE_CODE_IRQ_FPGASTATE	0x10
#define LICE_CODE_IRQ_MASK	0x11
#define LICE_CODE_INIT_FPGAPIC1	0x12
#define LICE_CODE_INIT_FPGAPIC2	0x13

#define ADDR_OFFSET_ENABLE_IRQ		(LICE_CODE_ENABLE_IRQ*4)
#define ADDR_OFFSET_DISABLE_IRQ		(LICE_CODE_DISABLE_IRQ*4)
#define ADDR_OFFSET_IRQ_HANDLER		(LICE_CODE_IRQ_HANDLER*4)
#define ADDR_OFFSET_IRQ_FPGASTATE	(LICE_CODE_IRQ_FPGASTATE*4)
#define ADDR_OFFSET_IRQ_MASK		(LICE_CODE_IRQ_MASK*4)
#define ADDR_OFFSET_INIT_FPGAPIC1	(LICE_CODE_INIT_FPGAPIC1*4)
#define ADDR_OFFSET_INIT_FPGAPIC2	(LICE_CODE_INIT_FPGAPIC2*4)

#endif


volatile xm_u32_t fpga_it_mask=0;

static void FpgaItarFreePicEnableIrq(xm_u32_t irq){
    StoreIoReg(FPGA_CTRL_IT+FPGA_IT_MASK, LoadIoReg(FPGA_CTRL_IT+FPGA_IT_MASK)|(1<<(irq-CPU_NO_HWIRQS)));
    fpga_it_mask|=(1<<(irq-CPU_NO_HWIRQS));
#if defined(CONFIG_DEV_LICE_INTERFACE) && defined(DEBUG_FPGA_ITARFREE)
SendDataToLiceInterface(irq,LICE_BASE_ADDRESS+ADDR_OFFSET_ENABLE_IRQ);
#endif
}

static void FpgaItarFreePicDisableIrq(xm_u32_t irq){
    StoreIoReg(FPGA_CTRL_IT+FPGA_IT_MASK, LoadIoReg(FPGA_CTRL_IT+FPGA_IT_MASK)&~(1<<(irq-CPU_NO_HWIRQS)));
    fpga_it_mask&= ~(1<<(irq-CPU_NO_HWIRQS));
#if defined(CONFIG_DEV_LICE_INTERFACE) && defined(DEBUG_FPGA_ITARFREE)
SendDataToLiceInterface(irq,LICE_BASE_ADDRESS+ADDR_OFFSET_DISABLE_IRQ);
#endif
}

xm_s32_t irqTab2FpgaHwIrq[ITAR_FREE_NO_HWIRQS]={
	FPGA_INTERRUPT_LEVEL_1,   // SDRAM // 16
	FPGA_INTERRUPT_LEVEL_2,   // IT_EADR // 17
	FPGA_INTERRUPT_LEVEL_3,   // IT_1HZ // 18
	FPGA_INTERRUPT_LEVEL_4,   // IT_32HZ // 19
	FPGA_INTERRUPT_LEVEL_5,  // IT_IO0 // 20
	FPGA_INTERRUPT_LEVEL_6,   // IT_IO1 // 21
	FPGA_INTERRUPT_LEVEL_7,   // IT_IO2 // 22
	FPGA_INTERRUPT_LEVEL_8,   // IT_IO3 // 23
	FPGA_INTERRUPT_LEVEL_9,   // IT_UART_TX // 24
	FPGA_INTERRUPT_LEVEL_10,  // IT_UART_RX // 25
	FPGA_INTERRUPT_LEVEL_11,  // IT_IOTOP // 26
	FPGA_INTERRUPT_LEVEL_12,  // IT_OSLNK_RX // 27
	FPGA_INTERRUPT_LEVEL_13,  // IT_OSLNK_TX // 28
	FPGA_INTERRUPT_LEVEL_14,  // IT_SPW_RX // 29
	FPGA_INTERRUPT_LEVEL_15  // IT_SPW_TX // 30
};

extern xm_s32_t irq2Vector[CONFIG_NO_HWIRQS];

static xm_s32_t FpgaItarFreeInitPic(void){
    int irq;

#if defined(CONFIG_DEV_LICE_INTERFACE) && defined(DEBUG_FPGA_ITARFREE)
SendDataToLiceInterface(LoadIoReg(FPGA_CTRL_IT+FPGA_IT_MASK)<<16|LoadIoReg(FPGA_CTRL_IT+FPGA_IT_PEND),LICE_BASE_ADDRESS+ADDR_OFFSET_INIT_FPGAPIC1); 
#endif
/*Configuration of I/O pins*/
    StoreIoReg(LEON_GPIO_BASE+IODIR_REG, MASK_PIOIN_FPGA_TO_CPU & LoadIoReg(LEON_GPIO_BASE+IODIR_REG));
    StoreIoReg(LEON_GPIO_BASE+IOT1_REG, EN0_ACTIVE|LE0_EDGE_TRIGGERED|PL0_HIGH_POSITVE_EDGE|PIO_FPGA_TO_CPU);

/*All Fpga interruptions are masked*/
    StoreIoReg(FPGA_CTRL_IT+FPGA_IT_MASK, LoadIoReg(FPGA_CTRL_IT+FPGA_IT_MASK)&(~MASK_ALL_FPGAIRQ));

/*Interruption in FPGA are cleared - the interrupts are cleared due to a problem with IT_SDRAM interrupt in the board*/
    StoreIoReg(FPGA_CTRL_IT+FPGA_IT_CLEAR, MASK_ALL_FPGAIRQ);
    //StoreIoReg(FPGA_CTRL_IT+FPGA_IT_CLEAR, MASK_ALL_FPGAIRQ);


    for (irq=CPU_NO_HWIRQS; irq<CONFIG_NO_HWIRQS; irq++) {
	hwIrqCtrl[irq]=(hwIrqCtrl_t){
	    .Enable=FpgaItarFreePicEnableIrq,
	    .Disable=FpgaItarFreePicDisableIrq,
	    .Ack=FpgaItarFreePicDisableIrq,
	    .End=FpgaItarFreePicEnableIrq,
	};
        irq2Vector[irq]=irqTab2FpgaHwIrq[irq-CPU_NO_HWIRQS];  /*The table irq2Vector is completed */
    }
#if defined(CONFIG_DEV_LICE_INTERFACE) && defined(DEBUG_FPGA_ITARFREE)
SendDataToLiceInterface(LoadIoReg(FPGA_CTRL_IT+FPGA_IT_MASK)<<16|LoadIoReg(FPGA_CTRL_IT+FPGA_IT_PEND),LICE_BASE_ADDRESS+ADDR_OFFSET_INIT_FPGAPIC2); 
#endif
}

// static xm_s32_t FpgaItarFreeShutdownPic(void){
//     StoreIoReg(LEON_GPIO_BASE+IOT1_REG, ~EN0_ACTIVE & LoadIoReg(LEON_GPIO_BASE+IOT1_REG));
// }

static void FpgaRaiseEvent(cpuCtxt_t *ctxt, xm_u8_t id) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xm_u32_t payload;
    xm_u32_t partitionId;
    xmHmLog_t log;

    if (IsSvIrqCtxt(ctxt)) {
	if (info->sched.cKThread->ctrl.g)
	    partitionId=GetPartitionCfg(info->sched.cKThread)->id;
	else
	    partitionId=XM_HYPERVISOR_ID;
    } else {
	partitionId=GetPartitionCfg(info->sched.cKThread)->id;
    }
    payload=GET_CTXT_PC(ctxt);

    memset(&log, 0, sizeof(xmHmLog_t));

    log.opCode = 0x00000000;
    log.opCode |= partitionId<<HMLOG_OPCODE_PARTID_BIT; //Partition ID
    log.opCode |= ((xm_u32_t)0x1)<<HMLOG_OPCODE_VALID_CPUCTXT_BIT; //Valid CPU contxt set
    log.opCode |= ((xm_u32_t)id)<<HMLOG_OPCODE_EVENT_BIT; //Event Id

    log.cpuCtxt.pc = (xm_u32_t)ctxt->pc;
    log.cpuCtxt.nPc = (xm_u32_t)ctxt->nPc;
    log.cpuCtxt.psr = (xm_u32_t)ctxt->psr;

    log.payload[0] = (xmWord_t)payload;

    HmRaiseEvent(&log);
}

volatile int aux=0;

static void FpgaPicIrqHandler(cpuCtxt_t *ctxt, void *irqData) {
    cpuCtxt_t l_ctxt=*ctxt;
    int irq;
    xm_u32_t ctrl_it;


    /*Interrupts pending*/
    ctrl_it=LoadIoReg(FPGA_CTRL_IT+FPGA_IT_PEND);

#if defined(CONFIG_DEV_LICE_INTERFACE) && defined(DEBUG_FPGA_ITARFREE)
    SendDataToLiceInterface(fpga_it_mask<<16|ctrl_it,LICE_BASE_ADDRESS+ADDR_OFFSET_IRQ_FPGASTATE);
#endif
    for (irq=0;irq<ITAR_FREE_NO_HWIRQS;irq++)
    {
	if ((ctrl_it & (1<<irq)) & fpga_it_mask)
	{

	    /*XM_HM_EV_FPGA_ERR_SDRAM and XM_HM_EV_FPGA_EADR events are generated */
	    if (irq<=IT_EADR){
// This is commented because I assume that the board doesn't have spurious interrupts.
//		if (aux)
		   FpgaRaiseEvent(ctxt,XM_HM_EV_FPGA_ERR_SDRAM+irq);
//		else
//		   aux=1;
	    } else {                
//	    FpgaItarFreePicDisableIrq(irq+CPU_NO_HWIRQS);  /*Disable interrupt - uncomment if XM mask the interrupts by default*/

                l_ctxt.irqNr=irq+CPU_NO_HWIRQS;
                if (irqHandlerTab[l_ctxt.irqNr].handler)
                    (*(irqHandlerTab[l_ctxt.irqNr].handler))(&l_ctxt, irqHandlerTab[l_ctxt.irqNr].data);
            }
	}
    }

    /*Interrupts clear*/
    StoreIoReg(FPGA_CTRL_IT+FPGA_IT_CLEAR,(fpga_it_mask&ctrl_it));
    // StoreIoReg(FPGA_CTRL_IT+FPGA_IT_CLEAR,(fpga_it_mask&ctrl_it));

    StoreIoReg(LEON_PIC_BASE+ITC_REG,1<<FPGA_CTRL_IT_PIRQ); /*Clear the I/0 iterrupt in the processor*/
//    HwEnableIrq(FPGA_CTRL_IT_PIRQ); /*Enable the interrupt in the processor - uncomment if XM mask the interrupts by default*/

#if defined(CONFIG_DEV_LICE_INTERFACE) && defined(DEBUG_FPGA_ITARFREE)
    SendDataToLiceInterface(ctrl_it<<16|LoadIoReg(FPGA_CTRL_IT+FPGA_IT_PEND),LICE_BASE_ADDRESS+ADDR_OFFSET_IRQ_HANDLER);
#endif

}

static kDevice_t fpgaItarFreeReg={
    .subId=0,
};

static const kDevice_t *GetFpgaItarFree(xm_u32_t subId) {
    return &fpgaItarFreeReg;
}

xm_s32_t InitFpgaItarFree(void) {
    eprintf("Init FPGA-CPUItarFree\n");
    FpgaItarFreeInitPic();
    SetIrqHandler(FPGA_CTRL_IT_PIRQ, FpgaPicIrqHandler, 0);

/*These events were commented due to hardware problems */
//    FpgaItarFreePicEnableIrq(IT_SDRAM+CPU_NO_HWIRQS);
//    FpgaItarFreePicEnableIrq(IT_EADR+CPU_NO_HWIRQS); 

    StoreIoReg(LEON_PIC_BASE+ITC_REG,1<<FPGA_CTRL_IT_PIRQ); /*Clear the I/0 iterrupt in the processor*/
    HwEnableIrq(FPGA_CTRL_IT_PIRQ); /*Enable the interrupt in the processor*/

    GetKDevTab[XM_DEV_FPGA_ITARFREE_ID]=GetFpgaItarFree;

    return 0;
}

// static void ShutdownFpgaItarFree(void) {
//     FpgaItarFreeShutdownPic();
//     HwDisableIrq(FPGA_CTRL_IT_PIRQ);
// }

REGISTER_KDEV_SETUP(InitFpgaItarFree);
#endif

