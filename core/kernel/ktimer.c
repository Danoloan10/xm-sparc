/*
 * $FILE: ktimers.c
 *
 * XM's timer interface
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:  
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 *
 */
/*
  Changelog:
  - [02/09/2015:SPR-020915-04] Remove VBOOT macro and boot.h
*/
#include <assert.h>
#include <rsvmem.h>
#include <ktimer.h>
#include <sched.h>
#include <spinlock.h>
#include <smp.h>
#include <stdc.h>

#include <local.h>

static xm_s32_t TimerHandler(void);

inline void SetHwTimer(xmTime_t nextAct) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xmTime_t nextTime, cTime;

    ASSERT(!HwIsSti());
    ASSERT(nextAct>=0);
    if (!nextAct) return;
    if ((info->time.flags&NEXT_ACT_IS_VALID)&&(nextAct>=info->time.nextAct))
	return;
    info->time.flags|=NEXT_ACT_IS_VALID;
    info->time.nextAct=nextAct;
    cTime=GetSysClockUsec();
    nextTime=nextAct-cTime;
    if (nextTime>=0) {
        if (nextTime<info->time.sysHwTimer->GetMinInterval())
            nextTime=info->time.sysHwTimer->GetMinInterval();
        
        if (nextTime>info->time.sysHwTimer->GetMaxInterval())
            nextTime=info->time.sysHwTimer->GetMaxInterval();
        info->time.nextAct=nextTime+cTime;
        info->time.sysHwTimer->SetHwTimer(nextTime);
    } else {
        info->time.sysHwTimer->SetHwTimer(1);
    }
}

xmTime_t TraverseKTimerQueue(struct dynList *l, xmTime_t cTime) {
    xmTime_t nextAct=MAX_XMTIME;
    kTimer_t *kTimer;

    DYNLIST_FOR_EACH_ELEMENT_BEGIN(l, kTimer, 1) {
	ASSERT(kTimer);
	if (kTimer->flags&KTIMER_ARMED){
	    if (kTimer->value<=cTime) {
                if (kTimer->Action)
		    kTimer->Action(kTimer, kTimer->actionArgs);
		if (kTimer->interval>0) {
		    // To be optimised
		    do {
			kTimer->value+=kTimer->interval;
		    } while(kTimer->value<=cTime);
		    if (nextAct>kTimer->value)
			nextAct=kTimer->value;
		} else
		    kTimer->flags&=~KTIMER_ARMED;
	    } else {
		if (nextAct>kTimer->value)
		    nextAct=kTimer->value;
	    }
	}
    } DYNLIST_FOR_EACH_ELEMENT_END(l);
    return nextAct;
}

static xm_s32_t TimerHandler(void) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xmTime_t cTime, nextAct, nLocalAct;

    ASSERT(!HwIsSti());
    info->time.flags&=~NEXT_ACT_IS_VALID;
    cTime=GetSysClockUsec();
    nextAct=TraverseKTimerQueue(&info->time.globalActiveKTimers, cTime);

    if (info->sched.cKThread->ctrl.g)
	if ((nLocalAct=TraverseKTimerQueue(&info->sched.cKThread->ctrl.localActiveKTimers, cTime))&& (nLocalAct<nextAct))
	    nextAct=nLocalAct;
    SetHwTimer(nextAct);
    return 0;
}

void InitKTimer(xm_s32_t cpuId, kTimer_t *kTimer, void (*Act)(kTimer_t *, void *), void *args, void *kThread) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    kThread_t *k=(kThread_t *)kThread;

    memset((xm_s8_t *)kTimer, 0, sizeof(kTimer_t));
    kTimer->actionArgs=args;
    kTimer->Action=Act;
    if(DynListInsertHead((k)?&k->ctrl.localActiveKTimers:&info->time.globalActiveKTimers, &kTimer->dynListPtrs)) {
        cpuCtxt_t ctxt;
        GetCpuCtxt(&ctxt);
	SystemPanic(&ctxt, "[KTIMER] Error allocating ktimer");
    }
}

xm_s32_t ArmKTimer(kTimer_t *kTimer, xmTime_t value, xmTime_t interval) {
    ASSERT(!HwIsSti());
    ASSERT(kTimer);    
    kTimer->value=value;
    kTimer->interval=interval;
    kTimer->flags|=KTIMER_ARMED;
    SetHwTimer(value);
    return 0;
}

xm_s32_t DisarmKTimer(kTimer_t *kTimer) {
    ASSERT(kTimer);
    if (!(kTimer->flags&KTIMER_ARMED))
	return -1;
    kTimer->flags&=~KTIMER_ARMED;
    return 0;
}

static void VTimerHndl(kTimer_t *kTimer, void *args) {
    kThread_t *k=(kThread_t *)args;
    ASSERT(k->ctrl.g->vClock.flags&VCLOCK_ENABLED);
    ASSERT(k->ctrl.g->vTimer.flags&VTIMER_ARMED);
    CHECK_KTHR_SANITY(k);
    if (k->ctrl.g->vTimer.interval>0)
	k->ctrl.g->vTimer.value+=k->ctrl.g->vTimer.interval;
    else
	k->ctrl.g->vTimer.flags&=~VTIMER_ARMED;

    SetPartitionExtIrqPending(GetPartition(k), XM_VT_EXT_EXEC_TIMER);
}

xm_s32_t InitVTimer(xm_s32_t vCpuId, vTimer_t *vTimer, void *k) {
    InitKTimer(vCpuId, &vTimer->kTimer, VTimerHndl, k, k);
    return 0;
}

xm_s32_t ArmVTimer(vTimer_t *vTimer, vClock_t *vClock, xmTime_t value, xmTime_t interval) {
    vTimer->value=value;
    
    vTimer->interval=interval;
    vTimer->flags|=VTIMER_ARMED;
    if (vClock->flags&VCLOCK_ENABLED)
	ArmKTimer(&vTimer->kTimer, value-vClock->acc+vClock->delta, interval);

    return 0;
}

xm_s32_t DisarmVTimer(vTimer_t *vTimer, vClock_t *vClock) {
    if (!(vTimer->flags&VTIMER_ARMED))
	return -1;
    vTimer->flags&=~VTIMER_ARMED;
    DisarmKTimer(&vTimer->kTimer);
    return 0;
}

xm_s32_t SetupKTimers(void) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    DynListInit(&info->time.globalActiveKTimers);
    info->time.sysHwTimer->SetTimerHandler(TimerHandler);
    return 0;
}

void SetupSysClock(void) {
    if (!sysHwClock||(sysHwClock->InitClock()<0)) {
        cpuCtxt_t ctxt;
        GetCpuCtxt(&ctxt);
	SystemPanic(&ctxt, "No system clock available\n");
    }

    kprintf(">> HWClocks [%s (%dKhz)]\n", sysHwClock->name, sysHwClock->freqKhz);
}

void SetupHwTimer(void) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    if (!(info->time.sysHwTimer=GetSysHwTimer())||(info->time.sysHwTimer->InitHwTimer()<0)) {
        cpuCtxt_t ctxt;
        GetCpuCtxt(&ctxt);
	SystemPanic(&ctxt, "No hwTimer available\n");
    }

    if ((GET_NRCPUS()>1)&&!(info->time.sysHwTimer->flags&PER_CPU)) {
        cpuCtxt_t ctxt;
        GetCpuCtxt(&ctxt);
	SystemPanic(&ctxt, "No hwTimer available\n");
    }

    kprintf(">> HwTimer [%s (%dKhz)]\n", info->time.sysHwTimer->name, info->time.sysHwTimer->freqKhz);
}
