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
  - [02/09/2015:SPR-280715-01] xmcBootTab removed.
  - [24/11/2015:SPR-241115-02] Passed the hmLog to ResetSystem.

  - 15/04/2016    Angel Esquinas (aesquinas@fentiss.com)
                  CP-160411-01: XM_RESET_COLD and XM_RESET_WARM are used instead
                  the HM event as values for reset status.

  - [2017/02/02]    Angel Esquinas
    SPR-170201-01: XM_RESET_COLD is used as resetStatus init when an HM event
    reset the system.
*/
#include <assert.h>
#include <checksum.h>
#include <kthread.h>
#include <hypercalls.h>
#include <rsvmem.h>
#include <stdc.h>
#include <xmconf.h>
#include <sched.h>
#include <objects/hm.h>
#include <queue.h>
#include <xmconf.h>
#ifdef CONFIG_OBJ_STATUS_ACC
#include <objects/status.h>
#endif

static struct queue sysHmLog;
extern xm_u32_t resetStatusInit[];
static xm_s32_t hmInit=0;

static xm_s32_t WriteHmLog(xmObjDesc_t desc, xm_u32_t *__gParam logId, xm_u32_t noLogs) {
    xmHmLog_t log;
    xm_s32_t e;
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    struct xmcPartition *cfg=GetPartitionCfg(info->sched.cKThread);
    if (OBJDESC_GET_PARTITIONID(desc)!=cfg->rId)
         return XM_INVALID_PARAM;
    if (!logId||!noLogs)
	return XM_INVALID_PARAM;
    /* SPR-130115-07 */ /*SPR-280115-01*/
    for (e=0; e<noLogs; e++) { 
        if ((logId[e] < (XM_HM_MAX_GENERIC_EVENTS+XM_HM_MAX_ARCH_EVENTS)) ||
            (logId[e] >= XM_HM_MAX_EVENTS))
        {
            return XM_INVALID_PARAM;
        }
    }

    for (e=0; e<noLogs; e++) {    
        memset(&log, 0, sizeof(xmHmLog_t));
        log.opCode&=~(HMLOG_OPCODE_SYS_MASK|HMLOG_OPCODE_PARTID_MASK|HMLOG_OPCODE_EVENT_MASK);
        log.opCode|=(cfg->rId<<HMLOG_OPCODE_PARTID_BIT)|(logId[e]<<HMLOG_OPCODE_EVENT_BIT);
        HmRaiseEvent(&log);
    }
    return noLogs*sizeof(xmHmLog_t);
}

static xm_s32_t ReadHmLog(xmObjDesc_t desc, xmHmLog_t *__gParam log, xm_u32_t size) {
    xm_s32_t e, noLogs=size/sizeof(xmHmLog_t);
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    if (OBJDESC_GET_PARTITIONID(desc)!=XM_HYPERVISOR_ID)
	return XM_INVALID_PARAM;

    if (!(GetPartitionCfg(info->sched.cKThread)->flags&XM_PART_SYSTEM))
        return XM_PERM_ERROR;

    if (CheckGParam(log, size, 4, PFLAG_NOT_NULL|PFLAG_RW)<0) return XM_INVALID_PARAM;
    
    if (!log||!noLogs)
	return XM_INVALID_PARAM;
    
    for (e=0; e<noLogs; e++)
	if (QueueExtractElement(&sysHmLog, &log[e])<0) {
	    return e*sizeof(xmHmLog_t);
        }
    
    return noLogs*sizeof(xmHmLog_t);
}

static xm_s32_t CtrlHmLog(xmObjDesc_t desc, xm_u32_t cmd, union hmCmd *__gParam args) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    if (OBJDESC_GET_PARTITIONID(desc)!=XM_HYPERVISOR_ID)
	return XM_INVALID_PARAM;
    
    if (!(GetPartitionCfg(info->sched.cKThread)->flags&XM_PART_SYSTEM))
        return XM_PERM_ERROR;
    if (CheckGParam(args, sizeof(union hmCmd), 4, PFLAG_NOT_NULL|PFLAG_RW)<0) return XM_INVALID_PARAM;
    switch(cmd) {
    case XM_HM_GET_STATUS:
	args->status.noEvents=QueueGetNoElem(&sysHmLog);
        return XM_OK;
    }
    return XM_INVALID_PARAM;
}

static const struct object hmObj={
    .Read=(readObjOp_t)ReadHmLog,
    .Write=(writeObjOp_t)WriteHmLog,
    .Ctrl=(ctrlObjOp_t)CtrlHmLog,
};

xm_s32_t SetupHm(void) {
    void *logBuffer;
    objectTab[OBJ_CLASS_HM]=&hmObj;
    GET_MEMZ(logBuffer, sizeof(struct xmHmLog)*CONFIG_OBJ_HM_LOG_NO_ELEM);
    QueueInit(&sysHmLog, CONFIG_OBJ_HM_LOG_NO_ELEM, sizeof(struct xmHmLog), logBuffer);
    hmInit=1;
    return 0;
}

REGISTER_OBJ(SetupHm);

