/*
 * $FILE: sched.c
 *
 * Scheduling related stuffs
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
  - [05/08/2015:SPR-010915-01   ] external synchronisation algorithm updated.
  - [02/09/2015:SPR-020915-03   ] 'goto' replaced with equivalent code.
  - [02/09/2015:SPR-020915-04   ] Remove VBOOT macro and boot.h.
  - [28/04/2016:CP-080316-01  JC] irqLastOccurence is always updated by the external synchronization signal. 
                                  It is done even if CONFIG_OBJ_STATUS_ACC is not defined.
*/

#include <assert.h>
#include <audit.h>
#include <rsvmem.h>
#include <irqs.h>
#include <sched.h>
#include <smp.h>
#include <spinlock.h>
#include <stdc.h>
#include <arch/asm.h>
#include <autoconf.h>
#include <kwatchdog.h>

#if defined(CONFIG_DEV_LICE_INTERFACE) || defined(CONFIG_DEV_LICE_SCHEDULING)
#include <drivers/lice_interface.h>
#endif

partition_t *partitionTab;
static struct schedData *schedDataTab;

static const struct xmcSchedCyclicPlan idleCyclicPlanTab= {
    .nameOffset = 0,
    .id = 0,
    .majorFrame = 0,
    .noSlots = 0,
    .slotsOffset = 0,
};

void InitSched(void) {
    extern void SetupKThreads(void);

    SetupKThreads();
    GET_MEMZ(partitionTab, sizeof(partition_t)*xmcTab.noPartitions);
    GET_MEMZ(schedDataTab, sizeof(struct schedData)*xmcTab.hpv.noCpus);
}

#ifdef CONFIG_PLAN_EXTSYNC
static volatile xm_s32_t extSync[CONFIG_NO_CPUS]={0}, actExtSync[CONFIG_NO_CPUS]={0}, sysReset[CONFIG_NO_CPUS]={0};

static void ExtSyncRaiseHmEvent(void) {
    xmHmLog_t hmLog;

    memset(&hmLog, 0, sizeof(xmHmLog_t));
    hmLog.opCode|=(XM_HM_EV_EXTSYNC_ERROR<<HMLOG_OPCODE_EVENT_BIT);
    hmLog.opCode|=HMLOG_OPCODE_SYS_MASK;
    HmRaiseEvent(&hmLog);
}

void SchedSyncHandler(cpuCtxt_t *irqCtxt, void *extra) {
    extSync[GET_CPU_ID()]=1;
    HwEnableIrq(irqCtxt->irqNr);
//    Schedule();
}

#endif

extern void DefaultIrqHandler(cpuCtxt_t *ctxt, void *data);

void InitSchedLocal(kThread_t *idle) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();

#ifdef CONFIG_PLAN_EXTSYNC
    xm_u32_t nCpu=GET_CPU_ID();
    xm_s32_t irqNr = xmcSchedCyclicPlanTab[xmcTab.hpv.cpuTab[GET_CPU_ID()].schedCyclicPlansOffset].extSync;
    actExtSync[nCpu]=0;
    if (irqNr != -1) {
//        localProcessor_t *info=GET_LOCAL_PROCESSOR();
//        info->cpu.globalIrqMask &=~(1<<irqNr); //Keep in mind the ext sync irq in the globalIrqMask
        actExtSync[nCpu]=1;
        extSync[nCpu]=0;
        if (irqHandlerTab[irqNr].handler==DefaultIrqHandler){ //if no previous handler has been installed, SchedSyncHandler is installed
	    SetIrqHandler(irqNr, SchedSyncHandler, 0);
        }
        HwEnableIrq(irqNr);
    }
    sysReset[nCpu] = 1;
#endif

    InitIdle(idle, GET_CPU_ID());
    info->sched.cKThread=info->sched.idleKThread=idle;
    info->sched.data=&schedDataTab[GET_CPU_ID()];
    memset(info->sched.data, 0, sizeof(struct schedData));
    InitKTimer(GET_CPU_ID(), &info->sched.data->kTimer, (void (*)(struct kTimer *, void *))SetSchedPending, NULL, NULL);
    info->sched.data->slot=-1;
    info->sched.data->plan.new=&xmcSchedCyclicPlanTab[xmcTab.hpv.cpuTab[GET_CPU_ID()].schedCyclicPlansOffset];
    info->sched.data->plan.current=0;
    info->sched.data->plan.prev=0;
}

