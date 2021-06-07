/*
 * $FILE: mil-std.c
 *
 *
 * $VERSION$
 *
 * Author: Javier O. Coronel <jcoronel@fentiss.com>
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
  Changelog:
  - [05/08/2015:SPR-010915-01] 1553 driver reimplemented.
  - [14/03/2016:CP-080316-01 AE] Apply patches proposed by CNES: xm-q-FT-CS-207_01.patch and xm-q-FT-CS-207_02.patch
  - [28/04/2016:CP-080316-01 JC] Remove previous patches applied: xm-q-FT-CS-207_01.patch and xm-q-FT-CS-207_02.patch
  - [28/04/2016:CP-080316-01 JC] Apply patches proposed by CNES: xm-q-FT-CS-254_01.patch and xm-q-FT-CS-249_01.patch
  - [06/05/2016:CP-080316-01 JC] Apply updating of a previous patch propused by CNES: xm-q-FT-CS-254_01.patch (sent 03-05-16)
  - [06/05/2016:CP-080316-01 JC] Add new configuration options for the device MIL-1553: parity and clock source - it is related 
                                 to patches xm-q-FT-CS-207_01.patch and xm-q-FT-CS-207_02.patch
*/

#include <kdevice.h>
#include <drivers/mil_std.h>
#include <arch/leon.h>
#include <irqs.h>
#include <sched.h>

#define MIL_STD_1553_IRQ        XM_VT_HW_PCI_TRAP_NR		/*Line 14 on LEON3 Processor*/
#define CORE1553BRM_EXT_SYNC	CORE1553BRM_IRQ_SUBAD        /*Irq 10 on core 1553BRM - SUBAD - Subaddress Accessed Interrupt*/

RESERVE_HWIRQ(MIL_STD_1553_IRQ);

/*LEON*/
RESERVE_IOPORTS(LEON_GRGPREG_BASE,1);
RESERVE_IOPORTS(LEON_CLKGATE_BASE,3);
/*LEON MIL-STD-1553*/
RESERVE_IOPORTS(MIL_STD_STATUS_CTRL,1);
RESERVE_IOPORTS(MIL_STD_IT_SETTING,1);
RESERVE_IOPORTS(MIL_STD_AHB_PAGE_ADR,1);

/*mil-std*/
RESERVE_IOPORTS(MIL_STD_BASE+CORE1553BRM_CTRL, 1);
RESERVE_IOPORTS(MIL_STD_BASE+CORE1553BRM_OP_STATUS, 1);
RESERVE_IOPORTS(MIL_STD_BASE+CORE1553BRM_IT_MASK, 1);  /*Reserve Interrupt Mask and Pending Interrupt registers*/
RESERVE_IOPORTS(MIL_STD_BASE+CORE1553BRM_IT_PEND, 1);
//RESERVE_IOPORTS(MIL_STD_BASE+CORE1553BRM_IT_PTR, 1);
RESERVE_IOPORTS(MIL_STD_BASE+CORE1553BRM_DESC_PTR, 1);
//RESERVE_IOPORTS(MIL_STD_BASE+CORE1553BRM_STATUS_WORD, 1);
RESERVE_IOPORTS(MIL_STD_BASE+CORE1553BRM_ENH_FEAT, 1);

desc1553Dev_t * g1553Dev=NULL;

static void MilStdEnableIrq(xm_u32_t irq)
{
    PicClearIrq(irq); /*Clear the I/0 iterrupt in the processor*/
    StoreIoReg(MIL_STD_BASE + CORE1553BRM_IT_MASK, CORE1553BRM_EXT_SYNC); /*Unmask ExtSync interrupt on Core 1553BRM*/
    PicEnableIrq(irq); /*Enable the interrupt in the processor*/
}

