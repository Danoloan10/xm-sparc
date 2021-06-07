/*
 * $FILE: hypercalls.c
 *
 * XM's hypercalls
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
  - [02/09/2015:SPR-020915-01] MISRA rules.9.1 violation fixed.
  - [02/09/2015:SPR-020915-02] 'continue' replaced with equivalent code.
  - [02/09/2015:SPR-280715-01] xmcBootTab removed.
  - [02/10/2015:SPR-021015-01] ResetPartitionSys() updates partition's 
    entry point.
  - [16/11/2015:SPR-290915-01] DivMod64 replaced a/b operation for 64-bit 
     integers
  - [20/11/2015:SPR-110915-01] RaiseIpviSys depends on IPVI_SUPPORT
  - [24/11/2015:SPR-241115-02] ResetSystem requires a hmLog (NULL).
  - [09/12/2015:SPR-031215-02] Entry point field removed from partition
  - [30/03/2016:CP-250216-01]  Ángel Esquinas (aesquinas@fentiss.com)
    Add ReloadWatchdogSys hypercall

  - 15/04/2016    Angel Esquinas (aesquinas@fentiss.com)
                  CP-160411-01:
                  - XM_RESET_COLD is used as initial reset status when reset
                  system is called.
*/

#include <assert.h>
#include <kthread.h>
#include <gaccess.h>
#include <physmm.h>
#include <processor.h>
#include <sched.h>
#include <stdc.h>
#include <hypercalls.h>
#include <virtmm.h>
#include <vmmap.h>
#include <kwatchdog.h>

#include <objects/status.h>
#include <objects/trace.h>

extern xm_u32_t resetStatusInit[];

__hypercall xm_s32_t HaltPartitionSys(xmId_t partitionId) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xmId_t pId=PartitionId2PId(partitionId);
    
    ASSERT(!HwIsSti());
    if (partitionId!=GetPartitionCfg(info->sched.cKThread)->rId) {
        if (!(GetPartitionCfg(info->sched.cKThread)->flags&XM_PART_SYSTEM))
            return XM_PERM_ERROR;
        if (pId>=xmcTab.noPartitions)
	    return XM_INVALID_PARAM;
        HALT_PARTITION(pId);
#ifdef CONFIG_SMP_SUPPORT
        xm_s32_t e;
        for (e=0; e<CONFIG_NO_VCPUS; e++) {
            xm_u8_t cpu=xmcVCpuTab[KID2PARTID(pId)*CONFIG_NO_VCPUS+e].cpu;
            if (cpu!=GET_CPU_ID()&&(cpu!=255)) {
                kThread_t *k=partitionTab[KID2PARTID(pId)].kThread[e];
                if (localProcessorInfo[cpu].sched.cKThread==k)
                    SendIpi(cpu, RESCHED_SYSTEM_IPI_MSG);
            }
        }
#endif
        return XM_OK;
    }

    HALT_PARTITION(pId);
    Schedule();
    return XM_OK;
}

__hypercall xm_s32_t HaltSystemSys(void) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    extern void HaltSystem(void);
    
    ASSERT(!HwIsSti());
    if (!(GetPartitionCfg(info->sched.cKThread)->flags&XM_PART_SYSTEM))
	return XM_PERM_ERROR;
#ifdef CONFIG_SMP_SUPPORT
    SendAllIpi(HALT_SYSTEM_IPI_MSG);
#endif
    HwCli();
    HaltSystem();
    return XM_OK;
}

// XXX: the ".data" section is restored during the initialisation
__hypercall xm_s32_t ResetSystemSys(xm_u32_t resetMode) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();

    ASSERT(!HwIsSti());
    if (!(GetPartitionCfg(info->sched.cKThread)->flags&XM_PART_SYSTEM))
    {
        return XM_PERM_ERROR;
    }
    if ((resetMode != XM_WARM_RESET) && (resetMode != XM_COLD_RESET))
    {
        return XM_INVALID_PARAM;
    }
    /* partitions are cold reset when reset system is produced */
    resetStatusInit[0] = XM_COLD_RESET;
    ResetSystem(resetMode, 0);
    return XM_OK;
}

