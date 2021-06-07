/*
 * $FILE: commports_ex.c
 *
 * Inter-partition communication mechanisms
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
  - [02/09/2015:SPR-020915-04] Remove VBOOT macro and boot.h
*/
#include <assert.h>
#include <list2.h>
#include <gaccess.h>
#include <kthread.h>
#include <hypercalls.h>
#include <rsvmem.h>
#include <sched.h>
#include <spinlock.h>
#include <stdc.h>
#include <xmconf.h>
#include <objects/commports.h>
#include <vmmap.h>
#include <kthread.h>

static union channel *channelTab;
static struct port *portTab;

static inline xm_s32_t CreateSamplingPort(xmObjDesc_t desc, xm_s8_t *__gParam portName, xm_u32_t maxMsgSize, xm_u32_t direction, xmTime_t refreshPeriod) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    struct xmcPartition *partition;
    xm_s32_t port;
    partition=GetPartitionCfg(info->sched.cKThread);
    if (CheckGParam(portName, CONFIG_ID_STRING_LENGTH, 1, PFLAG_NOT_NULL)<0) 
        return XM_INVALID_PARAM;
    if ((direction!=XM_SOURCE_PORT)&&(direction!=XM_DESTINATION_PORT))
        return XM_INVALID_PARAM;
    // Look for the channel
    for (port=partition->commPortsOffset; port<(partition->noPorts+partition->commPortsOffset); port++)
	if (!strncmp(portName, &xmcStringTab[xmcCommPorts[port].nameOffset], CONFIG_ID_STRING_LENGTH)) break;

    if (port>=partition->noPorts+partition->commPortsOffset)
        return XM_INVALID_CONFIG;

    if (port>=xmcTab.noCommPorts)
	return XM_INVALID_PARAM;

    if (xmcCommPorts[port].type!=XM_SAMPLING_PORT) 
	return XM_INVALID_CONFIG;
    if (direction!=xmcCommPorts[port].direction)
	return XM_INVALID_CONFIG;
    if (portTab[port].flags&COMM_PORT_OPENED)
        return XM_NO_ACTION;
    if (xmcCommPorts[port].channelId!=XM_NULL_CHANNEL) {
	ASSERT((xmcCommPorts[port].channelId>=0)&&(xmcCommPorts[port].channelId<xmcTab.noCommChannels));
	if (xmcCommChannelTab[xmcCommPorts[port].channelId].s.maxLength!=maxMsgSize)
	    return XM_INVALID_CONFIG;
        if (xmcCommChannelTab[xmcCommPorts[port].channelId].s.refreshPeriod!=refreshPeriod)
            return XM_INVALID_CONFIG;
        if (direction==XM_DESTINATION_PORT) {
            ASSERT(channelTab[xmcCommPorts[port].channelId].s.noReceivers<xmcCommChannelTab[xmcCommPorts[port].channelId].s.noReceivers);
            channelTab[xmcCommPorts[port].channelId].s.receiverTab[channelTab[xmcCommPorts[port].channelId].s.noReceivers]=info->sched.cKThread;
            channelTab[xmcCommPorts[port].channelId].s.receiverPortTab[channelTab[xmcCommPorts[port].channelId].s.noReceivers]=port;
            channelTab[xmcCommPorts[port].channelId].s.noReceivers++;        
        } else { // XM_SOURCE_PORT
            channelTab[xmcCommPorts[port].channelId].s.ctrl->spinLock=SPINLOCK_INIT;
            channelTab[xmcCommPorts[port].channelId].s.sender=info->sched.cKThread;
            channelTab[xmcCommPorts[port].channelId].s.senderPort=port;
        }
    }
    portTab[port].flags|=COMM_PORT_OPENED|COMM_PORT_EMPTY;
    portTab[port].partitionId=GetPartitionCfg(info->sched.cKThread)->id;    
    return port;
}

