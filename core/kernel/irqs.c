/*
 * $FILE: irqs.c
 *
 * Independent part of interrupt handling
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
  - [02/09/2015:SPR-020915-04] Remove __VBOOT macro and boot.h
*/
#include <bitwise.h>
#include <irqs.h>
#include <kthread.h>
#include <sched.h>
#include <stdc.h>
#include <processor.h>
#include <objects/hm.h>
#ifdef CONFIG_OBJ_STATUS_ACC
#include <objects/status.h>
#endif
#include <local.h>

// Definitions
struct irqTabEntry irqHandlerTab[CONFIG_NO_HWIRQS];
trapHandler_t trapHandlerTab[NO_TRAPS];
hwIrqCtrl_t hwIrqCtrl[CONFIG_NO_HWIRQS];

void DefaultIrqHandler(cpuCtxt_t *ctxt, void *data) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    struct xmcPartition *cfg=GetPartitionCfg(info->sched.cKThread);
    xmHmLog_t hmLog;

    memset(&hmLog, 0, sizeof(xmHmLog_t));
    hmLog.opCode|=XM_HM_EV_UNEXPECTED_TRAP<<HMLOG_OPCODE_EVENT_BIT;

    hmLog.opCode|=(ctxt->irqNr&HMLOG_OPCODE_MODID_MASK)<<HMLOG_OPCODE_MODID_BIT;

    if (info->sched.cKThread->ctrl.g)
        hmLog.opCode|=cfg->rId<<HMLOG_OPCODE_PARTID_BIT;
    hmLog.opCode|=HMLOG_OPCODE_SYS_MASK;

    CpuCtxt2HmCpuCtxt(ctxt, &hmLog.cpuCtxt);
    hmLog.opCode|=HMLOG_OPCODE_VALID_CPUCTXT_MASK;
    HmRaiseEvent(&hmLog);
    kprintf("Unexpected irq %d\n", ctxt->irqNr);
}

static void TriggerIrqHandler(cpuCtxt_t *ctxt, void *data) {
    xmId_t partId;
    partId=xmcTab.hpv.hwIrqTab[ctxt->irqNr].owner;

    SetPartitionHwIrqPending(&partitionTab[partId], ctxt->irqNr);
}

void SetTrapPending(cpuCtxt_t *ctxt) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    ASSERT(!AreKThreadFlagsSet(info->sched.cKThread, KTHREAD_TRAP_PENDING_F));
    SetKThreadFlags(info->sched.cKThread, KTHREAD_TRAP_PENDING_F);
}

static inline xmAddress_t IsInPartExTable(xmAddress_t addr) {
    extern struct exPTable {
        xmAddress_t a;
        xmAddress_t b;
    } exPTable[]; 
    struct exPTable *exPTablePtr;
    xm_s32_t e;
    
    for (exPTablePtr=exPTable; exPTablePtr; exPTablePtr=(struct exPTable *)exPTablePtr[e].b) {
        for (e=0; exPTablePtr[e].a; e++)
            if (addr==exPTablePtr[e].a) 
                return exPTablePtr[e].b;
    }
    return 0;
}

void DoTrap(cpuCtxt_t *ctxt) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    struct xmcPartition *cfg=GetPartitionCfg(info->sched.cKThread);
    xmHmLog_t hmLog;
    xm_s32_t action;
    xmWord_t pc;
    ASSERT(ctxt->irqNr<NO_TRAPS);

    if ((pc=IsInPartExTable(GET_CTXT_PC(ctxt)))) {
        SET_CTXT_PC(ctxt, pc);
        return;
    }

    if (trapHandlerTab[ctxt->irqNr])
        if (trapHandlerTab[ctxt->irqNr](ctxt))
            return;

    memset(&hmLog, 0, sizeof(xmHmLog_t));
    hmLog.opCode|=
        ((ctxt->irqNr+XM_HM_MAX_GENERIC_EVENTS)<<HMLOG_OPCODE_EVENT_BIT);
    if (IsSvIrqCtxt(ctxt)) {
	if (info->sched.cKThread->ctrl.g)
            hmLog.opCode|=(cfg->rId<<HMLOG_OPCODE_PARTID_BIT);
        if (ArchTrapIsSysCtxt(ctxt))
            hmLog.opCode|=HMLOG_OPCODE_SYS_MASK;
    } else {
        hmLog.opCode|=(cfg->rId<<HMLOG_OPCODE_PARTID_BIT);
    }

    CpuCtxt2HmCpuCtxt(ctxt, &hmLog.cpuCtxt);
    hmLog.opCode|=HMLOG_OPCODE_VALID_CPUCTXT_MASK;