xm_s32_t HmRaiseEvent(xmHmLog_t *log) {
    xm_s32_t propagate=0, oldPlanId;
    xm_u32_t eventId, system;
    cpuCtxt_t ctxt;
    xmId_t partitionId, pId;
    xmTime_t cTime;
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    cpuCtxt_t *TCpuCtxt;
#ifdef CONFIG_OBJ_HM_VERBOSE
    xm_s32_t e;
#endif

    if (!hmInit) return 0;
    cTime=GetSysClockUsec();
    eventId=(log->opCode&HMLOG_OPCODE_EVENT_MASK)>>HMLOG_OPCODE_EVENT_BIT;
    ASSERT((eventId>=0)&&(eventId<XM_HM_MAX_EVENTS));   
    partitionId=(log->opCode&HMLOG_OPCODE_PARTID_MASK)>>HMLOG_OPCODE_PARTID_BIT;
    pId = PartitionId2PId(partitionId);
    system=(log->opCode&HMLOG_OPCODE_SYS_MASK)?1:0;
#ifdef CONFIG_OBJ_STATUS_ACC
    systemStatus.noHmEvents++;
#endif
    log->timestamp=cTime;
#ifdef CONFIG_OBJ_HM_VERBOSE
    kprintf("[HM:%lld] event %d (%d): sys %d: Id %d\n", log->timestamp, eventId, (log->opCode&HMLOG_OPCODE_MODID_MASK)>>HMLOG_OPCODE_MODID_BIT, system, partitionId);
    if (!(log->opCode&HMLOG_OPCODE_VALID_CPUCTXT_MASK)) {
        for (e=0; e<XM_HMLOG_PAYLOAD_LENGTH; e++)
            kprintf("0x%lx ", log->payload[e]);
        kprintf("\n");
    } else {
        PrintHmCpuCtxt(&log->cpuCtxt);
    }
#endif
    if (system) {
	if (xmcTab.hpv.hmTab[eventId].log) {
            QueueInsertElement(&sysHmLog, log);
        }
	switch(xmcTab.hpv.hmTab[eventId].action) {
	case XM_HM_AC_IGNORE:
	    // Doing nothing
	    break;
	case XM_HM_AC_HYPERVISOR_COLD_RESET:
	    resetStatusInit[0] = XM_COLD_RESET;
	    ResetSystem(XM_COLD_RESET, log);
	    break;
	case XM_HM_AC_HYPERVISOR_WARM_RESET:
	    resetStatusInit[0] = XM_COLD_RESET; /* The partitions are cold reset */
	    ResetSystem(XM_WARM_RESET, log);    /* The system is warm reset */
	    break;
	case XM_HM_AC_SWITCH_TO_MAINTENANCE:
            SwitchSchedPlan(1, &oldPlanId);
	    MakePlanSwitch(cTime, GET_LOCAL_PROCESSOR()->sched.data);
            Schedule();
	    break;
	case XM_HM_AC_HALT:
	    HaltSystem();
	    break;
	default:
	    GetCpuCtxt(&ctxt);
	    SystemPanic(&ctxt,"Unknown health-monitor action %d\n", xmcTab.hpv.hmTab[eventId].action);
	}
    } else {
        
	if (partitionTab[pId].cfg->hmTab[eventId].log) {
            QueueInsertElement(&sysHmLog, log);
        }
	ASSERT(pId<xmcTab.noPartitions);
	switch(partitionTab[pId].cfg->hmTab[eventId].action) {
	case XM_HM_AC_IGNORE:
            if (info->sched.cKThread->ctrl.g){
		TCpuCtxt=info->sched.cKThread->ctrl.irqCpuCtxt;
		TCpuCtxt->pc+=0x4;
		TCpuCtxt->nPc+=0x4;
            }
	    break;
	case XM_HM_AC_PARTITION_COLD_RESET:
#ifdef CONFIG_OBJ_HM_VERBOSE
	    kprintf("[HM] Partition %d cold reseted\n", partitionId);
#endif 
	    ResetPartition(&partitionTab[pId], xmcPartitionTab[pId].entryPoint,
	        1, XM_COLD_RESET);
	    Schedule();
	    break;
	case XM_HM_AC_PARTITION_WARM_RESET:
#ifdef CONFIG_OBJ_HM_VERBOSE
	    kprintf("[HM] Partition %d warm reseted\n", partitionId);
#endif
	    ResetPartition(&partitionTab[pId], xmcPartitionTab[pId].entryPoint,
	        0, XM_WARM_RESET);
	    break;
	case XM_HM_AC_HYPERVISOR_COLD_RESET:
	    resetStatusInit[0] = XM_COLD_RESET;
	    ResetSystem(XM_COLD_RESET, log);
	    break;
	case XM_HM_AC_HYPERVISOR_WARM_RESET:
	    resetStatusInit[0] = XM_COLD_RESET;  /* The partitions are cold reseted */
	    ResetSystem(XM_WARM_RESET, log);     /* The system is warm reset */
	    break;
	case XM_HM_AC_SUSPEND:	    
#ifdef CONFIG_OBJ_HM_VERBOSE
	    kprintf("[HM] Partition %d suspended\n", partitionId);
#endif
            SUSPEND_PARTITION(pId);
	    Schedule();
	    break;
	case XM_HM_AC_HALT:
#ifdef CONFIG_OBJ_HM_VERBOSE
	    kprintf("[HM] Partition %d halted\n", partitionId);
#endif
            HALT_PARTITION(pId);
	    Schedule();
	    break;
        case XM_HM_AC_SWITCH_TO_MAINTENANCE:
            SwitchSchedPlan(1, &oldPlanId);
	    MakePlanSwitch(cTime, GET_LOCAL_PROCESSOR()->sched.data);
            Schedule();
	    break;
	case XM_HM_AC_PROPAGATE:
	    propagate=1;
	    break;
	default:
            GetCpuCtxt(&ctxt);
	    SystemPanic(&ctxt,"Unknown health-monitor action %d\n", 
                        partitionTab[pId].cfg->hmTab[eventId].action);
	}
    }

    return propagate;
}