static xm_s32_t ReadSamplingPort(xmObjDesc_t desc,  void *__gParam msgPtr, xmSize_t msgSize, xm_u32_t *__gParam flags) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xm_s32_t port=OBJDESC_GET_ID(desc);
    struct xmcCommChannel *xmcChannel;
    union channel *channel;
    xmSize_t retSize=0;
    struct guest *g;
    xmWord_t *commPortBitmap;

    if (CheckGParam(flags, sizeof(xm_u32_t), 4, PFLAG_RW)<0)
        return XM_INVALID_PARAM;

    if (port>=xmcTab.noCommPorts)
	return XM_INVALID_PARAM;

    // Reading a port which does not belong to this partition
    if (portTab[port].partitionId!=GetPartitionCfg(info->sched.cKThread)->id)
	return XM_INVALID_PARAM;

    if (!(portTab[port].flags&COMM_PORT_OPENED))
	return XM_INVALID_PARAM;

    if (xmcCommPorts[port].type!=XM_SAMPLING_PORT) 
	return XM_INVALID_PARAM;

    if (xmcCommPorts[port].direction!=XM_DESTINATION_PORT)
	return XM_INVALID_PARAM;
    
    if (!msgSize)
	return XM_INVALID_CONFIG;

    if (xmcCommPorts[port].channelId!=XM_NULL_CHANNEL) {
	xmcChannel=&xmcCommChannelTab[xmcCommPorts[port].channelId];
	if (msgSize>xmcChannel->s.maxLength)
	    return XM_INVALID_CONFIG;

        if (CheckGParam(msgPtr, msgSize, 1, PFLAG_NOT_NULL|PFLAG_RW)<0)
            return XM_INVALID_PARAM;
	channel=&channelTab[xmcCommPorts[port].channelId];
        if (SpinIsLocked(&channel->s.ctrl->spinLock))
            return XM_NOT_AVAILABLE;
        SpinLock(&channel->s.ctrl->spinLock);
	retSize=(msgSize<channel->s.ctrl->length)?msgSize:channel->s.ctrl->length;
	memcpy(msgPtr, channel->s.ctrl+1, retSize);
        SpinUnlock(&channel->s.ctrl->spinLock);
        portTab[port].flags&=~COMM_PORT_MSG_MASK;
        portTab[port].flags|=COMM_PORT_CONSUMED_MSG;
#ifdef CONFIG_OBJ_STATUS_ACC
        systemStatus.noSamplingPortMsgsRead++;
        if (info->sched.cKThread->ctrl.g)
            partitionStatus[GetPartitionCfg(info->sched.cKThread)->id].noSamplingPortMsgsRead++;
#endif
        g=info->sched.cKThread->ctrl.g;
        commPortBitmap=(xmWord_t *)((xmAddress_t)g->partCtrlTab+sizeof(partitionControlTable_t)+sizeof(struct xmPhysicalMemMap)*CONFIG_MAX_NO_MAREAS);
        ASSERT((port-GetPartitionCfg(info->sched.cKThread)->commPortsOffset)<g->partCtrlTab->noCommPorts);
        xmClearBit(commPortBitmap, port, xmcTab.noCommPorts);

        if (channel->s.sender) {
            g=channel->s.sender->ctrl.g;
            commPortBitmap=(xmWord_t *)((xmAddress_t)g->partCtrlTab+sizeof(partitionControlTable_t)+sizeof(struct xmPhysicalMemMap)*CONFIG_MAX_NO_MAREAS);
            xmClearBit(commPortBitmap, channel->s.senderPort, xmcTab.noCommPorts);
            SetExtIrqPending(channel->s.sender, XM_VT_EXT_SAMPLING_PORT);
            portTab[channel->s.senderPort].flags&=~COMM_PORT_MSG_MASK;
            portTab[channel->s.senderPort].flags|=COMM_PORT_CONSUMED_MSG;
        }

	if (flags) {
	    *flags=0;
            SpinLock(&channel->s.ctrl->spinLock);
	    if (retSize&&(xmcChannel->s.refreshPeriod!=XM_INFINITE_TIME)&&(channel->s.ctrl->timestamp+xmcChannel->s.refreshPeriod)>GetSysClockUsec())
                *flags=XM_COMM_MSG_VALID;
            SpinUnlock(&channel->s.ctrl->spinLock);
	}
        if (retSize == 0)
            return XM_NO_ACTION;
    }
    else {
        return XM_INVALID_CONFIG;
    }
    return retSize;
}

