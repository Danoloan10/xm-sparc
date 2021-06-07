/*
 * $FILE: commports.c
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
  - [02/09/2015:SPR-020915-04] Remove VBOOT macro and boot.h
*/
#include <assert.h>
#include <list.h>
#include <gaccess.h>
#include <kthread.h>
#include <hypercalls.h>
#include <rsvmem.h>
#include <sched.h>
#include <spinlock.h>
#include <stdc.h>
#include <xmconf.h>
#include <objects/commports.h>

static union channel *channelTab;
static struct port *portTab;

static inline xm_s32_t CreateSamplingPort(xmObjDesc_t desc, xm_s8_t *__gParam portName, xm_u32_t maxMsgSize, xm_u32_t direction, xmTime_t refreshPeriod) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    struct xmcPartition *partition;
    xm_s32_t port;
    xm_u32_t flags;

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

#ifdef CONFIG_SMP_SUPPORT 
    SpinLock(&portTab[port].spinLock);
#endif
    flags=portTab[port].flags;
#ifdef CONFIG_SMP_SUPPORT
    SpinUnlock(&portTab[port].spinLock);
#endif
    if (flags&COMM_PORT_OPENED)
        return XM_NO_ACTION;

    if (xmcCommPorts[port].channelId!=XM_NULL_CHANNEL) {
	ASSERT((xmcCommPorts[port].channelId>=0)&&(xmcCommPorts[port].channelId<xmcTab.noCommChannels));
        
	if (xmcCommChannelTab[xmcCommPorts[port].channelId].s.maxLength!=maxMsgSize)
	    return XM_INVALID_CONFIG;
        if (xmcCommChannelTab[xmcCommPorts[port].channelId].s.refreshPeriod!=refreshPeriod)
            return XM_INVALID_CONFIG;
//
        if (direction==XM_DESTINATION_PORT) {
            ASSERT(channelTab[xmcCommPorts[port].channelId].s.noReceivers<xmcCommChannelTab[xmcCommPorts[port].channelId].s.noReceivers);
            channelTab[xmcCommPorts[port].channelId].s.receiverTab[channelTab[xmcCommPorts[port].channelId].s.noReceivers]=info->sched.cKThread;
            channelTab[xmcCommPorts[port].channelId].s.receiverPortTab[channelTab[xmcCommPorts[port].channelId].s.noReceivers]=port;
            channelTab[xmcCommPorts[port].channelId].s.noReceivers++;        
        } else { // XM_SOURCE_PORT
            channelTab[xmcCommPorts[port].channelId].s.sender=info->sched.cKThread;
            channelTab[xmcCommPorts[port].channelId].s.senderPort=port;
        }   
//     
    }
#ifdef CONFIG_SMP_SUPPORT
    SpinLock(&portTab[port].spinLock);
#endif
    portTab[port].flags|=COMM_PORT_OPENED|COMM_PORT_EMPTY;
    portTab[port].partitionId=GetPartitionCfg(info->sched.cKThread)->id;
#ifdef CONFIG_SMP_SUPPORT
    SpinUnlock(&portTab[port].spinLock);
#endif
    return port;
}

static xm_s32_t ReadSamplingPort(xmObjDesc_t desc,  void *__gParam msgPtr, xmSize_t msgSize, xm_u32_t *__gParam flags) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xm_s32_t port=OBJDESC_GET_ID(desc);
    struct xmcPartition *cfg;
    struct xmcCommChannel *xmcChannel;
    union channel *channel;
    xmSize_t retSize=0;
    struct guest *g;
    xmWord_t *commPortBitmap;
    xm_u32_t portFlags;
    xmId_t partId;

    if (CheckGParam(flags, sizeof(xm_u32_t), 4, PFLAG_RW)<0)
        return XM_INVALID_PARAM;

    if (port>=xmcTab.noCommPorts)
	return XM_INVALID_PARAM;

#ifdef CONFIG_SMP_SUPPORT
    SpinLock(&portTab[port].spinLock);