__hypercall xm_s32_t SetCacheStateSys(xm_u32_t cache, xm_u32_t action) {
#ifdef CONFIG_ENABLE_CACHE
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
#endif
    ASSERT(!HwIsSti());
#ifdef CONFIG_ENABLE_CACHE
    if ((action<XM_ACTIVATE_CACHE)||(action>XM_FLUSH_CACHE))
	return XM_INVALID_PARAM;

    if ((cache!=XM_DCACHE)&&(cache!=XM_ICACHE))
        return XM_INVALID_PARAM;
    
    switch(action) {
    case XM_FLUSH_CACHE:
        if (cache&XM_DCACHE)
            SetKThreadFlags(info->sched.cKThread, KTHREAD_FLUSH_DCACHE_F);
        if (cache&XM_ICACHE)
            SetKThreadFlags(info->sched.cKThread, KTHREAD_FLUSH_ICACHE_F);
	return XM_OK;
        break;
    case XM_ACTIVATE_CACHE:
        if (cache&XM_DCACHE)
            SetKThreadFlags(info->sched.cKThread, KTHREAD_DCACHE_ENABLED_F);
        if (cache&XM_ICACHE)
            SetKThreadFlags(info->sched.cKThread, KTHREAD_ICACHE_ENABLED_F);
        SetKThreadFlags(info->sched.cKThread, KTHREAD_FLUSH_DCACHE_F|KTHREAD_FLUSH_ICACHE_F);
        break;
    case XM_DEACTIVATE_CACHE:
        if (cache&XM_DCACHE)
            ClearKThreadFlags(info->sched.cKThread, KTHREAD_DCACHE_ENABLED_F);
        if (cache&XM_ICACHE)
            ClearKThreadFlags(info->sched.cKThread, KTHREAD_ICACHE_ENABLED_F);
        SetKThreadFlags(info->sched.cKThread, KTHREAD_FLUSH_DCACHE_F|KTHREAD_FLUSH_ICACHE_F);
        break;
    }
    info->sched.cKThread->ctrl.g->partCtrlTab->flags&=~(PARTITION_DCACHE_ENABLED_F|PARTITION_ICACHE_ENABLED_F);
    info->sched.cKThread->ctrl.g->partCtrlTab->flags|=(PARTITION_DCACHE_ENABLED_F|PARTITION_ICACHE_ENABLED_F)&info->sched.cKThread->ctrl.flags;
    return XM_OK;
#else
    return XM_INVALID_CONFIG;
#endif
}

__hypercall xm_s32_t SwitchSchedPlanSys(xm_u32_t newPlanId, xm_u32_t *__gParam currentPlanId) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();

    ASSERT(!HwIsSti());
    if (!(GetPartitionCfg(info->sched.cKThread)->flags&XM_PART_SYSTEM))
        return XM_PERM_ERROR;

    if (CheckGParam(currentPlanId, sizeof(xm_u32_t), 4, PFLAG_RW|PFLAG_NOT_NULL)<0)
        return XM_INVALID_PARAM;

    if (info->sched.data->plan.current->id==newPlanId)
        return XM_NO_ACTION;

    if ((newPlanId<1)||(newPlanId>=xmcTab.hpv.cpuTab[GET_CPU_ID()].noSchedCyclicPlans)) return XM_INVALID_PARAM;
    
    if (SwitchSchedPlan(newPlanId, currentPlanId))
        return XM_INVALID_CONFIG;

#ifdef CONFIG_SMP_SUPPORT
    SendAllIpi(RESCHED_SYSTEM_IPI_MSG);
#endif

    return XM_OK;
}

__hypercall xm_s32_t SuspendPartitionSys(xmId_t partitionId) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xmId_t pId=PartitionId2PId(partitionId);
    ASSERT(!HwIsSti());
    if (partitionId!=GetPartitionCfg(info->sched.cKThread)->rId) {
        if (!(GetPartitionCfg(info->sched.cKThread)->flags&XM_PART_SYSTEM))
            return XM_PERM_ERROR;
        if (pId>=xmcTab.noPartitions)
	    return XM_INVALID_PARAM;
        
        SUSPEND_PARTITION(pId); 
#ifdef CONFIG_SMP_SUPPORT
        xm_s32_t e;
        for (e=0; e<CONFIG_NO_VCPUS; e++) {
            xm_u8_t cpu=xmcVCpuTab[KID2PARTID(pId)*CONFIG_NO_VCPUS+e].cpu;
            if (cpu!=GET_CPU_ID()&&(cpu!=255)) {
                kThread_t *k=partitionTab[KID2PARTID(pId)].kThread[e];
                if (localProcessorInfo[cpu].sched.cKThread==k)
                    SendIpi(cpu, RESCHED_SYSTEM_IPI_MSG);
            }
        }
#endif

        return XM_OK;
    }
    SUSPEND_PARTITION(pId);
    Schedule();
    return XM_OK;
}