static xm_s32_t WriteSamplingPort(xmObjDesc_t desc, void *__gParam msgPtr, xmSize_t msgSize) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xm_s32_t port=OBJDESC_GET_ID(desc);
    struct xmcCommChannel *xmcChannel;
    union channel *channel;
    xm_s32_t e;
    struct guest *g;
    xmWord_t *commPortBitmap;

    if (port>=xmcTab.noCommPorts)
	return XM_INVALID_PARAM;

    // Reading a port which does not belong to this partition    
    if (portTab[port].partitionId!=GetPartitionCfg(info->sched.cKThread)->id)
	return XM_INVALID_PARAM;

    if (!(portTab[port].flags&COMM_PORT_OPENED))
	return XM_INVALID_PARAM;

    if (xmcCommPorts[port].type!=XM_SAMPLING_PORT) 
	return XM_INVALID_PARAM;

    if (xmcCommPorts[port].direction!=XM_SOURCE_PORT)
	return XM_INVALID_PARAM;

    if (!msgSize)
	return XM_INVALID_CONFIG;

    if (xmcCommPorts[port].channelId!=XM_NULL_CHANNEL) {
	xmcChannel=&xmcCommChannelTab[xmcCommPorts[port].channelId];
	if (msgSize>xmcChannel->s.maxLength)
	    return XM_INVALID_CONFIG;

        if (CheckGParam(msgPtr, msgSize, 1, PFLAG_NOT_NULL)<0) 
            return XM_INVALID_PARAM;
	channel=&channelTab[xmcCommPorts[port].channelId];
        if (SpinIsLocked(&channel->s.ctrl->spinLock))
            return XM_NOT_AVAILABLE;
        SpinLock(&channel->s.ctrl->spinLock);
	memcpy(channel->s.ctrl+1, msgPtr, msgSize);
        channel->s.ctrl->length=msgSize;
        channel->s.ctrl->timestamp=GetSysClockUsec();
        channel->s.ctrl->flags|=CHANNEL_NEW_MSG;
        SpinUnlock(&channel->s.ctrl->spinLock);
        portTab[port].flags&=~COMM_PORT_MSG_MASK;
        portTab[port].flags|=COMM_PORT_NEW_MSG;
#ifdef CONFIG_OBJ_STATUS_ACC
        systemStatus.noSamplingPortMsgsWritten++;
        if (info->sched.cKThread->ctrl.g)
            partitionStatus[GetPartitionCfg(info->sched.cKThread)->id].noSamplingPortMsgsWritten++;
#endif
        ASSERT(channel->s.sender==info->sched.cKThread);
        g=info->sched.cKThread->ctrl.g;
        commPortBitmap=(xmWord_t *)((xmAddress_t)g->partCtrlTab+sizeof(partitionControlTable_t)+sizeof(struct xmPhysicalMemMap)*CONFIG_MAX_NO_MAREAS);
        xmSetBit(commPortBitmap, port, xmcTab.noCommPorts);
        for (e=0; e<channel->s.noReceivers; e++) {
            if (xmcCommPorts[channel->s.receiverPortTab[e]].flags&XM_EXTERNAL_PORT) {                
               // SendIpi(xmcCommPorts[channel->s.receiverPortTab[e]].cpuId, COMM_PORT_IPI);
            } else {
                g=channel->s.receiverTab[e]->ctrl.g;
                commPortBitmap=(xmWord_t *)((xmAddress_t)g->partCtrlTab+sizeof(partitionControlTable_t)+sizeof(struct xmPhysicalMemMap)*CONFIG_MAX_NO_MAREAS);
                if (channel->s.receiverTab[e]) {
                    ASSERT(channel->s.receiverPortTab[e]-GetPartitionCfg(channel->s.receiverTab[e])->commPortsOffset < g->partCtrlTab->noCommPorts);
                    xmSetBit(commPortBitmap, channel->s.receiverPortTab[e], xmcTab.noCommPorts);
                    SetExtIrqPending(channel->s.receiverTab[e], XM_VT_EXT_SAMPLING_PORT);
                
                    portTab[channel->s.receiverPortTab[e]].flags&=~COMM_PORT_MSG_MASK;
                    portTab[channel->s.receiverPortTab[e]].flags|=COMM_PORT_NEW_MSG;
                }
            }
        }
    }
    else {
        return XM_INVALID_CONFIG;
    }

    return XM_OK;
}

static inline xm_s32_t GetSPortInfo(xmObjDesc_t desc, xmSamplingPortInfo_t *portInfo) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    struct xmcPartition *partition;
    struct xmcCommChannel *xmcChannel;
    xm_s32_t port;
    
    partition=GetPartitionCfg(info->sched.cKThread);
    
    if (CheckGParam(portInfo->portName, CONFIG_ID_STRING_LENGTH, 1, PFLAG_NOT_NULL)<0) 
        return XM_INVALID_PARAM;

    // Look for the channel
    for (port=partition->commPortsOffset; port<(partition->noPorts+partition->commPortsOffset); port++)
	if (!strcmp(portInfo->portName, &xmcStringTab[xmcCommPorts[port].nameOffset])) break;

    if (port>=xmcTab.noCommPorts)
	return XM_INVALID_PARAM;

    if (xmcCommPorts[port].type!=XM_SAMPLING_PORT) 
	return XM_INVALID_PARAM;
    
    if (xmcCommPorts[port].channelId!=XM_NULL_CHANNEL) {
	xmcChannel=&xmcCommChannelTab[xmcCommPorts[port].channelId];
	portInfo->refreshPeriod=xmcChannel->s.refreshPeriod;
	portInfo->maxMsgSize=xmcChannel->s.maxLength;
	portInfo->direction=xmcCommPorts[port].direction;
    } else {
	portInfo->refreshPeriod=XM_INFINITE_TIME;
	portInfo->maxMsgSize=0;
	portInfo->direction=0;
    }

    return XM_OK;
}

static inline xm_s32_t GetSPortStatus(xmObjDesc_t desc, xmSamplingPortStatus_t *status) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xm_s32_t port=OBJDESC_GET_ID(desc);
    struct xmcCommChannel *xmcChannel;
    union channel *channel;

    if (port>=xmcTab.noCommPorts)
	return XM_INVALID_PARAM;

    // Reading a port which does not belong to this partition    
    if (portTab[port].partitionId!=GetPartitionCfg(info->sched.cKThread)->id)
	return XM_INVALID_PARAM;
    
    if (!(portTab[port].flags&COMM_PORT_OPENED))
	return XM_INVALID_PARAM;
    
    if (xmcCommPorts[port].type!=XM_SAMPLING_PORT) 
	return XM_INVALID_PARAM;

    if (xmcCommPorts[port].channelId!=XM_NULL_CHANNEL) {
	xmcChannel=&xmcCommChannelTab[xmcCommPorts[port].channelId];
	status->flags=0;
	channel=&channelTab[xmcCommPorts[port].channelId];
        if (SpinIsLocked(&channel->s.ctrl->spinLock))
            return XM_NOT_AVAILABLE;
        SpinLock(&channel->s.ctrl->spinLock);
        status->lastMsgSize=channel->s.ctrl->length;
        status->timestamp=channel->s.ctrl->timestamp;
        SpinUnlock(&channel->s.ctrl->spinLock);
        if (portTab[port].flags&COMM_PORT_NEW_MSG){
            status->flags|=XM_COMM_NEW_MSG;
        } else if (portTab[port].flags&COMM_PORT_CONSUMED_MSG) {
            status->flags|=XM_COMM_CONSUMED_MSG;
        } else {
            status->flags|=XM_COMM_EMPTY_PORT;
        }
        SpinLock(&channel->s.ctrl->spinLock);
	if (channel->s.ctrl->timestamp&&(xmcChannel->s.refreshPeriod!=XM_INFINITE_TIME)&&((channel->s.ctrl->timestamp+xmcChannel->s.refreshPeriod)>GetSysClockUsec()))
            status->flags|=XM_COMM_MSG_VALID;
        SpinUnlock(&channel->s.ctrl->spinLock);
    } else
	memset(status, 0, sizeof(xmSamplingPortStatus_t));
    return XM_OK;
}