#endif
    portFlags=portTab[port].flags;
    partId=portTab[port].partitionId;
#ifdef CONFIG_SMP_SUPPORT
    SpinUnlock(&portTab[port].spinLock);
#endif
     if (partId!=GetPartitionCfg(info->sched.cKThread)->id)
	return XM_INVALID_PARAM;

     if (!(portFlags&COMM_PORT_OPENED))
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
#ifdef CONFIG_SMP_SUPPORT
        SpinLock(&channel->s.lock);
        channel->s.noReaders++;
        SpinUnlock(&channel->s.lock);

	retSize=(msgSize<channel->s.length[channel->s.rBuffer])?msgSize:channel->s.length[channel->s.rBuffer];
	memcpy(msgPtr, channel->s.buffer[channel->s.rBuffer], retSize);
#else
        retSize=(msgSize<channel->s.length)?msgSize:channel->s.length;
	memcpy(msgPtr, channel->s.buffer, retSize);
#endif
#ifdef CONFIG_SMP_SUPPORT
        SpinLock(&portTab[port].spinLock);
#endif
        portTab[port].flags&=~COMM_PORT_MSG_MASK;
        portTab[port].flags|=COMM_PORT_CONSUMED_MSG;
#ifdef CONFIG_SMP_SUPPORT
        SpinUnlock(&portTab[port].spinLock);
#endif
#ifdef CONFIG_OBJ_STATUS_ACC
        systemStatus.noSamplingPortMsgsRead++;
        if (info->sched.cKThread->ctrl.g)
            partitionStatus[GetPartitionCfg(info->sched.cKThread)->id].noSamplingPortMsgsRead++;
#endif

        g=info->sched.cKThread->ctrl.g;
        cfg=GetPartitionCfg(info->sched.cKThread);
        commPortBitmap=(xmWord_t *)((xmAddress_t)g->partCtrlTab+sizeof(partitionControlTable_t)+sizeof(struct xmPhysicalMemMap)*CONFIG_MAX_NO_MAREAS);
        ASSERT((port-cfg->commPortsOffset)<g->partCtrlTab->noCommPorts);
        xmClearBit(commPortBitmap, port, xmcTab.noCommPorts);

        if (channel->s.sender) {
            g=channel->s.sender->ctrl.g;
            cfg=GetPartitionCfg(channel->s.sender);
            commPortBitmap=(xmWord_t *)((xmAddress_t)g->partCtrlTab+sizeof(partitionControlTable_t)+sizeof(struct xmPhysicalMemMap)*CONFIG_MAX_NO_MAREAS);
            xmClearBit(commPortBitmap, channel->s.senderPort, xmcTab.noCommPorts);
            SetPartitionExtIrqPending(GetPartition(channel->s.sender), XM_VT_EXT_SAMPLING_PORT);
            portTab[channel->s.senderPort].flags&=~COMM_PORT_MSG_MASK;
            portTab[channel->s.senderPort].flags|=COMM_PORT_CONSUMED_MSG;

        }
	if (flags) {
	    *flags=0;
#ifdef CONFIG_SMP_SUPPORT
	    if (retSize&&(xmcChannel->s.refreshPeriod!=XM_INFINITE_TIME)&&(channel->s.timestamp[channel->s.rBuffer]+xmcChannel->s.refreshPeriod)>GetSysClockUsec())
		*flags=XM_COMM_MSG_VALID;
#else
            if (retSize&&(xmcChannel->s.refreshPeriod!=XM_INFINITE_TIME)&&(channel->s.timestamp+xmcChannel->s.refreshPeriod)>GetSysClockUsec())
		*flags=XM_COMM_MSG_VALID;
#endif
        }
#ifdef CONFIG_SMP_SUPPORT
        SpinLock(&channel->s.lock);
        channel->s.noReaders--;
        if (channel->s.noReaders==0) {
            if (channel->s.flags&SAMPLING_ICHANNEL_USED) {
                xm_s32_t tmp = channel->s.rBuffer;
                channel->s.rBuffer = channel->s.iBuffer;
                channel->s.iBuffer = tmp;
                channel->s.flags&=~SAMPLING_ICHANNEL_USED;
            }
        }
        SpinUnlock(&channel->s.lock);
