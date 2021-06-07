/*
 * $FILE: hm.c
 *
 * Health Monitor
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
  - [02/09/2015:SPR-020915-04] Remove VBOOT macro and boot.h
*/

#include <assert.h>
#include <rsvmem.h>
#include <hypercalls.h>
#include <kthread.h>
#include <stdc.h>
#include <sched.h>
#include <objects/status.h>

xmSystemStatus_t systemStatus;
xmPartitionStatus_t *partitionStatus;

static xm_s32_t CtrlStatus(xmObjDesc_t desc, xm_u32_t cmd, union statusCmd *__gParam args) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    extern xm_u32_t sysResetCounter[];
    extern xm_u32_t sysResetStatus[];
    xmId_t partId, pId;
    partId=OBJDESC_GET_PARTITIONID(desc);
    pId=PartitionId2PId(partId);
    if (partId!=GetPartitionCfg(info->sched.cKThread)->rId)
	if (!(GetPartitionCfg(info->sched.cKThread)->flags&XM_PART_SYSTEM))
	    return XM_PERM_ERROR;
    
    if (CheckGParam(args, sizeof(union statusCmd), 4, PFLAG_NOT_NULL|PFLAG_RW)<0) 
	return XM_INVALID_PARAM;
    switch(cmd) {
    case XM_GET_SYSTEM_STATUS:
	if (pId==XM_HYPERVISOR_ID) {
	    systemStatus.resetCounter=sysResetCounter[0];
            systemStatus.resetStatus=sysResetStatus[0];
	    memcpy(&args->status.system, &systemStatus, sizeof(xmSystemStatus_t));
	} else {
            xm_s32_t e;
	    if ((pId<0)||(pId>=xmcTab.noPartitions))
		return XM_INVALID_PARAM;
            
            for (e=0; e<CONFIG_NO_VCPUS; e++) {
                if (AreKThreadFlagsSet(partitionTab[pId].kThread[e], KTHREAD_HALTED_F)) {
                    partitionStatus[pId].state[e]=XM_STATUS_HALTED;
                } else if (AreKThreadFlagsSet(partitionTab[pId].kThread[e], KTHREAD_SUSPENDED_F)) {
                    partitionStatus[pId].state[e]=XM_STATUS_SUSPENDED;
                } else  if (AreKThreadFlagsSet(partitionTab[pId].kThread[e], KTHREAD_READY_F))
                    partitionStatus[pId].state[e]=XM_STATUS_READY;
                else
                    partitionStatus[pId].state[e]=XM_STATUS_IDLE;            
                partitionStatus[pId].execClock[e]=GetTimeUsecVClock(&partitionTab[pId].kThread[e]->ctrl.g->vClock);
            }
            partitionStatus[pId].resetCounter=partitionTab[pId].kThread[0]->ctrl.g->resetCounter;
            partitionStatus[pId].resetStatus=partitionTab[pId].kThread[0]->ctrl.g->resetStatus;
            memcpy(&args->status.partition, &partitionStatus[pId], sizeof(xmPartitionStatus_t));
#ifdef CONFIG_OBJ_STATUS_ACC
            /* Interrupts belonging to the partition */
            for (e=0; e<CONFIG_NO_HWIRQS; e++)
                if ((1<<e)&GetPartitionCfg(info->sched.cKThread)->hwIrqs)
                    args->status.partition.irqLastOccurence[e] = systemStatus.irqLastOccurence[e];
#ifdef CONFIG_PLAN_EXTSYNC
            e=xmcSchedCyclicPlanTab[xmcTab.hpv.cpuTab[GET_CPU_ID()].schedCyclicPlansOffset].extSync;
            if (e!=-1)
                args->status.partition.irqLastOccurence[e] = systemStatus.irqLastOccurence[e];
#endif
#endif
	}	
	return XM_OK;
    case XM_GET_SCHED_PLAN_STATUS:
        args->status.plan.switchTime=info->sched.data->planSwitchTime;
        args->status.plan.next=info->sched.data->plan.new->id;
        args->status.plan.current=info->sched.data->plan.current->id;
        if (info->sched.data->plan.prev)
            args->status.plan.prev=info->sched.data->plan.prev->id;
        else
            args->status.plan.prev=-1;
        return XM_OK;
    }
    return XM_INVALID_PARAM;
}

static const struct object statusObj={
    .Ctrl=(ctrlObjOp_t)CtrlStatus,
};

xm_s32_t SetupStatus(void) {
    GET_MEMZ(partitionStatus, sizeof(xmPartitionStatus_t)*xmcTab.noPartitions);
    objectTab[OBJ_CLASS_STATUS]=&statusObj;

    return 0;
}


REGISTER_OBJ(SetupStatus);