static xm_s32_t CtrlSamplingPort(xmObjDesc_t desc, xm_u32_t cmd, union samplingPortCmd *__gParam args) {
    if (CheckGParam(args, sizeof(union samplingPortCmd), 4, PFLAG_NOT_NULL|PFLAG_RW)<0) 
        return XM_INVALID_PARAM;
    switch(cmd) {
    case XM_COMM_CREATE_PORT:
	if (!args->create.portName||(CheckGParam(args->create.portName, CONFIG_ID_STRING_LENGTH,1, PFLAG_NOT_NULL)<0)) return XM_INVALID_PARAM;
	return CreateSamplingPort(desc, args->create.portName, args->create.maxMsgSize, args->create.direction, args->create.refreshPeriod);
    case XM_COMM_GET_PORT_STATUS:
	return GetSPortStatus(desc, &args->status);
    case XM_COMM_GET_PORT_INFO:
	return GetSPortInfo(desc, &args->info);
    }
    return XM_INVALID_PARAM;
}

static const struct object samplingPortObj={
    .Read=(readObjOp_t)ReadSamplingPort,
    .Write=(writeObjOp_t)WriteSamplingPort,
    .Ctrl=(ctrlObjOp_t)CtrlSamplingPort,
};

static inline xm_s32_t CreateQueuingPort(xmObjDesc_t desc, xm_s8_t *__gParam portName, xm_s32_t maxNoMsgs, xm_s32_t maxMsgSize, xm_u32_t direction) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    struct xmcPartition *partition;
    xm_s32_t port;
    partition=GetPartitionCfg(info->sched.cKThread);
    if (CheckGParam(portName, CONFIG_ID_STRING_LENGTH, 1, PFLAG_NOT_NULL)<0) 
        return XM_INVALID_PARAM;
    if ((direction!=XM_SOURCE_PORT)&&(direction!=XM_DESTINATION_PORT))
        return XM_INVALID_PARAM;
    // Look for the channel
    for (port=partition->commPortsOffset; port<(partition->noPorts+partition->commPortsOffset); port++)
	if (!strncmp(portName, &xmcStringTab[xmcCommPorts[port].nameOffset], CONFIG_ID_STRING_LENGTH)) break;

    if (port>=partition->noPorts+partition->commPortsOffset)
        return XM_INVALID_CONFIG;

    if (port>=xmcTab.noCommPorts)
	return XM_INVALID_PARAM;
    if (xmcCommPorts[port].type!=XM_QUEUING_PORT) 
	return XM_INVALID_CONFIG;
    if (direction!=xmcCommPorts[port].direction)
	return XM_INVALID_CONFIG;
    if (portTab[port].flags&COMM_PORT_OPENED)
        return XM_NO_ACTION;

    if (xmcCommPorts[port].channelId!=XM_NULL_CHANNEL) {
	ASSERT((xmcCommPorts[port].channelId>=0)&&(xmcCommPorts[port].channelId<xmcTab.noCommChannels));

	if (direction==XM_DESTINATION_PORT) {
	    channelTab[xmcCommPorts[port].channelId].q.receiver=info->sched.cKThread;
            channelTab[xmcCommPorts[port].channelId].q.receiverPort=port;
	} else { // XM_SOURCE_PORT
            channelTab[xmcCommPorts[port].channelId].q.ctrl->spinLock=SPINLOCK_INIT;
            channelTab[xmcCommPorts[port].channelId].q.sender=info->sched.cKThread;
            channelTab[xmcCommPorts[port].channelId].q.senderPort=port;
        }  

	if (xmcCommChannelTab[xmcCommPorts[port].channelId].q.maxNoMsgs!=maxNoMsgs)
	    return XM_INVALID_CONFIG;
	if (xmcCommChannelTab[xmcCommPorts[port].channelId].q.maxLength!=maxMsgSize)
	    return XM_INVALID_CONFIG;	
    }
    portTab[port].flags|=COMM_PORT_OPENED;
    portTab[port].partitionId=GetPartitionCfg(info->sched.cKThread)->id;
    return port;
}