#endif
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
    struct xmcPartition *cfg;
    xm_u32_t flags;
    xmId_t partId;


    if (port>=xmcTab.noCommPorts)
	return XM_INVALID_PARAM;

#ifdef CONFIG_SMP_SUPPORT
    SpinLock(&portTab[port].spinLock);
#endif 
    flags=portTab[port].flags;
    partId=portTab[port].partitionId;
#ifdef CONFIG_SMP_SUPPORT
    SpinUnlock(&portTab[port].spinLock);
#endif

   // Reading a port which does not belong to this partition    
    if (partId!=GetPartitionCfg(info->sched.cKThread)->id)
	return XM_INVALID_PARAM;

    if (!(flags&COMM_PORT_OPENED))
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
#ifdef CONFIG_SMP_SUPPORT
	memcpy(channel->s.buffer[channel->s.sBuffer], msgPtr, msgSize);        
#else
        memcpy(channel->s.buffer, msgPtr, msgSize);
#endif
        portTab[port].flags&=~COMM_PORT_MSG_MASK;
        portTab[port].flags|=COMM_PORT_NEW_MSG;
#ifdef CONFIG_OBJ_STATUS_ACC
        systemStatus.noSamplingPortMsgsWritten++;
        if (info->sched.cKThread->ctrl.g)
            partitionStatus[GetPartitionCfg(info->sched.cKThread)->id].noSamplingPortMsgsWritten++;
#endif
        ASSERT(channel->s.sender==info->sched.cKThread);
        g=info->sched.cKThread->ctrl.g;
        cfg=GetPartitionCfg(info->sched.cKThread);
        commPortBitmap=(xmWord_t *)((xmAddress_t)g->partCtrlTab+sizeof(partitionControlTable_t)+sizeof(struct xmPhysicalMemMap)*CONFIG_MAX_NO_MAREAS);
        xmSetBit(commPortBitmap, port, xmcTab.noCommPorts);
        for (e=0; e<channel->s.noReceivers; e++) {
            g=channel->s.receiverTab[e]->ctrl.g;
            cfg=GetPartitionCfg(channel->s.receiverTab[e]);
            commPortBitmap=(xmWord_t *)((xmAddress_t)g->partCtrlTab+sizeof(partitionControlTable_t)+sizeof(struct xmPhysicalMemMap)*CONFIG_MAX_NO_MAREAS);
            if (channel->s.receiverTab[e]) {
                ASSERT(channel->s.receiverPortTab[e]-cfg->commPortsOffset < g->partCtrlTab->noCommPorts);

                xmSetBit(commPortBitmap, channel->s.receiverPortTab[e], xmcTab.noCommPorts);
		SetPartitionExtIrqPending(GetPartition(channel->s.receiverTab[e]), XM_VT_EXT_SAMPLING_PORT);
                
                portTab[channel->s.receiverPortTab[e]].flags&=~COMM_PORT_MSG_MASK;
                portTab[channel->s.receiverPortTab[e]].flags|=COMM_PORT_NEW_MSG;
            }
        }
#ifdef CONFIG_SMP_SUPPORT
	channel->s.length[channel->s.sBuffer]=msgSize;
        channel->s.timestamp[channel->s.sBuffer]=GetSysClockUsec();	
        SpinLock(&channel->s.lock);
        if (channel->s.noReaders==0) {
            xm_s32_t tmp = channel->s.sBuffer;
            channel->s.sBuffer = channel->s.rBuffer;
            channel->s.rBuffer = tmp;
        } else {
            xm_s32_t tmp = channel->s.sBuffer;
            channel->s.sBuffer = channel->s.iBuffer;
            channel->s.iBuffer = tmp;
            channel->s.flags|=SAMPLING_ICHANNEL_USED;
        }
        SpinUnlock(&channel->s.lock);
