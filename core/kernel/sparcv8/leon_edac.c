/*
 * $FILE: leon_edac.c
 *
 * EDAC for GR712RC support
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@fentiss.com>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

/*
  Changelog:
  - [17/11/2015:SPR-221015-01] leon_ead.c file created
*/

#include <physmm.h>
#include <local.h>
#include <irqs.h>
#include <sparcv8/leon.h>
#include <sparcv8/leon_edac.h>
#include <sparcv8/xmconf.h>
#include <objects/status.h>
#include <objects/hm.h>

#define TRUE 1
#define FALSE 0

static xm_s32_t Sparcv8EDACFault(cpuCtxt_t *ctxt) {
    xm_u32_t ahbStatus = LoadIoReg(LEON_AHB_STATUS);

    /* Check if trap is from EDAC fault */
    if (ahbStatus & (1<<8)) {

        /* Correctable error */
        if (ahbStatus & (1<<9)) {
            /* No correctable error should be produced if trap with error was
             * generated.
             * TODO: Study effects or DMA transactions
             */
            kprintf("ERROR: Correctable in trap\n");
        }
        else
        {
            /* Uncorrectable error */
            xmHmLog_t hmLog;
            localProcessor_t *info=GET_LOCAL_PROCESSOR();
            struct xmcPartition *cfg=GetPartitionCfg(info->sched.cKThread);
            xm_u32_t ahbFailingAddr = LoadIoReg(LEON_AHB_FAILING_ADDRESS);

            memset(&hmLog, 0, sizeof(xmHmLog_t));
            hmLog.opCode =
                XM_HM_EV_SPARCV8_UNCORRECTABLE_EDAC_ERROR << HMLOG_OPCODE_EVENT_BIT;

            if (IsSvIrqCtxt(ctxt)) {
                if (info->sched.cKThread->ctrl.g)
                {
                    hmLog.opCode |= (cfg->rId << HMLOG_OPCODE_PARTID_BIT);
                }
                if (ArchTrapIsSysCtxt(ctxt))
                {
                    hmLog.opCode |= HMLOG_OPCODE_SYS_MASK;
                }
            }
            else
            {
                hmLog.opCode|=(cfg->rId<<HMLOG_OPCODE_PARTID_BIT);
            }
            hmLog.payload[0] = ahbFailingAddr; 
            /* NE resetted */
            StoreIoReg(LEON_AHB_STATUS, 0);
            HmRaiseEvent(&hmLog);
        }
        return TRUE;
    }
    return FALSE;
}


static void AhbBusErrorIrqHandler(cpuCtxt_t *ctxt, void *irqData) {
    xm_u32_t ahbStatus = LoadIoReg(LEON_AHB_STATUS);

    /* New EDAC error detected */
    if (ahbStatus & (1 << 8))
    {
        localProcessor_t *info=GET_LOCAL_PROCESSOR();
        xm_u32_t ahbFailingAddr = LoadIoReg(LEON_AHB_FAILING_ADDRESS);
        /* Correctable error */
        if (ahbStatus & (1<<9)) {
            xm_u32_t flags;
            /* Memory scrubbing is only performed in read/write memory */
            if ((PmmFindArea(ahbFailingAddr, 4,
                GetPartition(info->sched.cKThread), &flags) == 1) &&
                ((flags & XM_MEM_AREA_READONLY) != XM_MEM_AREA_READONLY))
            {
                xm_u32_t tmp = LoadIoReg(ahbFailingAddr);
                StoreIoReg(ahbFailingAddr, tmp);
                systemStatus.edacErrorCounter++;
            }
            /* NE resetted */
            StoreIoReg(LEON_AHB_STATUS, 0);
        }
    }
    HwEnableIrq(LEON_AHB_BUS_ERROR_IRQ);
}


void InitEDAC(void) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    systemStatus.edacErrorCounter = 0;

    SetIrqHandler(LEON_AHB_BUS_ERROR_IRQ, AhbBusErrorIrqHandler, 0);    
    /*tt=0x1*/
    SetTrapHandler(4, Sparcv8EDACFault);
    /*tt=0x9*/
    SetTrapHandler(16, Sparcv8EDACFault);
    HwEnableIrq(LEON_AHB_BUS_ERROR_IRQ);
    info->cpu.globalIrqMask&=~(1<<LEON_AHB_BUS_ERROR_IRQ);
}