////////////////// CYCLIC SCHEDULER
xm_s32_t SwitchSchedPlan(xm_s32_t newPlanId, xm_s32_t *oldPlanId) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xm_s32_t e;
    *oldPlanId=-1;
    if (info->sched.data->plan.current)
        *oldPlanId=info->sched.data->plan.current->id;
    
    for (e=0; e<xmcTab.hpv.noCpus; e++) {
        if (newPlanId<xmcTab.hpv.cpuTab[e].noSchedCyclicPlans)
            localProcessorInfo[e].sched.data->plan.new=&xmcSchedCyclicPlanTab[xmcTab.hpv.cpuTab[e].schedCyclicPlansOffset+newPlanId];
        else
            localProcessorInfo[e].sched.data->plan.new=&idleCyclicPlanTab;
    }

    return 0;
}

inline void MakePlanSwitch(xmTime_t cTime, struct schedData *data) {
    if (data->plan.current!=data->plan.new) {
        data->plan.prev=data->plan.current;
        data->plan.current=data->plan.new;
        data->planSwitchTime=cTime;
        data->slot=-1;
        data->mjf=0;
        data->slotExec=-1;
    }
}

static kThread_t *GetReadyKThread(struct schedData *cyclic) {
    const struct xmcSchedCyclicPlan *plan;
    xmTime_t cTime=GetSysClockUsec();
    kThread_t *newK=0;
    xm_u32_t t, nextTime;
    xm_s32_t slotTabEntry;
    xm_u32_t getIdle;
#ifdef CONFIG_PLAN_EXTSYNC
    xm_s32_t interval = xmcSchedCyclicPlanTab[xmcTab.hpv.cpuTab[GET_CPU_ID()].schedCyclicPlansOffset].interval;
    xm_s32_t syncIrqNr = xmcSchedCyclicPlanTab[xmcTab.hpv.cpuTab[GET_CPU_ID()].schedCyclicPlansOffset].extSync;
    xm_u32_t nCpu = GET_CPU_ID();
#endif
#ifdef CONFIG_WATCHDOG
    struct hwTimerData *hwWatchdogData=0;
    hwTimer_t *watchdog=AllocWatchdog(hwWatchdogData);
#endif /* CONFIG_WATCHDOG */
    if (cyclic->nextAct>cTime)
        return (cyclic->kThread&&!AreKThreadFlagsSet(cyclic->kThread, KTHREAD_HALTED_F|KTHREAD_SUSPENDED_F)&&AreKThreadFlagsSet(cyclic->kThread, KTHREAD_READY_F))?cyclic->kThread:0;
    plan=cyclic->plan.current;
    if (cyclic->mjf<=cTime) {
#ifdef CONFIG_PLAN_EXTSYNC
        if (actExtSync[nCpu])
        {
            if (sysReset[nCpu])
            {
                const struct xmcSchedCyclicPlan *initPlan;
                initPlan=cyclic->plan.new;
                HwClearIrq(syncIrqNr);
                HwEnableIrq(syncIrqNr);
                while (!extSync[nCpu])
                {
                    xmTime_t iVal= cyclic->mjf + interval + initPlan->majorFrame;
                    while (!IsHwIrqPending(syncIrqNr)
                            && (cTime < iVal))
                    {
                        cTime = GetSysClockUsec();
                    }
                    if (cTime < iVal)
                    {
                        cpuCtxt_t ctxt;
                        ctxt.irqNr = syncIrqNr;
                        if (irqHandlerTab[syncIrqNr].handler)
                        {
                            (*(irqHandlerTab[syncIrqNr].handler))(&ctxt,
                                    irqHandlerTab[syncIrqNr].data);
                        }
                        if (extSync[nCpu])
                        {
                           systemStatus.irqLastOccurence[syncIrqNr]=cTime;
                        }
                    }
                    else
                    {
                        ExtSyncRaiseHmEvent();
                        extSync[nCpu]=1;
                    }
                }
                HwClearIrq(syncIrqNr);
                HwDisableIrq(syncIrqNr);
                cTime = GetSysClockUsec();
                sysReset[nCpu] = 0;
            }
            else
            {
                if (!extSync[nCpu])
                {
                    HwClearIrq(syncIrqNr);
                    HwEnableIrq(syncIrqNr);
                    while (!extSync[nCpu])
                    {
                        xmTime_t iVal= cyclic->mjf + interval;
                        while (!IsHwIrqPending(syncIrqNr)
                                && (cTime < iVal))
                        {
                            cTime = GetSysClockUsec();
                        }
                        if (cTime < iVal)
                        {
                            cpuCtxt_t ctxt;
                            ctxt.irqNr = syncIrqNr;
                            if (irqHandlerTab[syncIrqNr].handler)
                            {
                                (*(irqHandlerTab[syncIrqNr].handler))(&ctxt,
                                        irqHandlerTab[syncIrqNr].data);
                            }
                            if (extSync[nCpu])
                            {
                                systemStatus.irqLastOccurence[syncIrqNr]=cTime;
                            }
                        }
                        else
                        {
                            ExtSyncRaiseHmEvent();
                            extSync[nCpu]=1;
                        }
                    }
                    HwClearIrq(syncIrqNr);
                    HwDisableIrq(syncIrqNr);
                }
                else
                {
                    HwClearIrq(syncIrqNr);
                    HwDisableIrq(syncIrqNr);
                    ExtSyncRaiseHmEvent();
                }
            }
            cyclic->slot = -1;
        }
#endif
#ifdef CONFIG_WATCHDOG
        if(numMafs==CONFIG_WATCHDOG_NMAF)
            numMafs=0;
        if(!numMafs){
#ifdef CONFIG_AUDIT_EVENTS
            RaiseAuditEvent(XM_HYPERVISOR_ID, XM_AUDIT_START_WATCHDOG);
#endif /* CONFIG_AUDIT_EVENTS */
#ifdef CONFIG_DEV_LICE_INTERFACE
            LiceCmdStartWatchdog();
#endif /* CONFIG_DEV_LICE_INTERFACE */
            watchdog->SetHwTimer(cTime+CONFIG_WATCHDOG_TIMEOUT,hwWatchdogData);
#ifdef CONFIG_DEV_LICE_INTERFACE
            LiceCmdEndWatchdog();
#endif /* CONFIG_DEV_LICE_INTERFACE */
#ifdef CONFIG_AUDIT_EVENTS
            RaiseAuditEvent(XM_HYPERVISOR_ID, XM_AUDIT_END_WATCHDOG);
#endif /* CONFIG_AUDIT_EVENTS */

        }
        numMafs++;
#endif /* CONFIG_WATCHDOG */
        MakePlanSwitch(cTime, cyclic);
        plan=cyclic->plan.current;
        if (cyclic->slot>=0) {
            while(cyclic->mjf<=cTime) {
                cyclic->sExec=cyclic->mjf;
                cyclic->mjf+=plan->majorFrame;
            }
#ifdef CONFIG_OBJ_STATUS_ACC
            systemStatus.currentMaf++;
#endif            
        } else {
            cyclic->sExec=cTime;
            cyclic->mjf=plan->majorFrame+cyclic->sExec;
        } 
        cyclic->slotExec=-1;
        cyclic->slot=0;
#ifdef CONFIG_WATCHDOG
        if(numMafs==CONFIG_WATCHDOG_NMAF)
            numMafs=0;
        if(numMafs==0)
        {
#ifdef CONFIG_DEV_LICE_INTERFACE
            LiceCmdStartWatchdog();
#endif /* CONFIG_DEV_LICE_INTERFACE */
#ifdef CONFIG_AUDIT_EVENTS
            RaiseAuditEvent(XM_HYPERVISOR_ID, XM_AUDIT_START_WATCHDOG);
#endif /* CONFIG_AUDIT_EVENTS */
            watchdog->SetHwTimer(cTime+CONFIG_WATCHDOG_TIMEOUT,hwWatchdogData);
#ifdef CONFIG_DEV_LICE_INTERFACE
            LiceCmdEndWatchdog();
#endif /* CONFIG_DEV_LICE_INTERFACE */
#ifdef CONFIG_AUDIT_EVENTS
            RaiseAuditEvent(XM_HYPERVISOR_ID, XM_AUDIT_END_WATCHDOG);
#endif /* CONFIG_AUDIT_EVENTS */
        }
        numMafs++;
#endif /* CONFIG_WATCHDOG */
    }
#ifdef CONFIG_PLAN_EXTSYNC
    extSync[nCpu]=0;
#endif
    t=cTime-cyclic->sExec;
    nextTime=plan->majorFrame;
    getIdle = 0;
    // Calculate our next slot
    if (cyclic->slot>=plan->noSlots)
        getIdle = 1; // getting idle
    while (!getIdle && 
           t>=xmcSchedCyclicSlotTab[plan->slotsOffset+cyclic->slot].eExec) {
        cyclic->slot++;
        if (cyclic->slot>=plan->noSlots)
            getIdle = 1; // getting idle
    }
    if (!getIdle) {
        slotTabEntry=plan->slotsOffset+cyclic->slot;
        if (t>=xmcSchedCyclicSlotTab[slotTabEntry].sExec) {
            ASSERT((xmcSchedCyclicSlotTab[slotTabEntry].partitionId>=0)&&(xmcSchedCyclicSlotTab[slotTabEntry].partitionId<xmcTab.noPartitions));
            ASSERT(partitionTab[xmcSchedCyclicSlotTab[slotTabEntry].partitionId].kThread[xmcSchedCyclicSlotTab[slotTabEntry].vCpuId]);
            newK=partitionTab[xmcSchedCyclicSlotTab[slotTabEntry].partitionId].kThread[xmcSchedCyclicSlotTab[slotTabEntry].vCpuId];
            if (!AreKThreadFlagsSet(newK, KTHREAD_HALTED_F|KTHREAD_SUSPENDED_F)&&
                AreKThreadFlagsSet(newK, KTHREAD_READY_F)) {
                nextTime=xmcSchedCyclicSlotTab[slotTabEntry].eExec;
            } else {
                cyclic->slotExec=cyclic->slot;
                newK=0;
                if ((cyclic->slot+1)<plan->noSlots)
                    nextTime=xmcSchedCyclicSlotTab[slotTabEntry+1].sExec;
            }        
        } else {
            nextTime=xmcSchedCyclicSlotTab[slotTabEntry].sExec;
        }
    }

    ASSERT(cyclic->nextAct<(nextTime+cyclic->sExec));
    cyclic->nextAct=nextTime+cyclic->sExec;
    ArmKTimer(&cyclic->kTimer, cyclic->nextAct, 0);
    slotTabEntry=plan->slotsOffset+cyclic->slot;
    if (newK&&newK->ctrl.g) {
        newK->ctrl.g->partCtrlTab->schedInfo.releasePoint=(xmcSchedCyclicSlotTab[slotTabEntry].flags&XM_PERIOD_START_SLOT)?1:0;
        newK->ctrl.g->partCtrlTab->schedInfo.noSlot=cyclic->slot;
        newK->ctrl.g->partCtrlTab->schedInfo.id=xmcSchedCyclicSlotTab[slotTabEntry].id;
        newK->ctrl.g->partCtrlTab->schedInfo.slotDuration=xmcSchedCyclicSlotTab[slotTabEntry].eExec-xmcSchedCyclicSlotTab[slotTabEntry].sExec;
        SetPartitionExtIrqPending(GetPartition(newK), XM_VT_EXT_CYCLIC_SLOT_START);
    }

    cyclic->kThread=newK;

    return newK;
} 