#else
        channel->s.length=msgSize;
        channel->s.timestamp=GetSysClockUsec();
#endif
    }
    else {
        return XM_INVALID_CONFIG;
    }
    return XM_OK;
}

static inline xm_s32_t GetSPortInfo(xmObjDesc_t desc, xmSamplingPortInfo_t *info) {
    localProcessor_t *infoProcessor=GET_LOCAL_PROCESSOR();
    struct xmcPartition *partition;
    struct xmcCommChannel *xmcChannel;
    xm_s32_t port;
    
    partition=GetPartitionCfg(infoProcessor->sched.cKThread);
    
    if (CheckGParam(info->portName, CONFIG_ID_STRING_LENGTH, 1, PFLAG_NOT_NULL)<0) 
        return XM_INVALID_PARAM;

    // Look for the channel
    for (port=partition->commPortsOffset; port<(partition->noPorts+partition->commPortsOffset); port++)
	if (!strcmp(info->portName, &xmcStringTab[xmcCommPorts[port].nameOffset])) break;

    if (port>=xmcTab.noCommPorts)
	return XM_INVALID_PARAM;

    if (xmcCommPorts[port].type!=XM_SAMPLING_PORT) 
	return XM_INVALID_PARAM;
    
    if (xmcCommPorts[port].channelId!=XM_NULL_CHANNEL) {
	xmcChannel=&xmcCommChannelTab[xmcCommPorts[port].channelId];
	info->refreshPeriod=xmcChannel->s.refreshPeriod;
	info->maxMsgSize=xmcChannel->s.maxLength;
	info->direction=xmcCommPorts[port].direction;
    } else {
	info->refreshPeriod=XM_INFINITE_TIME;
	info->maxMsgSize=0;
	info->direction=0;
    }
    info->objDesc = OBJDESC_SET_ID(desc, port);
    return XM_OK;
}

static inline xm_s32_t GetSPortStatus(xmObjDesc_t desc, xmSamplingPortStatus_t *status) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xm_s32_t port=OBJDESC_GET_ID(desc);
    struct xmcCommChannel *xmcChannel;
    union channel *channel;
    xm_u32_t flags;
    xmId_t partId;

    if (port>=xmcTab.noCommPorts)
	return XM_INVALID_PARAM;

#ifdef CONFIG_SMP_SUPPORT
    SpinLock(&portTab[port].spinLock);
#endif
    flags=portTab[port].flags;
    partId=portTab[port].partitionId;
#ifdef CONFIG_SMP_SUPPORT
    SpinUnlock(&portTab[port].spinLock);