static xm_s32_t SendQueuingPort(xmObjDesc_t desc, void *__gParam msgPtr, xm_u32_t msgSize) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xm_s32_t port=OBJDESC_GET_ID(desc);
    struct xmcCommChannel *xmcChannel;
    union channel *channel;
    struct msg *msg = NULL;
    struct guest *g;
    xmWord_t *commPortBitmap;
    if (port>=xmcTab.noCommPorts)
	return XM_INVALID_PARAM;
    // Reading a port which does not belong to this partition    
    if (portTab[port].partitionId!=GetPartitionCfg(info->sched.cKThread)->id)
	return XM_INVALID_PARAM;
    if (!(portTab[port].flags&COMM_PORT_OPENED))
	return XM_INVALID_PARAM;
    if (xmcCommPorts[port].type!=XM_QUEUING_PORT) 
	return XM_INVALID_PARAM;
    if (xmcCommPorts[port].direction!=XM_SOURCE_PORT)
	return XM_INVALID_PARAM;
    if (!msgSize)
	return XM_INVALID_CONFIG;

    if (xmcCommPorts[port].channelId!=XM_NULL_CHANNEL) {        
	xmcChannel=&xmcCommChannelTab[xmcCommPorts[port].channelId];
	if (msgSize>xmcChannel->q.maxLength)
	    return XM_INVALID_CONFIG;
        if (CheckGParam(msgPtr, msgSize, 1, PFLAG_NOT_NULL)<0) 
            return XM_INVALID_PARAM;
	channel=&channelTab[xmcCommPorts[port].channelId];
        SpinLock(&channel->q.ctrl->spinLock);
	if (channel->q.ctrl->usedMsgs<xmcChannel->q.maxNoMsgs) {
    	    if (!(msg=(struct msg *)DynListRemoveTailR((xmAddress_t)channel->q.ctrl, &channel->q.ctrl->freeMsgs))) {
                cpuCtxt_t ctxt;
                SpinUnlock(&channel->q.ctrl->spinLock);
                GetCpuCtxt(&ctxt);
    		SystemPanic(&ctxt, "[SendQueuingPort] Queuing channels internal error");
            }
            SpinUnlock(&channel->q.ctrl->spinLock);
    	    memcpy((xm_u8_t *)(msg->buffer+(xmAddress_t)channel->q.ctrl), msgPtr, msgSize);
#ifdef CONFIG_OBJ_STATUS_ACC
            systemStatus.noQueuingPortMsgsSent++;
            if (info->sched.cKThread->ctrl.g)
                partitionStatus[GetPartitionCfg(info->sched.cKThread)->id].noQueuingPortMsgsSent++;
#endif
	    msg->length=msgSize;
            SpinLock(&channel->q.ctrl->spinLock);
    	    DynListInsertHeadR((xmAddress_t)channel->q.ctrl, &channel->q.ctrl->recvMsgs, &msg->listNode);
    	    channel->q.ctrl->usedMsgs++;
            channel->q.ctrl->flags|=CHANNEL_NEW_MSG;
            SpinUnlock(&channel->q.ctrl->spinLock);
	    if (channel->q.receiver) {
                if (xmcCommPorts[channel->q.receiverPort].flags&XM_EXTERNAL_PORT) {
                    //SendIpi(xmcCommPorts[channel->q.receiverPort].cpuId, COMM_PORT_IPI);
                } else {
                    g=channel->q.receiver->ctrl.g;
                    commPortBitmap=(xmWord_t *)((xmAddress_t)g->partCtrlTab+sizeof(partitionControlTable_t)+sizeof(struct xmPhysicalMemMap)*CONFIG_MAX_NO_MAREAS);
                    ASSERT((channel->q.receiverPort - GetPartitionCfg(channel->q.receiver)->commPortsOffset) < g->partCtrlTab->noCommPorts);
                    xmSetBit(commPortBitmap, channel->q.receiverPort, xmcTab.noCommPorts);
                    SetExtIrqPending(channel->q.receiver, XM_VT_EXT_QUEUING_PORT);
                }
	    }
            g=info->sched.cKThread->ctrl.g;
            commPortBitmap=(xmWord_t *)((xmAddress_t)g->partCtrlTab+sizeof(partitionControlTable_t)+sizeof(struct xmPhysicalMemMap)*CONFIG_MAX_NO_MAREAS);
            ASSERT(channel->q.sender==info->sched.cKThread);
            xmSetBit(commPortBitmap, channel->q.senderPort, xmcTab.noCommPorts);
    	} else {
            SpinUnlock(&channel->q.ctrl->spinLock);
    	    return XM_NOT_AVAILABLE;
        }
        SpinUnlock(&channel->q.ctrl->spinLock);
    } else
	return XM_INVALID_CONFIG;

    return XM_OK;
}