static void MilStdDisableIrq(xm_u32_t irq){
    StoreIoReg(MIL_STD_BASE+CORE1553BRM_IT_MASK,0x0); /*Mask all Core1553BRM interrupts*/
    PicDisableIrq(irq); /*Disable the interrupt in the processor*/
}

static xm_s32_t MilStdReadModeCode(xm_u16_t nModeCode, xmAddress_t *pData,
        xm_u32_t * nBytes)
{
    xm_s32_t rtn = 0;
    xm_u16_t miw = 0;
    xm_u16_t tmpCtrlWord = 0;
    xm_u8_t status = 0;

    *pData = 0;
    *nBytes = 0;
    if (((xm_u16_t)(ReadByPassMmuHalfWord((void *)&g1553Dev->descModeCodeRx[nModeCode].controlWord)) & C_DBCW_BAC)
            == C_DBCW_BAC)
    {
        /* Read the Message Information Word  */
        miw =  ReadByPassMmuHalfWord((void *)&g1553Dev->rxModeCodeMsgs[nModeCode].msgs[0]);

        /* Extract data from the MIW  */
        status = (xm_u8_t) (miw & 0x009F);

        /* Test the message error bit */
        if ((status & C_MIW_ME) == C_MIW_ME)
        {
            rtn = -1;
        }
        else
        {
            *pData = (xmAddress_t) (&g1553Dev->rxModeCodeMsgs[nModeCode].msgs[4]);
            *nBytes = 2;
        }
        /* Reset BAC bit */
        tmpCtrlWord= ReadByPassMmuHalfWord((void *)&g1553Dev->descModeCodeRx[nModeCode].controlWord) & ~C_DBCW_BAC;
        WriteByPassMmuHalfWord((void *)&g1553Dev->descModeCodeRx[nModeCode].controlWord,  tmpCtrlWord);
    }
    return rtn;
}

static void MilStdIrqHandler(cpuCtxt_t *ctxt, void *irqData) {
    xm_s32_t nIrqLog = 0;
    xm_u16_t iiw = 0;
    nIrqLog = (LoadIoReg(MIL_STD_BASE + CORE1553BRM_IT_PTR) & 0x1f) >> 1;
    if (nIrqLog == 0)
    {
        nIrqLog = 16;
    }
    nIrqLog--;
//    eprintf("nIrqLod=%d ",nIrqLog);
    iiw=ReadByPassMmuHalfWord((void *)&g1553Dev->irqLogs[nIrqLog].iiw);
    if (iiw & CORE1553BRM_EXT_SYNC)
    {
        xm_u16_t iaw = ReadByPassMmuHalfWord((void *)&g1553Dev->irqLogs[nIrqLog].iaw);
        xm_s32_t nModeCodeMsg = 0;
//        eprintf("MC=0x%x-iaw=%d\n",(iaw & IAW_MODE_CODE),(iaw>>2));
        if (iaw & IAW_MODE_CODE)
        {
            xmAddress_t pData;
            xm_u32_t nBytesRead;
            xm_s32_t rtn;
            nModeCodeMsg = ((iaw >> 2)&0x1F);
            if (nModeCodeMsg == SYNC_MODE_CODE)
            {
                rtn = MilStdReadModeCode(nModeCodeMsg, &pData, &nBytesRead);
                if ((!rtn) && (nBytesRead > 0))
                {
//                    eprintf("data=0x%x\n",ReadByPassMmuHalfWord((void *)pData));
                    if (ReadByPassMmuHalfWord((void *)pData) == 0)
                    {
                        SchedSyncHandler(ctxt, irqData);
                        return;
                    }
                }
            }
        }
    }
    HwEnableIrq(MIL_STD_1553_IRQ);

//    xm_u32_t ctrl_it;
//    xm_u32_t tmp;
//	/*MIL Interrupts pending*/
//    ctrl_it=LoadIoReg(MIL_STD_BASE+CORE1553BRM_IT_PEND);
//    eprintf("Irq MIL-STD Handler=0x%x\n",ctrl_it);
//    tmp=LoadIoReg(MIL_STD_BASE+CORE1553BRM_OP_STATUS);
//    //tmp=LoadIoReg(0xFFF00004);
//    eprintf("Register op_status=0x%lx\n",0xffff&tmp);
//    tmp=LoadIoReg(MIL_STD_BASE+CORE1553BRM_IT_MASK);
//    eprintf("CORE_MASK=0x%x\n", (0xffff)&(LoadIoReg(MIL_STD_BASE+CORE1553BRM_IT_MASK)));
//    StoreIoReg(MIL_STD_BASE+CORE1553BRM_IT_MASK,0x0); /*Mask all interrupt on Core 1553BRM*/

//	/*Clear interrupt on the device*/
//    if ((ctrl_it&0xF000)&&(ctrl_it&0x0FBF))
//       StoreIoReg(MIL_STD_IT_SETTING,MIL_STD_IT_SETTING_INTLEVEL); /*Hardware and Message interrupt acknowledge. ACKs to 0
//    else if (ctrl_it&0xF000)
//       StoreIoReg(MIL_STD_IT_SETTING,MIL_STD_IT_SETTING_INTACKM|MIL_STD_IT_SETTING_INTLEVEL); /*Hardware interrupt acknowledge. INTACKH to 0*/
//    else if (ctrl_it&0x0FBF)
//       StoreIoReg(MIL_STD_IT_SETTING,MIL_STD_IT_SETTING_INTACKH|MIL_STD_IT_SETTING_INTLEVEL); /*Message interrupt acknowledge. INTACKM to 0*/
//
//    StoreIoReg(MIL_STD_IT_SETTING,MIL_STD_IT_SETTING_INTACKH|MIL_STD_IT_SETTING_INTACKM|MIL_STD_IT_SETTING_INTLEVEL); /*INTACKH/INTACKM to 1*/


//	/*Handle interrupts*/
//    if (ctrl_it&CORE1553BRM_EXT_SYNC)
//       SchedSyncHandler(ctxt,irqData);
}