#endif

    // Reading a port which does not belong to this partition    
    if (partId!=GetPartitionCfg(info->sched.cKThread)->id)
	return XM_INVALID_PARAM;
    
    if (!(flags&COMM_PORT_OPENED))
	return XM_INVALID_PARAM;
    
    if (xmcCommPorts[port].type!=XM_SAMPLING_PORT) 
	return XM_INVALID_PARAM;

    if (xmcCommPorts[port].channelId!=XM_NULL_CHANNEL) {
	xmcChannel=&xmcCommChannelTab[xmcCommPorts[port].channelId];
	status->flags=0;
	channel=&channelTab[xmcCommPorts[port].channelId];
#ifdef CONFIG_SMP_SUPPORT
        if (SpinIsLocked(&channel->s.lock))
            return XM_NOT_AVAILABLE;
        SpinLock(&channel->s.lock);
        status->lastMsgSize=channel->s.length[channel->s.rBuffer];
        status->timestamp=channel->s.timestamp[channel->s.rBuffer];
        SpinUnlock(&channel->s.lock);
#else
        status->lastMsgSize=channel->s.length;
        status->timestamp=channel->s.timestamp;
#endif
        if (portTab[port].flags&COMM_PORT_NEW_MSG){
            status->flags|=XM_COMM_NEW_MSG;
        } else if (portTab[port].flags&COMM_PORT_CONSUMED_MSG) {
            status->flags|=XM_COMM_CONSUMED_MSG;
        } else {
            status->flags|=XM_COMM_EMPTY_PORT;
        }
#ifdef CONFIG_SMP_SUPPORT
        SpinLock(&channel->s.lock);
	if (channel->s.timestamp[channel->s.rBuffer]&&(xmcChannel->s.refreshPeriod!=XM_INFINITE_TIME)&&((channel->s.timestamp[channel->s.rBuffer]+xmcChannel->s.refreshPeriod)>GetSysClockUsec()))
	    status->flags|=XM_COMM_MSG_VALID;    
        SpinUnlock(&channel->s.lock);
#else
        if (channel->s.timestamp&&(xmcChannel->s.refreshPeriod!=XM_INFINITE_TIME)&&((channel->s.timestamp+xmcChannel->s.refreshPeriod)>GetSysClockUsec()))
	    status->flags|=XM_COMM_MSG_VALID;    
#endif   
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
    xm_u32_t flags;

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
    flags=portTab[port].flags;

    if (flags&COMM_PORT_OPENED)
        return XM_NO_ACTION;

    if (xmcCommPorts[port].channelId!=XM_NULL_CHANNEL) {
	ASSERT((xmcCommPorts[port].channelId>=0)&&(xmcCommPorts[port].channelId<xmcTab.noCommChannels));

	if (direction==XM_DESTINATION_PORT) {
	    channelTab[xmcCommPorts[port].channelId].q.receiver=info->sched.cKThread;
            channelTab[xmcCommPorts[port].channelId].q.receiverPort=port;
	} else { // XM_SOURCE_PORT
            channelTab[xmcCommPorts[port].channelId].q.sender=info->sched.cKThread;
            channelTab[xmcCommPorts[port].channelId].q.senderPort=port;
        }  

	if (xmcCommChannelTab[xmcCommPorts[port].channelId].q.maxNoMsgs!=maxNoMsgs)
	    return XM_INVALID_CONFIG;
	if (xmcCommChannelTab[xmcCommPorts[port].channelId].q.maxLength!=maxMsgSize)
	    return XM_INVALID_CONFIG;	
    }
#ifdef CONFIG_SMP_SUPPORT
    SpinLock(&portTab[port].spinLock);
#endif
    portTab[port].flags|=COMM_PORT_OPENED;
    portTab[port].partitionId=GetPartitionCfg(info->sched.cKThread)->id;
#ifdef CONFIG_SMP_SUPPORT
    SpinUnlock(&portTab[port].spinLock);
#endif
    return port;
}

static xm_s32_t SendQueuingPort(xmObjDesc_t desc, void *__gParam msgPtr, xm_u32_t msgSize) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xm_s32_t port=OBJDESC_GET_ID(desc);
    struct xmcCommChannel *xmcChannel;
    union channel *channel;
    struct msg *msg;
    struct guest *g;
    xmWord_t *commPortBitmap;
    struct xmcPartition *cfg;
    xm_u32_t flags;
    xmId_t partId;


    if (port>=xmcTab.noCommPorts)
	return XM_INVALID_PARAM;

#ifdef CONFIG_SMP_SUPPORT
    SpinLock(&portTab[port].spinLock);
#endif
    flags=portTab[port].flags;
    partId=portTab[port].partitionId;
#ifdef CONFIG_SMP_SUPPORT
    SpinUnlock(&portTab[port].spinLock);