#ifdef CONFIG_VERBOSE_TRAP
    DumpState(ctxt);
#endif
    action=HmRaiseEvent(&hmLog);
    if (IsSvIrqCtxt(ctxt)&&((hmLog.opCode&HMLOG_OPCODE_SYS_MASK)!=HMLOG_OPCODE_SYS_MASK))
        PartitionPanic(ctxt,"Partition in unrecoverable state\n");
    if (!IsSvIrqCtxt(ctxt)) {
	if (action)
	    SetTrapPending(ctxt);
    } else
	SystemPanic(ctxt,"Unexpected/unhandled trap - TRAP: 0x%x ERROR CODE: 0x%x\n", info->sched.cKThread->ctrl.g->partCtrlTab->trap2Vector[ctxt->irqNr], GET_ECODE(ctxt));
}

void DoUnrecovExcp(cpuCtxt_t *ctxt) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    struct xmcPartition *cfg=GetPartitionCfg(info->sched.cKThread);
    xmHmLog_t hmLog;

    memset(&hmLog, 0, sizeof(xmHmLog_t));
    hmLog.opCode|=(XM_HM_EV_PARTITION_UNRECOVERABLE<<HMLOG_OPCODE_EVENT_BIT);
    if (IsSvIrqCtxt(ctxt)) {
	if (info->sched.cKThread->ctrl.g)
            hmLog.opCode|=(cfg->rId<<HMLOG_OPCODE_PARTID_BIT);
        hmLog.opCode|=HMLOG_OPCODE_SYS_MASK;
    } else {
        hmLog.opCode|=(cfg->rId<<HMLOG_OPCODE_PARTID_BIT);
    }

    CpuCtxt2HmCpuCtxt(ctxt, &hmLog.cpuCtxt);
    hmLog.opCode|=HMLOG_OPCODE_VALID_CPUCTXT_MASK;
    DumpState(ctxt);
    HmRaiseEvent(&hmLog);

    PartitionPanic(ctxt,"Partition unrecoverable error : 0x%x\n", ctxt->irqNr);
}

void DoIrq(cpuCtxt_t *ctxt) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    ASSERT(!HwIsSti());
#ifdef CONFIG_OBJ_STATUS_ACC
    systemStatus.noIrqs++;
#endif
    info->cpu.irqNestingCounter++;
    HwAckIrq(ctxt->irqNr);
    if (irqHandlerTab[ctxt->irqNr].handler)
	(*(irqHandlerTab[ctxt->irqNr].handler))(ctxt, irqHandlerTab[ctxt->irqNr].data);
    else
	DefaultIrqHandler(ctxt, 0);
#ifdef CONFIG_OBJ_STATUS_ACC
    systemStatus.irqLastOccurence[ctxt->irqNr]=GetSysClockUsec();
#endif
    info->cpu.irqNestingCounter--;
    do {
	Schedule();
    } while (info->cpu.irqNestingCounter==SCHED_PENDING);
    ASSERT(!HwIsSti());
    ASSERT(!(info->cpu.irqNestingCounter&SCHED_PENDING));
}

void SetupIrqs(void) {
    xm_s32_t irqNr;

    for (irqNr=0; irqNr<CONFIG_NO_HWIRQS; irqNr++) {
	if (xmcTab.hpv.hwIrqTab[irqNr].owner!=XM_IRQ_NO_OWNER) {
	    irqHandlerTab[irqNr]=(struct irqTabEntry){
		.handler=TriggerIrqHandler, 
		.data=0,
	    };
	} else  {
	    irqHandlerTab[irqNr]=(struct irqTabEntry){
		.handler=DefaultIrqHandler, 
		.data=0,
	    };
	}
    }

    for (irqNr=0; irqNr<NO_TRAPS; irqNr++)
	trapHandlerTab[irqNr]=0;

    ArchSetupIrqs();
}

irqHandler_t SetIrqHandler(xm_s32_t irq, irqHandler_t irqHandler, void *data) {
    irqHandler_t oldHandler=irqHandlerTab[irq].handler;
    if (irqHandler) {
	irqHandlerTab[irq]=(struct irqTabEntry){
	    .handler=irqHandler,
	    .data=data,
	};
    } else
	irqHandlerTab[irq]=(struct irqTabEntry){
	    .handler=DefaultIrqHandler,
	    .data=0,
	};
    return oldHandler;
}