__hypercall xm_s32_t ResumePartitionSys(xmId_t partitionId) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xmId_t pId=PartitionId2PId(partitionId);
    ASSERT(!HwIsSti());
    if (partitionId!=GetPartitionCfg(info->sched.cKThread)->rId) {
        if (!(GetPartitionCfg(info->sched.cKThread)->flags&XM_PART_SYSTEM))
            return XM_PERM_ERROR;
        if (pId>=xmcTab.noPartitions)
	    return XM_INVALID_PARAM;
        
        RESUME_PARTITION(pId);
#ifdef CONFIG_SMP_SUPPORT
        xm_s32_t e;
        for (e=0; e<CONFIG_NO_VCPUS; e++) {
            xm_u8_t cpu=xmcVCpuTab[KID2PARTID(pId)*CONFIG_NO_VCPUS+e].cpu;
            if (cpu!=GET_CPU_ID()&&(cpu!=255)) {
                kThread_t *k=partitionTab[KID2PARTID(pId)].kThread[e];
                if (localProcessorInfo[cpu].sched.cKThread==k)
                    SendIpi(cpu, RESCHED_SYSTEM_IPI_MSG);
            }
        }
#endif
        return XM_OK;
    }
    RESUME_PARTITION(pId);
    Schedule();
    return XM_OK;
}

__hypercall xm_s32_t ResetPartitionSys(xmId_t partitionId, xmAddress_t entryPoint, xm_u32_t resetMode, xm_u32_t status) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xmId_t pId = PartitionId2PId(partitionId);
	xm_u32_t i = 0;
    ASSERT(!HwIsSti());

    if ((resetMode!=XM_WARM_RESET)&&(resetMode!=XM_COLD_RESET))
        return XM_INVALID_PARAM;
    if (partitionId!=GetPartitionCfg(info->sched.cKThread)->rId) {
        if (!(GetPartitionCfg(info->sched.cKThread)->flags&XM_PART_SYSTEM))
            return XM_PERM_ERROR;
        if (pId>=xmcTab.noPartitions)
	    return XM_INVALID_PARAM;
    }

    for (i=0; i<xmcPartitionTab[pId].noPhysicalMemoryAreas; i++) {
        if ((entryPoint >= xmcPhysMemAreaTab[i+xmcPartitionTab[pId].physicalMemoryAreasOffset].mappedAt) &&
            (entryPoint < (xmcPhysMemAreaTab[i+xmcPartitionTab[pId].physicalMemoryAreasOffset].mappedAt
            + xmcPhysMemAreaTab[i+xmcPartitionTab[pId].physicalMemoryAreasOffset].size)))
            break;
    }

    if ((i >= xmcPartitionTab[pId].noPhysicalMemoryAreas) || 
        ((xm_u32_t)entryPoint&0x7)) {
        entryPoint = xmcPartitionTab[pId].entryPoint;
    }
    else
    {
        xmcPartitionTab[pId].entryPoint = entryPoint;
    }

    HALT_PARTITION(pId);
    ResetPartition(&partitionTab[pId], entryPoint, (resetMode==XM_COLD_RESET)?1:0, status);
#ifdef CONFIG_SMP_SUPPORT
    xm_s32_t e;
    for (e=0; e<CONFIG_NO_VCPUS; e++) {
        xm_u8_t cpu=xmcVCpuTab[KID2PARTID(pId)*CONFIG_NO_VCPUS+e].cpu;
        if (cpu!=GET_CPU_ID()&&(cpu!=255)) {
            kThread_t *k = partitionTab[KID2PARTID(pId)].kThread[e];
            if (localProcessorInfo[cpu].sched.cKThread==k)
                SendIpi(cpu, RESCHED_SYSTEM_IPI_MSG);
        }
    }