#endif

    // Reading a port which does not belong to this partition    
    if (partId!=GetPartitionCfg(info->sched.cKThread)->id)
	return XM_INVALID_PARAM;
    if (!(flags&COMM_PORT_OPENED))
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
        SpinLock(&channel->q.lock);
	if (channel->q.usedMsgs<xmcChannel->q.maxNoMsgs) {
    	    if (!(msg=(struct msg *)DynListRemoveTail(&channel->q.freeMsgs))) {
                cpuCtxt_t ctxt;
                SpinUnlock(&channel->q.lock);
                GetCpuCtxt(&ctxt);
    		SystemPanic(&ctxt, "[SendQueuingPort] Queuing channels internal error");
            }
            SpinUnlock(&channel->q.lock);
    	    memcpy(msg->buffer, msgPtr, msgSize);
#ifdef CONFIG_OBJ_STATUS_ACC
            systemStatus.noQueuingPortMsgsSent++;
            if (info->sched.cKThread->ctrl.g)
                partitionStatus[GetPartitionCfg(info->sched.cKThread)->id].noQueuingPortMsgsSent++;
#endif
	    msg->length=msgSize;
            SpinLock(&channel->q.lock);
    	    DynListInsertHead(&channel->q.recvMsgs, &msg->listNode);
    	    channel->q.usedMsgs++;

            SpinUnlock(&channel->q.lock);
	    if (channel->q.receiver) {
                g=channel->q.receiver->ctrl.g;
                cfg=GetPartitionCfg(channel->q.receiver);
                commPortBitmap=(xmWord_t *)((xmAddress_t)g->partCtrlTab+sizeof(partitionControlTable_t)+sizeof(struct xmPhysicalMemMap)*CONFIG_MAX_NO_MAREAS);
                ASSERT((channel->q.receiverPort - cfg->commPortsOffset) < g->partCtrlTab->noCommPorts);
                xmSetBit(commPortBitmap, channel->q.receiverPort, xmcTab.noCommPorts);
		SetPartitionExtIrqPending(GetPartition(channel->q.receiver), XM_VT_EXT_QUEUING_PORT);
	    }

            g=info->sched.cKThread->ctrl.g;
            cfg=GetPartitionCfg(info->sched.cKThread);
            commPortBitmap=(xmWord_t *)((xmAddress_t)g->partCtrlTab+sizeof(partitionControlTable_t)+sizeof(struct xmPhysicalMemMap)*CONFIG_MAX_NO_MAREAS);
            ASSERT(channel->q.sender==info->sched.cKThread);
            xmSetBit(commPortBitmap, channel->q.senderPort, xmcTab.noCommPorts);

    	} else {
            SpinUnlock(&channel->q.lock);
    	    return XM_NOT_AVAILABLE;
        }
        SpinUnlock(&channel->q.lock);
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
    struct msg *msg;
    xm_u32_t flags;
    xmId_t partId;

    if (port>=xmcTab.noCommPorts)
	return XM_INVALID_PARAM;
#ifdef CONFIG_SMP_SUPPORT
    SpinLock(&portTab[port].spinLock);
#endif
    flags=portTab[port].flags;
    partId=portTab[port].partitionId;
#ifdef CONFIG_SMP_SUPPORT
    SpinUnlock(&portTab[port].spinLock);