static xm_s32_t ReceiveQueuingPort(xmObjDesc_t desc, void *__gParam msgPtr, xm_u32_t msgSize) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xm_s32_t port=OBJDESC_GET_ID(desc);
    struct xmcCommChannel *xmcChannel;
    union channel *channel;
    xmSize_t retSize=0;
    struct msg *msg = NULL;

    if (port>=xmcTab.noCommPorts)
	return XM_INVALID_PARAM;

    // Reading a port which does not belong to this partition
    if (portTab[port].partitionId!=GetPartitionCfg(info->sched.cKThread)->id)
	return XM_INVALID_PARAM;
    
    if (!(portTab[port].flags&COMM_PORT_OPENED))
	return XM_INVALID_PARAM;

    if (xmcCommPorts[port].type!=XM_QUEUING_PORT) 
	return XM_INVALID_PARAM;
    
    if (xmcCommPorts[port].direction!=XM_DESTINATION_PORT)
	return XM_INVALID_PARAM;

    if (!msgSize)
	return XM_INVALID_CONFIG;

    if (xmcCommPorts[port].channelId!=XM_NULL_CHANNEL) {
	xmcChannel=&xmcCommChannelTab[xmcCommPorts[port].channelId];
	if (msgSize>xmcChannel->q.maxLength)
	    return XM_INVALID_CONFIG;

        if (CheckGParam(msgPtr, msgSize, 1, PFLAG_NOT_NULL|PFLAG_RW)<0)
            return XM_INVALID_PARAM;

	channel=&channelTab[xmcCommPorts[port].channelId];
        SpinLock(&channel->q.ctrl->spinLock);
	if (channel->q.ctrl->usedMsgs>0) {
	    if (!(msg=(struct msg *)DynListRemoveTailR((xmAddress_t)channel->q.ctrl, &channel->q.ctrl->recvMsgs))) {
                cpuCtxt_t ctxt;
                SpinUnlock(&channel->q.ctrl->spinLock);
                GetCpuCtxt(&ctxt);
    		SystemPanic(&ctxt, "[ReceiveQueuingPort] Queuing channels internal error");
            }
            SpinUnlock(&channel->q.ctrl->spinLock);
	    retSize=(msgSize<msg->length)?msgSize:msg->length;
	    memcpy(msgPtr, (xm_u8_t *)(msg->buffer+(xmAddress_t)channel->q.ctrl), retSize);
#ifdef CONFIG_OBJ_STATUS_ACC
            systemStatus.noQueuingPortMsgsReceived++;
            if (info->sched.cKThread->ctrl.g)
                partitionStatus[GetPartitionCfg(info->sched.cKThread)->id].noQueuingPortMsgsReceived++;
#endif
            SpinLock(&channel->q.ctrl->spinLock);
	    DynListInsertHeadR((xmAddress_t)channel->q.ctrl, &channel->q.ctrl->freeMsgs, &msg->listNode);
    	    channel->q.ctrl->usedMsgs--;
            SpinUnlock(&channel->q.ctrl->spinLock);
            if (channel->q.sender)
                SetExtIrqPending(channel->q.sender, XM_VT_EXT_QUEUING_PORT);
            if(!channel->q.ctrl->usedMsgs) {
                struct guest *g=info->sched.cKThread->ctrl.g;
                xmWord_t *commPortBitmap=(xmWord_t *)((xmAddress_t)g->partCtrlTab+sizeof(partitionControlTable_t)+sizeof(struct xmPhysicalMemMap)*CONFIG_MAX_NO_MAREAS);
                ASSERT((port-GetPartitionCfg(info->sched.cKThread)->commPortsOffset)<g->partCtrlTab->noCommPorts);
                xmClearBit(commPortBitmap, (port-GetPartitionCfg(info->sched.cKThread)->commPortsOffset), xmcTab.noCommPorts);

                if (channel->q.sender) {
                    g=channel->q.sender->ctrl.g;
                    commPortBitmap=(xmWord_t *)((xmAddress_t)g->partCtrlTab+sizeof(partitionControlTable_t)+sizeof(struct xmPhysicalMemMap)*CONFIG_MAX_NO_MAREAS);
                    ASSERT((channel->q.senderPort-GetPartitionCfg(channel->q.sender)->commPortsOffset)<g->partCtrlTab->noCommPorts);
                    xmClearBit(commPortBitmap, channel->q.senderPort, xmcTab.noCommPorts);
                }
            }
	} else {
            SpinUnlock(&channel->q.ctrl->spinLock);
	    return XM_NOT_AVAILABLE;
        }
        SpinUnlock(&channel->q.ctrl->spinLock);
    } else
	return XM_INVALID_CONFIG;

    return retSize;      
}

static inline xm_s32_t GetQPortStatus(xmObjDesc_t desc, xmQueuingPortStatus_t *status) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xm_s32_t port=OBJDESC_GET_ID(desc);

    if (port>=xmcTab.noCommPorts)
	return XM_INVALID_PARAM;

    // Reading a port which does not belong to this partition    
    if (portTab[port].partitionId!=GetPartitionCfg(info->sched.cKThread)->id)
	return XM_INVALID_PARAM;
    
    if (!(portTab[port].flags&COMM_PORT_OPENED))
	return XM_INVALID_PARAM;
    
    if (xmcCommPorts[port].type!=XM_QUEUING_PORT)
	return XM_INVALID_PARAM;

    if (xmcCommPorts[port].channelId!=XM_NULL_CHANNEL) {
        SpinLock(&channelTab[xmcCommPorts[port].channelId].q.ctrl->spinLock); 
	status->noMsgs=channelTab[xmcCommPorts[port].channelId].q.ctrl->usedMsgs;
        SpinUnlock(&channelTab[xmcCommPorts[port].channelId].q.ctrl->spinLock); 
    } else
	memset(status, 0, sizeof(xmQueuingPortStatus_t));
    
    return XM_OK;
}

