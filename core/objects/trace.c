/*
 * $FILE: trace.c
 *
 * Tracing mechanism
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
#include <checksum.h>
#include <hypercalls.h>
#include <kthread.h>
#include <kdevice.h>
#include <queue.h>
#include <stdc.h>
#include <xmconf.h>
#include <sched.h>
#include <objects/trace.h>
#include <objects/hm.h>

static struct queue sysTraceLog, *partTraceLog;

static xm_s32_t ReadTrace(xmObjDesc_t desc, xmTraceEvent_t *__gParam event, xm_u32_t size) {
    xm_s32_t e, noTraces=size/sizeof(xmTraceEvent_t);
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xmId_t partId, pId;
    
    partId=OBJDESC_GET_PARTITIONID(desc);
    pId=PartitionId2PId(partId);
    if (partId!=GetPartitionCfg(info->sched.cKThread)->rId)
        if (!(GetPartitionCfg(info->sched.cKThread)->flags&XM_PART_SYSTEM))
	    return XM_PERM_ERROR;

    if (!noTraces)
	return XM_INVALID_PARAM;

    if (CheckGParam(event, size, 4, PFLAG_NOT_NULL|PFLAG_RW)<0) return XM_INVALID_PARAM;
    if (partId==XM_HYPERVISOR_ID) {
        for (e=0; e<noTraces; e++) {
            if (QueueExtractElement(&sysTraceLog, &event[e])<0)
                return e*sizeof(xmTraceEvent_t);
        }
    } else {
	if ((pId<0)||(pId>=xmcTab.noPartitions))
	    return XM_INVALID_PARAM;
        for (e=0; e<noTraces; e++) {
            if (QueueExtractElement(&partTraceLog[pId], &event[e])<0)
                return e*sizeof(xmTraceEvent_t);
        }
    }
    return noTraces*sizeof(xmTraceEvent_t);
}

static xm_s32_t WriteTrace(xmObjDesc_t desc, xm_u8_t *__gParam trace, xm_u32_t size) {
    xm_s32_t e, noTraces=size/sizeof(xmTraceEvent_t), written;
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xmId_t partId, pId;
    xmTraceEvent_t event[noTraces];
    partId=OBJDESC_GET_PARTITIONID(desc);
    pId=PartitionId2PId(partId);
    if (partId!=GetPartitionCfg(info->sched.cKThread)->rId)
	return XM_PERM_ERROR;
    
    if (!noTraces)
	return XM_INVALID_PARAM;

    if (CheckGParam(trace, XM_TRACE_PAYLOAD_LENGTH, 4, PFLAG_NOT_NULL)<0) return XM_INVALID_PARAM;

    for (written=0, e=0; e<noTraces; e++) {
        event[e].timestamp=(xm_u32_t)GetSysClockUsec();
        memcpy(event[e].payload, trace, XM_TRACE_PAYLOAD_LENGTH);
        QueueInsertElement(&partTraceLog[pId], &event[e]);
        written++;
    }
    return written*sizeof(xmTraceEvent_t);
}

static xm_s32_t CtrlTrace(xmObjDesc_t desc, xm_u32_t cmd, union traceCmd *__gParam args) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    struct queue *log;
    xmId_t partId, pId;

    partId=OBJDESC_GET_PARTITIONID(desc);
    pId=PartitionId2PId(partId);
    if (partId!=GetPartitionCfg(info->sched.cKThread)->rId)
        if (!(GetPartitionCfg(info->sched.cKThread)->flags&XM_PART_SYSTEM))
	    return XM_PERM_ERROR;
    if (CheckGParam(args, sizeof(union traceCmd), 4, PFLAG_NOT_NULL|PFLAG_RW)<0) 
	return XM_INVALID_PARAM;
    if (partId==XM_HYPERVISOR_ID)
	log=&sysTraceLog;
    else {
	if ((pId<0)||(pId>=xmcTab.noPartitions))
	    return XM_INVALID_PARAM;
	log=&partTraceLog[pId];
    }
    
    switch(cmd) {
    case XM_TRACE_GET_STATUS:        
	args->status.noEvents=QueueGetNoElem(log);
	return XM_OK;
    }
    return XM_INVALID_PARAM;
}

static const struct object traceObj={
    .Read=(readObjOp_t)ReadTrace,
    .Write=(writeObjOp_t)WriteTrace,
    .Ctrl=(ctrlObjOp_t)CtrlTrace,
};

xm_s32_t SetupTrace(void) {
    void *logBuffer;
    xm_s32_t e;
    objectTab[OBJ_CLASS_TRACE]=&traceObj;
    GET_MEMZ(logBuffer, sizeof(struct xmTraceEvent)*CONFIG_OBJ_TRACE_LOG_NO_ELEM);
    QueueInit(&sysTraceLog, CONFIG_OBJ_TRACE_LOG_NO_ELEM, sizeof(struct xmTraceEvent), logBuffer);
    GET_MEMZ(partTraceLog, xmcTab.noPartitions*sizeof(struct queue));
    for (e=0; e<xmcTab.noPartitions; e++) {
        GET_MEMZ(logBuffer, sizeof(struct xmTraceEvent)*CONFIG_OBJ_TRACE_LOG_NO_ELEM);
        QueueInit(&partTraceLog[e], CONFIG_OBJ_TRACE_LOG_NO_ELEM, sizeof(struct xmTraceEvent), logBuffer);
    }
    return 0;
}

REGISTER_OBJ(SetupTrace);

#ifdef CONFIG_AUDIT_EVENTS
void RaiseAuditEvent(xm_u8_t partId, xm_u8_t event) {
    xmTraceEvent_t trace;
    trace.timestamp=(xm_u32_t)GetSysClockUsec();
    trace.payload[0]=partId;
    trace.payload[1]=event;
    QueueInsertElement(&sysTraceLog, &trace);
}
#endif