#endif
    return XM_OK;
}

__hypercall xm_s32_t ShutdownPartitionSys(xmId_t partitionId) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xmId_t pId=PartitionId2PId(partitionId);
    ASSERT(!HwIsSti());
if (partitionId!=GetPartitionCfg(info->sched.cKThread)->rId) {
        if (!(GetPartitionCfg(info->sched.cKThread)->flags&XM_PART_SYSTEM))
            return XM_PERM_ERROR;
	if (pId>=xmcTab.noPartitions)
	    return XM_INVALID_PARAM;
    }

    SHUTDOWN_PARTITION(pId);
#ifdef CONFIG_SMP_SUPPORT
    xm_s32_t e;
    for (e=0; e<CONFIG_NO_VCPUS; e++) {
        xm_u8_t cpu=xmcVCpuTab[KID2PARTID(pId)*CONFIG_NO_VCPUS+e].cpu;
        if (cpu!=GET_CPU_ID()&&(cpu!=255)) {
            kThread_t *k=partitionTab[KID2PARTID(pId)].kThread[e];
            if (localProcessorInfo[cpu].sched.cKThread==k)
                SendIpi(cpu, RESCHED_SYSTEM_IPI_MSG);
        }
    }
#endif
    return XM_OK;
}

__hypercall xm_s32_t IdleSelfSys(void) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    ASSERT(!HwIsSti());
//    ClearKThreadFlags(info->sched.cKThread, KTHREAD_READY_F);
    SchedYield(info, info->sched.idleKThread);
    return XM_OK;
}

__hypercall xm_s32_t SetTimerSys(xm_u32_t clockId, xmTime_t abstime, xmTime_t interval) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xm_s32_t ret=XM_OK;
    xmTime_t cTime;
    xmTime_t minInterval = info->time.sysHwTimer->GetMinInterval();
    ASSERT(!HwIsSti());
    if ((abstime<0LL)||(interval<0LL)||((interval>0LL)&&(interval<minInterval)))
	return XM_INVALID_PARAM;

    // Disarming a timer
    if (!abstime) {
	switch(clockId) {
	case XM_HW_CLOCK:
	    DisarmKTimer(&info->sched.cKThread->ctrl.g->kTimer);
	    return XM_OK;
	case XM_EXEC_CLOCK:
	    DisarmVTimer(&info->sched.cKThread->ctrl.g->vTimer, &info->sched.cKThread->ctrl.g->vClock);
	    return XM_OK;
	default:
	    return XM_INVALID_PARAM;
	}
    }
    // Arming a timer
    if (interval!=0){
        cTime=GetSysClockUsec();
        if ((abstime+interval)<cTime){
            abstime+=interval*DivMod64(cTime-abstime, interval, 0);
        }
    }
    switch(clockId) {
    case XM_HW_CLOCK:
	ret=ArmKTimer(&info->sched.cKThread->ctrl.g->kTimer, abstime, interval);
	break;
    case XM_EXEC_CLOCK:
	ret=ArmVTimer(&info->sched.cKThread->ctrl.g->vTimer, &info->sched.cKThread->ctrl.g->vClock, abstime, interval);
	break;
    default:
	return XM_INVALID_PARAM;
    }

    return ret;
}

__hypercall xm_s32_t GetTimeSys(xm_u32_t clockId, xmTime_t *__gParam time) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    if (CheckGParam(time, sizeof(xm_s64_t), 8, PFLAG_RW|PFLAG_NOT_NULL)<0)
        return XM_INVALID_PARAM;

    switch(clockId) {
    case XM_HW_CLOCK:
	*time=GetSysClockUsec();
	break;       
    case XM_EXEC_CLOCK:
	*time=GetTimeUsecVClock(&info->sched.cKThread->ctrl.g->vClock);
	break;
    default:
	return XM_INVALID_PARAM;
    }
    return XM_OK;
}