#endif

    // Reading a port which does not belong to this partition
    if (partId!=GetPartitionCfg(info->sched.cKThread)->id)
	return XM_INVALID_PARAM;
    
    if (!(flags&COMM_PORT_OPENED))
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
        SpinLock(&channel->q.lock);
	if (channel->q.usedMsgs>0) {
	    if (!(msg=(struct msg *)DynListRemoveTail(&channel->q.recvMsgs))) {
                cpuCtxt_t ctxt;
                SpinUnlock(&channel->q.lock);
                GetCpuCtxt(&ctxt);
    		SystemPanic(&ctxt, "[ReceiveQueuingPort] Queuing channels internal error");
            }
            SpinUnlock(&channel->q.lock);
	    retSize=(msgSize<msg->length)?msgSize:msg->length;
	    memcpy(msgPtr, msg->buffer, retSize);
#ifdef CONFIG_OBJ_STATUS_ACC
            systemStatus.noQueuingPortMsgsReceived++;
            if (info->sched.cKThread->ctrl.g)
                partitionStatus[GetPartitionCfg(info->sched.cKThread)->id].noQueuingPortMsgsReceived++;
#endif
            SpinLock(&channel->q.lock);
	    DynListInsertHead(&channel->q.freeMsgs, &msg->listNode);
    	    channel->q.usedMsgs--;
            SpinUnlock(&channel->q.lock);
            if (channel->q.sender)
                SetPartitionExtIrqPending(GetPartition(channel->q.sender), XM_VT_EXT_QUEUING_PORT);
            
            if(!channel->q.usedMsgs) {
                struct guest *g=info->sched.cKThread->ctrl.g;
                struct xmcPartition *cfg=GetPartitionCfg(info->sched.cKThread);
                xmWord_t *commPortBitmap=(xmWord_t *)((xmAddress_t)g->partCtrlTab+sizeof(partitionControlTable_t)+sizeof(struct xmPhysicalMemMap)*CONFIG_MAX_NO_MAREAS);

                ASSERT((port-cfg->commPortsOffset)<g->partCtrlTab->noCommPorts);
                xmClearBit(commPortBitmap, (port-cfg->commPortsOffset), xmcTab.noCommPorts);

                if (channel->q.sender) {
                    g=channel->q.sender->ctrl.g;
                    cfg=GetPartitionCfg(channel->q.sender);
                    commPortBitmap=(xmWord_t *)((xmAddress_t)g->partCtrlTab+sizeof(partitionControlTable_t)+sizeof(struct xmPhysicalMemMap)*CONFIG_MAX_NO_MAREAS);
                    ASSERT((channel->q.senderPort-cfg->commPortsOffset)<g->partCtrlTab->noCommPorts);
                    xmClearBit(commPortBitmap, channel->q.senderPort, xmcTab.noCommPorts);
                }
            }
	} else {
            SpinUnlock(&channel->q.lock);
	    return XM_NOT_AVAILABLE;
        }
        SpinUnlock(&channel->q.lock);
    } else
	return XM_INVALID_CONFIG;

    return retSize;      
}

static inline xm_s32_t GetQPortStatus(xmObjDesc_t desc, xmQueuingPortStatus_t *status) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xm_s32_t port=OBJDESC_GET_ID(desc);
    xm_u32_t flags;
    xmId_t partId;

    if (port>=xmcTab.noCommPorts)
	return XM_INVALID_PARAM;
#ifdef CONFIG_SMP_SUPPORT
    SpinLock(&portTab[port].spinLock);
#endif
    flags=portTab[port].flags;
    partId=portTab[port].partitionId;
#ifdef CONFIG_SMP_SUPPORT
    SpinUnlock(&portTab[port].spinLock);
#endif

    // Reading a port which does not belong to this partition    
    if (partId!=GetPartitionCfg(info->sched.cKThread)->id)
	return XM_INVALID_PARAM;
    
    if (!(flags&COMM_PORT_OPENED))
	return XM_INVALID_PARAM;
    if (xmcCommPorts[port].type!=XM_QUEUING_PORT)
	return XM_INVALID_PARAM;

    if (xmcCommPorts[port].channelId!=XM_NULL_CHANNEL) {
#ifdef CONFIG_SMP_SUPPORT
        SpinLock(&channelTab[xmcCommPorts[port].channelId].q.lock);
#endif
	status->noMsgs=channelTab[xmcCommPorts[port].channelId].q.usedMsgs;
#ifdef CONFIG_SMP_SUPPORT
        SpinUnlock(&channelTab[xmcCommPorts[port].channelId].q.lock);
#endif
    } else
	memset(status, 0, sizeof(xmQueuingPortStatus_t));
    
    return XM_OK;
}