void SetSchedPending(void) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    info->cpu.irqNestingCounter|=SCHED_PENDING;
}

void Schedule(void) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xmWord_t hwFlags;
    kThread_t *newK;
    CHECK_KTHR_SANITY(info->sched.cKThread);
 
    if (!(info->sched.flags&LOCAL_SCHED_ENABLED)) {
        info->cpu.irqNestingCounter&=~(SCHED_PENDING);
        return;
    }

    HwSaveFlagsCli(hwFlags);
    // When an interrupt is in-progress, the scheduler shouldn't be invoked
    if (info->cpu.irqNestingCounter&IRQ_IN_PROGRESS) {
        info->cpu.irqNestingCounter|=SCHED_PENDING;
        HwRestoreFlags(hwFlags);
        return;
    }
 
    info->cpu.irqNestingCounter&=(~SCHED_PENDING);
    if (!(newK=GetReadyKThread(info->sched.data)))
        newK=info->sched.idleKThread;
    
    CHECK_KTHR_SANITY(newK);
    if (newK!=info->sched.cKThread) {
#if defined(CONFIG_DEV_LICE_INTERFACE) || defined(CONFIG_DEV_LICE_SCHEDULING)
        xm_u32_t mem_cfg1;
        mem_cfg1=set_iop_bit();
#endif

        SwitchKThreadArchPre(newK, info->sched.cKThread);
        if (info->sched.cKThread->ctrl.g) // not idle kthread
            StopVClock(&info->sched.cKThread->ctrl.g->vClock,  &info->sched.cKThread->ctrl.g->vTimer);

#ifdef CONFIG_DEV_LICE_SCHEDULING
        if (info->sched.cKThread->ctrl.g)
            LiceCmdEndPartition(GetPartitionCfg(info->sched.cKThread)->id, info->sched.cKThread->ctrl.g->partCtrlTab->schedInfo.id);
        else
            LiceCmdEndPartition(0xffff, 0xffff);
#endif
        if (newK->ctrl.g)
            SetHwTimer(TraverseKTimerQueue(&newK->ctrl.localActiveKTimers, GetSysClockUsec()));        
        info->sched.cKThread->ctrl.irqMask=HwIrqGetMask();
        HwIrqSetMask(newK->ctrl.irqMask);
#ifdef CONFIG_DEV_LICE_SCHEDULING
        if (newK->ctrl.g)
            LiceCmdStartPartition(GetPartitionCfg(newK)->id, newK->ctrl.g->partCtrlTab->schedInfo.id);
        else
            LiceCmdStartPartition(0xffff, 0xffff);
#endif
#ifdef CONFIG_AUDIT_EVENTS
        if (info->sched.cKThread->ctrl.g)
            RaiseAuditEvent(GetPartitionCfg(info->sched.cKThread)->id, XM_AUDIT_END_PARTITION);           
        else
            RaiseAuditEvent(XM_HYPERVISOR_ID, XM_AUDIT_END_IDLE);          

        if (newK->ctrl.g)
            RaiseAuditEvent(GetPartitionCfg(newK)->id, XM_AUDIT_BEGIN_PARTITION);
        else
            RaiseAuditEvent(XM_HYPERVISOR_ID, XM_AUDIT_BEGIN_IDLE); 
#endif
#ifdef CONFIG_DEV_LICE_INTERFACE
        LiceCmdBeginCS();
#endif
#ifdef CONFIG_AUDIT_EVENTS
        RaiseAuditEvent(XM_HYPERVISOR_ID, XM_AUDIT_BEGIN_CS); 
#endif
        /*
         * Check if XM Virtual Watchdog is exausted. If not exausted call
         * the action associated: reload the hw watchdog.
         * If the XM Virtual Watchdog is exahusted no action is done.
         *
         * The KWatchdog is initialized in this point if previously has not been
         * initialized.
         */
        if (IsKWatchdogExpired() == 0) /* No expired */
        {
            KWatchdogReloadHw();
        }

        CONTEXT_SWITCH(newK, &info->sched.cKThread);

#ifdef CONFIG_DEV_LICE_INTERFACE
        LiceCmdEndCS();
#endif
#ifdef CONFIG_AUDIT_EVENTS
        RaiseAuditEvent(XM_HYPERVISOR_ID, XM_AUDIT_END_CS); 
#endif
#if defined(CONFIG_DEV_LICE_INTERFACE) || defined(CONFIG_DEV_LICE_SCHEDULING)
        restore_iop_bit(mem_cfg1);
#endif
        if (info->sched.cKThread->ctrl.g)
            ResumeVClock(&info->sched.cKThread->ctrl.g->vClock, &info->sched.cKThread->ctrl.g->vTimer);
        SwitchKThreadArchPost(info->sched.cKThread);
    }
    HwRestoreFlags(hwFlags);
}