/*
  __hypercall xm_s32_t SetIrqLevelSys(xm_u32_t level) {
  localSched_t *sched=GET_LOCAL_SCHED();
  ASSERT(!HwIsSti());
    
    
  //info->sched.cKThread->ctrl.g->partCtrlTab->iFlags&=~IFLAGS_IRQ_MASK;
  //info->sched.cKThread->ctrl.g->partCtrlTab->iFlags|=level&IFLAGS_IRQ_MASK;

  return XM_OK;
  }
*/

__hypercall xm_s32_t ClearIrqMaskSys(xm_u32_t hwIrqsMask, xm_u32_t extIrqsMask) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    struct xmcPartition *cfg=GetPartitionCfg(info->sched.cKThread);
    xm_u32_t unmasked;
    xm_s32_t e;
    ASSERT(!HwIsSti());
    info->sched.cKThread->ctrl.g->partCtrlTab->hwIrqsMask&=~hwIrqsMask;
    info->sched.cKThread->ctrl.g->partCtrlTab->extIrqsMask&=~extIrqsMask;
    unmasked=hwIrqsMask&cfg->hwIrqs;
    for (e=0; unmasked; e++)
        if (unmasked&(1<<e)) {
            HwEnableIrq(e);
            unmasked&=~(1<<e);
        }

    return XM_OK;
}

__hypercall xm_s32_t SetIrqMaskSys(xm_u32_t hwIrqsMask, xm_u32_t extIrqsMask) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    struct xmcPartition *cfg=GetPartitionCfg(info->sched.cKThread);
    xm_u32_t masked;
    xm_s32_t e;
    ASSERT(!HwIsSti());
    info->sched.cKThread->ctrl.g->partCtrlTab->hwIrqsMask|=hwIrqsMask;
    info->sched.cKThread->ctrl.g->partCtrlTab->extIrqsMask|=extIrqsMask;
    masked=hwIrqsMask&cfg->hwIrqs;
    for (e=0; masked; e++)
        if (masked&(1<<e)) {
            HwDisableIrq(e);
            masked&=~(1<<e);
        }
    return XM_OK;
}

__hypercall xm_s32_t ForceIrqsSys(xm_u32_t hwIrqMask, xm_u32_t extIrqMask) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    struct xmcPartition *cfg=GetPartitionCfg(info->sched.cKThread);
    xm_u32_t forced;
    xm_s32_t e;

    ASSERT(!HwIsSti());
    
//    info->sched.cKThread->ctrl.g->partCtrlTab->hwIrqsPend|=hwIrqMask;
    info->sched.cKThread->ctrl.g->partCtrlTab->extIrqsPend|=extIrqMask;
    forced=hwIrqMask&cfg->hwIrqs;
    hwIrqMask&=~forced;
    for (e=0; forced; e++)
        if (forced&(1<<e)) {
            HwForceIrq(e);
            forced&=~(1<<e);
        }
    for (e=0; hwIrqMask; e++)
        if (hwIrqMask&(1<<e)) {
            info->sched.cKThread->ctrl.g->partCtrlTab->hwIrqsPend|=(1<<e);
            hwIrqMask&=~(1<<e);
        }
    return XM_OK;
}

__hypercall xm_s32_t ClearIrqsSys(xm_u32_t hwIrqMask, xm_u32_t extIrqMask) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    struct xmcPartition *cfg=GetPartitionCfg(info->sched.cKThread);
    xm_u32_t pending;
    xm_s32_t e;

    ASSERT(!HwIsSti());
    info->sched.cKThread->ctrl.g->partCtrlTab->hwIrqsPend&=~hwIrqMask;
    info->sched.cKThread->ctrl.g->partCtrlTab->extIrqsPend&=~extIrqMask;
    pending=hwIrqMask&cfg->hwIrqs;
    for (e=0; pending; e++)
        if (pending&(1<<e)) {
            HwClearIrq(e);
            pending&=~(1<<e);
        }
    return XM_OK;
}

__hypercall xm_s32_t RouteIrqSys(xm_u32_t type, xm_u32_t irq, xm_u16_t vector) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    ASSERT(!HwIsSti());
    
    if ((irq>=32)||(vector>=256))
        return XM_INVALID_PARAM;
    switch(type) {
    case XM_TRAP_TYPE:
        info->sched.cKThread->ctrl.g->partCtrlTab->trap2Vector[irq]=vector;
        break;
    case XM_HWIRQ_TYPE:
        info->sched.cKThread->ctrl.g->partCtrlTab->hwIrq2Vector[irq]=vector;
        break;
    case XM_EXTIRQ_TYPE:
        info->sched.cKThread->ctrl.g->partCtrlTab->extIrq2Vector[irq]=vector;
        break;
    default:
        return XM_INVALID_PARAM;
    }

    return XM_OK;
}