static inline xm_s32_t GetQPortInfo(xmObjDesc_t desc, xmQueuingPortInfo_t *info) {
    localProcessor_t *infoProcessor=GET_LOCAL_PROCESSOR();
    struct xmcPartition *partition;
    struct xmcCommChannel *xmcChannel;
    xm_s32_t port;
    
    partition=GetPartitionCfg(infoProcessor->sched.cKThread);
    
    if (CheckGParam(info->portName, CONFIG_ID_STRING_LENGTH, 1, PFLAG_NOT_NULL)<0) 
        return XM_INVALID_PARAM;
    
    // Look for the channel
    for (port=partition->commPortsOffset; port<(partition->noPorts+partition->commPortsOffset); port++)
	if (!strcmp(info->portName, &xmcStringTab[xmcCommPorts[port].nameOffset])) break;

    if (port>=xmcTab.noCommPorts)
	return XM_INVALID_PARAM;

    if (xmcCommPorts[port].type!=XM_QUEUING_PORT) 
	return XM_INVALID_PARAM;
    
    if (xmcCommPorts[port].channelId!=XM_NULL_CHANNEL) {
	xmcChannel=&xmcCommChannelTab[xmcCommPorts[port].channelId];        
	info->maxNoMsgs=xmcChannel->q.maxNoMsgs;
	info->maxMsgSize=xmcChannel->q.maxLength;
	info->direction=xmcCommPorts[port].direction;
        info->flags=xmcCommPorts[port].flags;
    } else {
        info->maxNoMsgs=0;
	info->maxMsgSize=0;
	info->direction=0;
    }
    info->objDesc = OBJDESC_SET_ID(desc, port);
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

    GET_MEMZ(channelTab, sizeof(union channel)*xmcTab.noCommChannels);
    GET_MEMZ(portTab, sizeof(struct port)*xmcTab.noCommPorts);

    /* create the channels */
    for (e=0; e<xmcTab.noCommChannels; e++) {
	switch(xmcCommChannelTab[e].type) {
	case XM_SAMPLING_CHANNEL:
#ifdef CONFIG_SMP_SUPPORT
	    GET_MEMZ(channelTab[e].s.buffer[0], xmcCommChannelTab[e].s.maxLength*3);
#else
            GET_MEMZ(channelTab[e].s.buffer, xmcCommChannelTab[e].s.maxLength);
#endif
            GET_MEMZ(channelTab[e].s.receiverTab, xmcCommChannelTab[e].s.noReceivers*sizeof(kThread_t *));
            GET_MEMZ(channelTab[e].s.receiverPortTab, xmcCommChannelTab[e].s.noReceivers*sizeof(xm_s32_t));
#ifdef CONFIG_SMP_SUPPORT
            for (i=0; i<2; i++)
                channelTab[e].s.buffer[i+1]=channelTab[e].s.buffer[i]+xmcCommChannelTab[e].s.maxLength;
            channelTab[e].s.sBuffer = 0;
            channelTab[e].s.iBuffer = 1;
            channelTab[e].s.rBuffer = 2;
            channelTab[e].s.noReaders = 0;
            channelTab[e].s.flags = 0;
            channelTab[e].s.lock = SPINLOCK_INIT;
#endif
	    break;
	case XM_QUEUING_CHANNEL:
	    GET_MEMZ(channelTab[e].q.msgPool, sizeof(struct msg)*xmcCommChannelTab[e].q.maxNoMsgs);
            channelTab[e].q.lock = SPINLOCK_INIT;
	    DynListInit(&channelTab[e].q.freeMsgs);
	    DynListInit(&channelTab[e].q.recvMsgs);
	    for (i=0; i<xmcCommChannelTab[e].q.maxNoMsgs; i++) {
		GET_MEMZ(channelTab[e].q.msgPool[i].buffer, xmcCommChannelTab[e].q.maxLength);
		if(DynListInsertHead(&channelTab[e].q.freeMsgs, &channelTab[e].q.msgPool[i].listNode)) {
                    cpuCtxt_t ctxt;
                    GetCpuCtxt(&ctxt);
		    SystemPanic(&ctxt, "[SetupComm] Queuing channels initialisation error");
                }
	    }
	    break;
	}
    }
    objectTab[OBJ_CLASS_SAMPLING_PORT]=&samplingPortObj;
    objectTab[OBJ_CLASS_QUEUING_PORT]=&queuingPortObj;    

    return 0;
}

REGISTER_OBJ(SetupComm);