static inline xm_s32_t GetQPortInfo(xmObjDesc_t desc, xmQueuingPortInfo_t *portInfo) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    struct xmcPartition *partition;
    struct xmcCommChannel *xmcChannel;
    xm_s32_t port;
    
    partition=GetPartitionCfg(info->sched.cKThread);
    
    if (CheckGParam(portInfo->portName, CONFIG_ID_STRING_LENGTH, 1, PFLAG_NOT_NULL)<0) 
        return XM_INVALID_PARAM;
    
    // Look for the channel
    for (port=partition->commPortsOffset; port<(partition->noPorts+partition->commPortsOffset); port++)
	if (!strcmp(portInfo->portName, &xmcStringTab[xmcCommPorts[port].nameOffset])) break;

    if (port>=xmcTab.noCommPorts)
	return XM_INVALID_PARAM;

    if (xmcCommPorts[port].type!=XM_QUEUING_PORT) 
	return XM_INVALID_PARAM;
    
    if (xmcCommPorts[port].channelId!=XM_NULL_CHANNEL) {
	xmcChannel=&xmcCommChannelTab[xmcCommPorts[port].channelId];        
	portInfo->maxNoMsgs=xmcChannel->q.maxNoMsgs;
	portInfo->maxMsgSize=xmcChannel->q.maxLength;
	portInfo->direction=xmcCommPorts[port].direction;
        portInfo->flags=xmcCommPorts[port].flags;
    } else {
        portInfo->maxNoMsgs=0;
	portInfo->maxMsgSize=0;
	portInfo->direction=0;
    }

    return XM_OK;
}

static xm_s32_t CtrlQueuingPort(xmObjDesc_t desc, xm_u32_t cmd, union queuingPortCmd *__gParam args) {
    if (CheckGParam(args, sizeof(union queuingPortCmd), 4, PFLAG_NOT_NULL|PFLAG_RW)<0) return XM_INVALID_PARAM;

    switch(cmd) {
    case XM_COMM_CREATE_PORT:
	if (!args->create.portName||(CheckGParam(args->create.portName, CONFIG_ID_STRING_LENGTH, 1, PFLAG_NOT_NULL)<0)) return XM_INVALID_PARAM;
	return CreateQueuingPort(desc, args->create.portName, args->create.maxNoMsgs, args->create.maxMsgSize, args->create.direction);
    case XM_COMM_GET_PORT_STATUS:
	return GetQPortStatus(desc, &args->status);
    case XM_COMM_GET_PORT_INFO:
	return GetQPortInfo(desc, &args->info);
    }
    return XM_INVALID_PARAM;
}

static const struct object queuingPortObj={
    .Read=(readObjOp_t)ReceiveQueuingPort,
    .Write=(writeObjOp_t)SendQueuingPort,    
    .Ctrl=(ctrlObjOp_t)CtrlQueuingPort,
};

void ResetPartitionPorts(struct xmcPartition *cfg) {
    xm_s32_t port;

    for (port=cfg->commPortsOffset; port<(cfg->noPorts+cfg->commPortsOffset); port++) {
        if (portTab[port].flags&COMM_PORT_OPENED) {            
            if ((xmcCommPorts[port].type==XM_SAMPLING_PORT)&&(xmcCommPorts[port].direction==XM_DESTINATION_PORT))
                channelTab[xmcCommPorts[port].channelId].s.noReceivers=0;
            portTab[port].flags&=~COMM_PORT_OPENED;
        }
    }
}