__hypercall xm_s32_t ReadObjectSys(xmObjDesc_t objDesc, void *__gParam buffer, xmSize_t size, xm_u32_t *__gParam flags) {
    xm_s32_t class;

    ASSERT(!HwIsSti());
    
    class=OBJDESC_GET_CLASS(objDesc);
    if (objectTab[class]&&objectTab[class]->Read)
	return objectTab[class]->Read(objDesc, buffer, size, flags);
    
    return XM_INVALID_PARAM;
}

__hypercall xm_s32_t WriteObjectSys(xmObjDesc_t objDesc, void *__gParam buffer, xmSize_t size, xm_u32_t *__gParam flags) {
    xm_s32_t class;
    ASSERT(!HwIsSti());
    class=OBJDESC_GET_CLASS(objDesc);
    if (objectTab[class]&&objectTab[class]->Write)
	return objectTab[class]->Write(objDesc, buffer, size, flags);
    
    return XM_INVALID_PARAM;
}

__hypercall xm_s32_t SeekObjectSys(xmObjDesc_t objDesc, xmAddress_t offset, xm_u32_t whence) {
    xm_s32_t class;

    ASSERT(!HwIsSti());
    
    class=OBJDESC_GET_CLASS(objDesc);
    if (objectTab[class]&&objectTab[class]->Seek)
	return objectTab[class]->Seek(objDesc, offset, whence);

    return XM_INVALID_PARAM;
}

__hypercall xm_s32_t CtrlObjectSys(xmObjDesc_t objDesc, xm_u32_t cmd, void *__gParam arg) {
    xm_s32_t class;

    ASSERT(!HwIsSti());
    class=OBJDESC_GET_CLASS(objDesc);
    if (objectTab[class]&&objectTab[class]->Ctrl)
	return objectTab[class]->Ctrl(objDesc, cmd, arg);
    return XM_INVALID_PARAM;
}

#ifdef CONFIG_IPVI_SUPPORT
__hypercall xm_s32_t RaiseIpviSys(xm_u8_t noIpvi) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    struct xmcPartIpvi *ipvi = NULL;
    xm_s32_t e, vcpu;
    kThread_t *k;

    ASSERT(!HwIsSti());
    if ((noIpvi<XM_VT_EXT_IPVI0)||(noIpvi>=XM_VT_EXT_IPVI0+CONFIG_MAX_NO_IPVI)) 
	return XM_INVALID_PARAM;
    ipvi=&GetPartitionCfg(info->sched.cKThread)->ipviTab[noIpvi-XM_VT_EXT_IPVI0];
    if (ipvi->noDsts<=0)
        return XM_NO_ACTION;

    for (e=0; e<ipvi->noDsts; e++){
        partition_t *p=&partitionTab[xmcDstIpvi[ipvi->dstOffset+e]];
        SetPartitionExtIrqPending(p, noIpvi);
        for (vcpu=0; vcpu<CONFIG_NO_VCPUS; vcpu++) {
            k=p->kThread[vcpu];
            if (!AreKThreadFlagsSet(k, KTHREAD_HALTED_F)) {
#ifdef CONFIG_SMP_SUPPORT
                xm_u8_t cpu=xmcVCpuTab[(KID2PARTID(k->ctrl.g->id)*CONFIG_NO_VCPUS)+KID2VCPUID(k->ctrl.g->id)].cpu;
                if (cpu!=GET_CPU_ID()&&(cpu!=255)) {
                    if (localProcessorInfo[cpu].sched.cKThread==k) {
                        SendIpi(cpu, RESCHED_SYSTEM_IPI_MSG);
                    }
                }
#endif
            }
        }
    }
    return XM_OK;
}
#endif