static xm_s32_t WriteMilStd(const kDevice_t *kDev, xm_u8_t *buffer, xmSSize_t len) {
    return 0;
}

static xm_s32_t ReadMilStd(const kDevice_t *kDev, xm_u8_t *buffer, xmSSize_t len) {
    return 0;
}

static kDevice_t milStdReg={
    .subId=0,
    .Reset=0,
    .Read=ReadMilStd,
    .Write=WriteMilStd,
};

static const kDevice_t *GetMilStd1553(xm_u32_t subId) {
    return &milStdReg;
}

xm_s32_t InitMilStd1553(void) {
    int i;

    eprintf("InitMilStd\n");
    g1553Dev=(desc1553Dev_t *)CONFIG_MIL_DESC_MEM_ADDR;
    memset(g1553Dev, 0, sizeof(desc1553Dev_t));
    /*Enable HW device on GR712 - Enable clock with the following options: 1553CK input -  24 MHZ*/
#ifdef CONFIG_MIL_STD_1553_INTERNAL_CLOCK
    StoreIoReg(LEON_GRGPREG_BASE,LoadIoReg(LEON_GRGPREG_BASE)|(CLK_SELECT_CLK_INPUT<<MIL_STD_1553_CLK_SELECT_BIT));
#endif

    /*Routine to enable the clock for the core 1553*/
    StoreIoReg(LEON_CLKGATE_BASE+CLKGATE_UNLOCK_REG,1<<CLKGATE_MIL_STD_1553);
    StoreIoReg(LEON_CLKGATE_BASE+CLKGATE_CLOCK_RST_REG,1<<CLKGATE_MIL_STD_1553);
    StoreIoReg(LEON_CLKGATE_BASE+CLKGATE_CLOCK_EN_REG,1<<CLKGATE_MIL_STD_1553);
    StoreIoReg(LEON_CLKGATE_BASE+CLKGATE_CLOCK_RST_REG,0x0);
    StoreIoReg(LEON_CLKGATE_BASE+CLKGATE_UNLOCK_REG,0x0);

    /*Reset BRM core*/
    StoreIoReg(MIL_STD_STATUS_CTRL,MIL_STD_STATUS_CTRL_RESET);

    /*Install handler and enable IRQ*/
    SetIrqHandler(MIL_STD_1553_IRQ, MilStdIrqHandler, 0);

    /*Redefine the allocation of functions to enable/disable interrupt*/
    hwIrqCtrl[MIL_STD_1553_IRQ]=(hwIrqCtrl_t){
        .Enable=MilStdEnableIrq,
                .Disable=MilStdDisableIrq,
                .Ack=MilStdDisableIrq,
                .End=MilStdEnableIrq,
    };

    /*Configuration of common control registers MIL-STD-1553*/

    /*Register 0 -> Control - Enable both buses, circular mode 1, interrupt log list and enable transmit word status.*/
    StoreIoReg(MIL_STD_BASE+CORE1553BRM_CTRL,CORE1553BRM_CTRL_XMTSW|CORE1553BRM_CTRL_BUFM_CIR_M1|CORE1553BRM_CTRL_BBEN|CORE1553BRM_CTRL_BAEN|CORE1553BRM_CTRL_INTEN|CORE1553BRM_CTRL_BCEN);

    /*Register 1 -> Operation and status - Set RT mode and parity.*/
#ifdef CONFIG_MIL_STD_1553_ODD_PARITY
    StoreIoReg(MIL_STD_BASE+CORE1553BRM_OP_STATUS,(MIL_STD_CORE1553BRM_ADDR_RT<<CORE1553BRM_OP_STATUS_RTA_BIT)|CORE1553BRM_OP_STATUS_MODE_RT|CORE1553BRM_OP_STATUS_RTPTY);
#else
    StoreIoReg(MIL_STD_BASE+CORE1553BRM_OP_STATUS,(MIL_STD_CORE1553BRM_ADDR_RT<<CORE1553BRM_OP_STATUS_RTA_BIT)|CORE1553BRM_OP_STATUS_MODE_RT);
#endif

    /*Register 3 -> Interrupt Mask - Unmask CORE1553BRM_EXT_SYNC (SUBAD - Subaddress Accessed Interrupt)*/
    StoreIoReg(MIL_STD_BASE+CORE1553BRM_IT_MASK,CORE1553BRM_EXT_SYNC);

    /*Register 5 -> Interrupt Pointer - Interrupt pointer register contains the staring base address and pointer location of the Interrupt Log List.*/
    StoreIoReg(MIL_STD_BASE+CORE1553BRM_IT_PTR,OFFSET_ADD(g1553Dev->irqLogs[0]));

    /*Register 8 -> Command block Pointer - This register contains the starting address of the descriptor table pointer*/
    StoreIoReg(MIL_STD_BASE+CORE1553BRM_DESC_PTR,OFFSET_ADD(g1553Dev->descRx[0]));

    /*Register 09 -> Status Word*/
    StoreIoReg(MIL_STD_BASE+CORE1553BRM_STATUS_WORD,0x0);

    /*Register 32 -> Enhanced features register - Select the core operation frequency*/
#ifdef CONFIG_MIL_STD_1553_INTERNAL_CLOCK 
    StoreIoReg(MIL_STD_BASE+CORE1553BRM_ENH_FEAT,(CORE1553BRM_ENH_FEAT_CLKFREQ_24<<CORE1553BRM_ENH_FEAT_CLKFREQ_BIT)); /*24 Mhz*/

#else

#ifdef CONFIG_MIL_STD_1553_CLKFREQ_12
      i=CORE1553BRM_ENH_FEAT_CLKFREQ_12;
#endif
#ifdef CONFIG_MIL_STD_1553_CLKFREQ_16
      i=CORE1553BRM_ENH_FEAT_CLKFREQ_16;
#endif
#ifdef CONFIG_MIL_STD_1553_CLKFREQ_20
      i=CORE1553BRM_ENH_FEAT_CLKFREQ_20;
#endif
#ifdef CONFIG_MIL_STD_1553_CLKFREQ_24
      i=CORE1553BRM_ENH_FEAT_CLKFREQ_24;
#endif

    StoreIoReg(MIL_STD_BASE+CORE1553BRM_ENH_FEAT,(i<<CORE1553BRM_ENH_FEAT_CLKFREQ_BIT));
#endif // CONFIG_MIL_STD_1553_INTERNAL_CLOCK


    /*Configuration of B1553BRM from GR712 board*/

    /*B1553BRM status/control register*/
    StoreIoReg(MIL_STD_STATUS_CTRL,MIL_STD_STATUS_CTRL_SSYSFN);

    /*B1553BRM interrupt register - interrupt level - INTACKH/INTACKM active high - INTACKH/INTACKM initialised to 1*/
    StoreIoReg(MIL_STD_IT_SETTING,MIL_STD_IT_SETTING_INTACKH|MIL_STD_IT_SETTING_INTACKM);

    /*B1553BRM AHB page address register - Set AHB page address register */
    StoreIoReg(MIL_STD_AHB_PAGE_ADR,(xmAddress_t)&g1553Dev->descRx[0]);


    /*Configuration of Remote Terminal-specific registers MIL-STD-1553*/

    /* Register 16-17 Illegalize the receive Subaddresses 0, 8, 29 and 31 */
    StoreIoReg(MIL_STD_BASE + CORE1553BRM_RT_CMD_LEGAL    ,  0x0101);
    StoreIoReg(MIL_STD_BASE + CORE1553BRM_RT_CMD_LEGAL + 4,  0xA000);

    /* Register 18-19 Illegalize the Transmit Subaddresses 0, 8, 29 and 31 */
    StoreIoReg(MIL_STD_BASE + CORE1553BRM_RT_CMD_LEGAL + 8,  0x0101);
    StoreIoReg(MIL_STD_BASE + CORE1553BRM_RT_CMD_LEGAL + 12, 0xA000);

    /* Register 20-21 Illegalize all the broadcast receive Subaddress except 29 and 31 */
    StoreIoReg(MIL_STD_BASE + CORE1553BRM_RT_CMD_LEGAL + 16, 0xFFFF);
    StoreIoReg(MIL_STD_BASE + CORE1553BRM_RT_CMD_LEGAL + 20, 0x5FFF);

    /* Register 22-23 Illegalize all the broadcast transmit Subaddress */
    StoreIoReg(MIL_STD_BASE + CORE1553BRM_RT_CMD_LEGAL + 24, 0xFFFF);
    StoreIoReg(MIL_STD_BASE + CORE1553BRM_RT_CMD_LEGAL + 28, 0xFFFF);

    /* Register 24-25 Legalize the receive Mode Code 17 */
    StoreIoReg(MIL_STD_BASE + CORE1553BRM_RT_CMD_LEGAL + 32, 0xFFFF);
    StoreIoReg(MIL_STD_BASE + CORE1553BRM_RT_CMD_LEGAL + 36, 0xFFFD);

    /* Register 26-27 Legalize the transmit Mode Code 2, 4, 5 and 8 */
    StoreIoReg(MIL_STD_BASE + CORE1553BRM_RT_CMD_LEGAL + 40, 0xFECB);
    StoreIoReg(MIL_STD_BASE + CORE1553BRM_RT_CMD_LEGAL + 44, 0xFFFF);

    /* Register 28-29 Legalize the broadcast receive Mode Code 17 */
    StoreIoReg(MIL_STD_BASE + CORE1553BRM_RT_CMD_LEGAL + 48, 0xFFFF);
    StoreIoReg(MIL_STD_BASE + CORE1553BRM_RT_CMD_LEGAL + 52, 0xFFFD);
 
    /* Register 30-31 Legalize the broadcast transmit Mode Code 4, 5 and 8 */
    StoreIoReg(MIL_STD_BASE + CORE1553BRM_RT_CMD_LEGAL + 56, 0xFECF);
    StoreIoReg(MIL_STD_BASE + CORE1553BRM_RT_CMD_LEGAL + 60, 0xFFFF);

    /*Clear the interrupt in the processor*/
    PicClearIrq(MIL_STD_1553_IRQ);


    /*Init descriptor table*/

    for (i=0;i<C_NB_MAX_SA;i++)
    {
        /* RX Sub-address 0 - 31 */
        g1553Dev->descRx[i].controlWord = (0x0100|C_DBCW_INTX|C_DBCW_IWA|C_DBCW_IBRD);
        g1553Dev->descRx[i].dataPointerA = OFFSET_ADD(g1553Dev->rxSubAddrMsgs[i]);
        g1553Dev->descRx[i].dataPointerB = OFFSET_ADD(g1553Dev->rxSubAddrMsgs[i]);
        g1553Dev->descRx[i].broadcastPointer = OFFSET_ADD(g1553Dev->rxSubAddrMsgs[i]);

        /* TX Sub-address 0 - 31 */
        g1553Dev->descTx[i].controlWord = (C_DBCW_IWA|C_DBCW_IBRD);
        g1553Dev->descTx[i].dataPointerA = OFFSET_ADD(g1553Dev->txSubAddrMsgs[i]);
        g1553Dev->descTx[i].dataPointerB = OFFSET_ADD(g1553Dev->txSubAddrMsgs[i]);
        g1553Dev->descTx[i].broadcastPointer = OFFSET_ADD(g1553Dev->txSubAddrMsgs[i]);

        /* RX Mode Code 0 - 31 */
        g1553Dev->descModeCodeRx[i].controlWord = (0x0100 | C_DBCW_INTX | C_DBCW_IWA | C_DBCW_IBRD);
        g1553Dev->descModeCodeRx[i].dataPointerA = OFFSET_ADD(g1553Dev->rxModeCodeMsgs[i]);
        g1553Dev->descModeCodeRx[i].dataPointerB = OFFSET_ADD(g1553Dev->rxModeCodeMsgs[i]);;
        g1553Dev->descModeCodeRx[i].broadcastPointer = OFFSET_ADD(g1553Dev->rxModeCodeMsgs[i]);;

        /* TX Mode Code 0 - 31 */
        g1553Dev->descModeCodeTx[i].controlWord = (C_DBCW_IWA | C_DBCW_IBRD);
        g1553Dev->descModeCodeTx[i].dataPointerA = OFFSET_ADD(g1553Dev->txModeCodeMsgs[i]);
        g1553Dev->descModeCodeTx[i].dataPointerB = OFFSET_ADD(g1553Dev->txModeCodeMsgs[i]);
        g1553Dev->descModeCodeTx[i].broadcastPointer = OFFSET_ADD(g1553Dev->txModeCodeMsgs[i]);
    }

#ifndef CONFIG_PLAN_EXTSYNC
//        HwEnableIrq(MIL_STD_1553_IRQ); /*Enable the interrupt in the processor*/  /*>>> This interrupt should be enabled by the scheduler*/
#endif
    GetKDevTab[XM_DEV_MIL_STD_ID]=GetMilStd1553;

    /*Start Core execution */
    StoreIoReg(MIL_STD_BASE+CORE1553BRM_CTRL,LoadIoReg(MIL_STD_BASE+CORE1553BRM_CTRL)|CORE1553BRM_CTRL_STEX);

    return 0;
}

REGISTER_KDEV_SETUP(InitMilStd1553);