xm_s32_t SetupComm(void) {
    xm_s32_t e, i;
    xmAddress_t vAddr, ptr;
    struct msg *msgPool;
    GET_MEMZ(channelTab, sizeof(union channel)*xmcTab.noCommChannels);
    GET_MEMZ(portTab, sizeof(struct port)*xmcTab.noCommPorts);
    vAddr=xmcPhysMemAreaTab[xmcTab.hpv.physicalMemoryAreasOffset].mappedAt+xmcPhysMemAreaTab[xmcTab.hpv.physicalMemoryAreasOffset].size;
    /* create the channels */
    for (e=0; e<xmcTab.noCommChannels; e++) {
	switch(xmcCommChannelTab[e].type) {
	case XM_SAMPLING_CHANNEL:
                if (xmcCommChannelTab[e].s.address&&xmcCommChannelTab[e].s.size) {
                ptr=vAddr;      
                // Map the channel
                vAddr=VmMapXm(xmcCommChannelTab[e].s.address, vAddr, xmcCommChannelTab[e].s.size, _PG_ATTR_PRESENT|_PG_ATTR_RW);
                memset((void *)ptr, 0, xmcCommChannelTab[e].s.size);
                channelTab[e].s.ctrl=(struct channelCtrlSampling *)ptr;
            } else {
                GET_MEMZ(channelTab[e].s.ctrl, sizeof(struct channelCtrlSampling)+xmcCommChannelTab[e].s.maxLength);               
            }
            GET_MEMZ(channelTab[e].s.receiverTab, xmcCommChannelTab[e].s.noReceivers*sizeof(kThread_t *));
            GET_MEMZ(channelTab[e].s.receiverPortTab, xmcCommChannelTab[e].s.noReceivers*sizeof(xm_s32_t));
	    break;
	case XM_QUEUING_CHANNEL:
            if (xmcCommChannelTab[e].q.address&&xmcCommChannelTab[e].q.size) {
                xm_s32_t found=0;
                for (i=0; i<xmcTab.noCommPorts; i++) {            
                    if (xmcCommPorts[i].channelId==e) {
                        if ((xmcCommPorts[i].type==XM_QUEUING_CHANNEL)&&
                            (xmcCommPorts[i].direction==XM_SOURCE_PORT)&&
                            !(xmcCommPorts[i].flags&XM_EXTERNAL_PORT)) {
                            found=1;
                            break;
                        }
                    }        
                }

                ptr=vAddr;
                vAddr=VmMapXm(xmcCommChannelTab[e].s.address, vAddr, xmcCommChannelTab[e].s.size, _PG_ATTR_PRESENT|_PG_ATTR_RW);
                memset((void *)ptr, 0, xmcCommChannelTab[e].q.size);
                channelTab[e].q.ctrl=(struct channelCtrlQueuing *)ptr;
                ptr+=sizeof(struct channelCtrlQueuing);
                msgPool=(struct msg *)ptr;
                ptr+=sizeof(struct msg)*xmcCommChannelTab[e].q.maxNoMsgs;
                for (i=0; i<xmcCommChannelTab[e].q.maxNoMsgs; i++) {
                    msgPool[i].buffer=(xmAddress_t)ptr-(xmAddress_t)channelTab[e].q.ctrl;
                    ptr+=xmcCommChannelTab[e].q.maxLength;
                }
                if (found) {
                    DynListInitR(&channelTab[e].q.ctrl->freeMsgs);
                    DynListInitR(&channelTab[e].q.ctrl->recvMsgs);
                    for (i=0; i<xmcCommChannelTab[e].q.maxNoMsgs; i++) {
                        if(DynListInsertHeadR((xmAddress_t)channelTab[e].q.ctrl, &channelTab[e].q.ctrl->freeMsgs, &msgPool[i].listNode)) {
                            cpuCtxt_t ctxt;
                            GetCpuCtxt(&ctxt);
                            SystemPanic(&ctxt, "[SetupComm] Queuing channels initialisation error");
                        }
                    }
                }
            } else {
                GET_MEMZ(channelTab[e].q.ctrl, sizeof(struct channelCtrlQueuing)+(sizeof(struct msg)+xmcCommChannelTab[e].q.maxLength)*xmcCommChannelTab[e].q.maxNoMsgs);
                msgPool=(struct msg *)(channelTab[e].q.ctrl+1);
                for (i=0; i<xmcCommChannelTab[e].q.maxNoMsgs; i++) {
                    msgPool[i].buffer=sizeof(struct channelCtrlQueuing)+
                        (sizeof(struct msg)*xmcCommChannelTab[e].q.maxNoMsgs)+
                        (i*xmcCommChannelTab[e].q.maxLength);
                }
                DynListInitR(&channelTab[e].q.ctrl->freeMsgs);
                DynListInitR(&channelTab[e].q.ctrl->recvMsgs);
                for (i=0; i<xmcCommChannelTab[e].q.maxNoMsgs; i++) {
                    if(DynListInsertHeadR((xmAddress_t)channelTab[e].q.ctrl, &channelTab[e].q.ctrl->freeMsgs, &msgPool[i].listNode)) {
                        cpuCtxt_t ctxt;
                        GetCpuCtxt(&ctxt);
                        SystemPanic(&ctxt, "[SetupComm] Queuing channels initialisation error");
                    }
                }
            }
	    break;
	}
        for (i=0; i<xmcTab.noCommPorts; i++) {            
            if (xmcCommPorts[i].channelId==e) {
                if ((xmcCommPorts[i].type==XM_SAMPLING_CHANNEL)&&
                    (xmcCommPorts[i].direction==XM_DESTINATION_PORT)&&
                    (xmcCommPorts[i].flags&XM_EXTERNAL_PORT)) {                 
                    channelTab[e].s.receiverPortTab[channelTab[e].s.noReceivers]=i;
                    channelTab[e].s.noReceivers++;                    
                }
                if ((xmcCommPorts[i].type==XM_QUEUING_CHANNEL)&&
                    (xmcCommPorts[i].direction==XM_DESTINATION_PORT)&&
                    (xmcCommPorts[i].flags&XM_EXTERNAL_PORT)) {
                    channelTab[e].q.receiver=(void *)-1;
                    channelTab[e].q.receiverPort=i;
                }
            }        
        }
    }
    objectTab[OBJ_CLASS_SAMPLING_PORT]=&samplingPortObj;
    objectTab[OBJ_CLASS_QUEUING_PORT]=&queuingPortObj;
    //SetIrqHandler(COMM_PORT_IPI, CommPortHandler, 0);
    //HwEnableIrq(COMM_PORT_IPI);
    return 0;
}

REGISTER_OBJ(SetupComm);