trapHandler_t SetTrapHandler(xm_s32_t trap, trapHandler_t trapHandler) {
    trapHandler_t oldHandler=trapHandlerTab[trap];

    trapHandlerTab[trap]=trapHandler;
    return oldHandler;
}

static inline xm_s32_t AreHwIrqsPending(partitionControlTable_t *partCtrlTab) {
    xm_s32_t eIrq;

    eIrq=partCtrlTab->hwIrqsPend&~partCtrlTab->hwIrqsMask;
    if (eIrq) {
#ifdef CONFIG_HWIRQ_PRIO_FBS
        eIrq=_Ffs(eIrq);
#else
        eIrq=_Fls(eIrq);
#endif
        ASSERT(eIrq>=0&&eIrq<16);	    
        return eIrq;
    }
    
    return -1;
}

static inline xm_s32_t AreExtIrqsPending(partitionControlTable_t *partCtrlTab) {
    xm_s32_t eIrq;
    eIrq=partCtrlTab->extIrqsPend&~partCtrlTab->extIrqsMask;
    if (eIrq) {
#ifdef CONFIG_HWIRQ_PRIO_FBS
        eIrq=_Ffs(eIrq);
#else
        eIrq=_Fls(eIrq);
#endif
	return eIrq;
    }       
  
    return -1;
}

static inline xm_s32_t AreExtTrapsPending(partitionControlTable_t *partCtrlTab) {
    xm_s32_t eIrq;

    eIrq=partCtrlTab->extIrqsPend&XM_EXT_TRAPS;
    if (eIrq) {
#ifdef CONFIG_HWIRQ_PRIO_FBS
        eIrq=_Ffs(eIrq);
#else
        eIrq=_Fls(eIrq);
#endif
	return eIrq;
    }       
  
    return -1;
}

xm_s32_t RaisePendIrqs(cpuCtxt_t *ctxt) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    partitionControlTable_t *partCtrlTab;
    xm_s32_t eIrq, emul;

    if (!info->sched.cKThread->ctrl.g||IsSvIrqCtxt(ctxt))
	return ~0;

    // SwTrap
    if (info->sched.cKThread->ctrl.g->swTrap&0x1) {
        emul=info->sched.cKThread->ctrl.g->swTrap>>1;
        info->sched.cKThread->ctrl.g->swTrap=0;
        return IrqVector2Address(emul);
    }
    
    partCtrlTab=info->sched.cKThread->ctrl.g->partCtrlTab;
    // 1) Check pending traps
    if (AreKThreadFlagsSet(info->sched.cKThread, KTHREAD_TRAP_PENDING_F)) {
        ClearKThreadFlags(info->sched.cKThread, KTHREAD_TRAP_PENDING_F);
        DisablePCtrlTabIrqs(&partCtrlTab->iFlags);
	emul=ArchEmulTrapIrq(ctxt, partCtrlTab, ctxt->irqNr);
        return IrqVector2Address(emul);
    }

    // 2) Check pending extended trap
    if ((eIrq=AreExtTrapsPending(partCtrlTab))>-1) {
        partCtrlTab->extIrqsPend&=~(1<<eIrq);
        DisablePCtrlTabIrqs(&partCtrlTab->iFlags);
	emul=ArchEmulExtIrq(ctxt, partCtrlTab, eIrq);
        return IrqVector2Address(emul);
    }

    // At this moment irq flags must be set
    if (!ArePCtrlTabIrqsSet(partCtrlTab->iFlags))
        return ~0;

    // 3) Check pending hwirqs
    if ((eIrq=AreHwIrqsPending(partCtrlTab))>-1) {
        partCtrlTab->hwIrqsPend&=~(1<<eIrq);
        MaskPCtrlTabIrq(&partCtrlTab->hwIrqsMask, (1<<eIrq));
        DisablePCtrlTabIrqs(&partCtrlTab->iFlags);
	emul=ArchEmulHwIrq(ctxt, partCtrlTab, eIrq);
        return IrqVector2Address(emul);
    }

     // 4) Check pending extirqs
    if ((eIrq=AreExtIrqsPending(partCtrlTab))>-1) {
        partCtrlTab->extIrqsPend&=~(1<<eIrq);
        MaskPCtrlTabIrq(&partCtrlTab->extIrqsMask, (1<<eIrq));
        DisablePCtrlTabIrqs(&partCtrlTab->iFlags);
	emul=ArchEmulExtIrq(ctxt, partCtrlTab, eIrq);
        return IrqVector2Address(emul);
    }

    // No emulation required
    return ~0;
}