__hypercall xm_s32_t GetGidByName(xm_u8_t *__gParam name, xm_u32_t entity) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    struct xmcPartition *cfg=GetPartitionCfg(info->sched.cKThread);
    xm_s32_t e, id=XM_INVALID_CONFIG;

    if (CheckGParam(name, CONFIG_ID_STRING_LENGTH, 1, PFLAG_NOT_NULL)<0) 
	return XM_INVALID_PARAM;

    switch(entity) {
    case XM_PARTITION_NAME:
        for (e=0; e<xmcTab.noPartitions; e++)
            if (!strncmp(&xmcStringTab[xmcPartitionTab[e].nameOffset], name, CONFIG_ID_STRING_LENGTH))  {
                id=xmcPartitionTab[e].id;
                if (id!=cfg->id)
                    if (!(GetPartitionCfg(info->sched.cKThread)->flags&XM_PART_SYSTEM))
                        return XM_PERM_ERROR;
                break;
            }        
        break;
    case XM_PLAN_NAME:
        for (e=0; e<xmcTab.noSchedCyclicPlans; e++)
            if (!strncmp(&xmcStringTab[xmcSchedCyclicPlanTab[e].nameOffset], name, CONFIG_ID_STRING_LENGTH))  {
                id=xmcSchedCyclicPlanTab[e].id;
                break;
            }
        break;
    default:
        return XM_INVALID_PARAM;
    }
    
    return id;
}

__hypercall xm_s32_t GetVCpuIdSys(void) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    return KID2VCPUID(info->sched.cKThread->ctrl.g->id);
}

__hypercall xm_s32_t HaltVCpuSys(xmId_t vCpuId) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    kThread_t *k;
    
    ASSERT(!HwIsSti());
    if (vCpuId>=CONFIG_NO_VCPUS)
        return XM_INVALID_PARAM;
    k=GetPartition(info->sched.cKThread)->kThread[vCpuId];
    
    HALT_VCPU(KID2PARTID(k->ctrl.g->id), KID2VCPUID(k->ctrl.g->id));
    if (k==info->sched.cKThread)
        Schedule();
#ifdef CONFIG_SMP_SUPPORT
    else {
        xm_u8_t cpu=xmcVCpuTab[CONFIG_NO_VCPUS*KID2PARTID(k->ctrl.g->id)+vCpuId].cpu;
        if (cpu!=GET_CPU_ID()&&(cpu!=255))
            if (localProcessorInfo[cpu].sched.cKThread==k)
                SendIpi(cpu, RESCHED_SYSTEM_IPI_MSG);
    }
#endif
    return XM_OK;
}

__hypercall xm_s32_t ResetVCpuSys(xmId_t vCpuId, xmAddress_t entryPoint) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    partition_t *partition=GetPartition(info->sched.cKThread);

    if (vCpuId>=CONFIG_NO_VCPUS)
        return XM_INVALID_PARAM;

    if (CheckGParam((void *)entryPoint, CONFIG_ID_STRING_LENGTH, 8, PFLAG_NOT_NULL)<0) 
	return XM_INVALID_PARAM;

    HALT_VCPU(partition->cfg->id, vCpuId);
    ResetKThread(partition->kThread[vCpuId], entryPoint);

#ifdef CONFIG_SMP_SUPPORT
    xm_u8_t cpu=xmcVCpuTab[CONFIG_NO_VCPUS*KID2PARTID(partition->kThread[vCpuId]->ctrl.g->id)+vCpuId].cpu;
    if ((cpu!=GET_CPU_ID())&&(cpu!=255))
        if (localProcessorInfo[cpu].sched.cKThread==partition->kThread[vCpuId])
            SendIpi(cpu, RESCHED_SYSTEM_IPI_MSG);
#endif
    return XM_OK;
}

/*
 *
 */
__hypercall xm_s32_t ReloadWatchdogSys(xmTime_t budget)
{
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xm_s32_t ret = XM_OK;

    ASSERT(!HwIsSti());

    if (!(GetPartitionCfg(info->sched.cKThread)->flags&XM_PART_SYSTEM))
    {
        ret = XM_PERM_ERROR;
    }
    else if(budget < 0)
    {
        ret = XM_INVALID_PARAM;
    }
    else
    {
        KWatchdogSetBudget(budget);
    }
    return ret;
}
