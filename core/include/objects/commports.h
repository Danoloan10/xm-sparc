/*
 * $FILE: commports.h
 *
 * Communication port object definitions
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_OBJ_COMMPORTS_H_
#define _XM_OBJ_COMMPORTS_H_

// Commands
#define XM_COMM_CREATE_PORT 0x0
#define XM_COMM_GET_PORT_STATUS 0x1
#define XM_COMM_GET_PORT_INFO 0x2

#ifdef _XM_KERNEL_
#include <kthread.h>
#else
#ifndef __gParam
#define __gParam
#endif
#endif

/* <track id="sampling-status-type"> */
typedef struct {
    xmTime_t timestamp;
    xm_u32_t lastMsgSize;
    xm_u32_t flags;
//#define XM_COMM_MSG_VALID 0x1
#define XM_COMM_PORT_STATUS_B 1
#define XM_COMM_PORT_STATUS_M 0x6
#define XM_COMM_EMPTY_PORT 0x0
#define XM_COMM_CONSUMED_MSG (XM_COMM_PORT_STATUS_B<<1)
#define XM_COMM_NEW_MSG (XM_COMM_PORT_STATUS_B<<2)
} xmSamplingPortStatus_t;
/* </track id="sampling-status-type"> */

//@ \void{<track id="sampling-info-type">}
typedef struct {
    char *__gParam portName;
#define XM_INFINITE_TIME ((xm_u32_t)-1)
    xmTime_t refreshPeriod; // Refresh period.
    xm_u32_t maxMsgSize;  // Max message size.
    xm_u32_t direction;
    xmObjDesc_t objDesc; // object descriptor, valid if the port is opened
} xmSamplingPortInfo_t;
//@ \void{</track id="sampling-info-type">}

/* <track id="queuing-status-type"> */
typedef struct {
    xm_u32_t noMsgs;       // Current number of messages.
    //xm_u32_t flags;
} xmQueuingPortStatus_t;
/* </track id="queuing-status-type"> */

//@ \void{<track id="queuing-info-type">}
typedef struct {
    char *__gParam portName;
    xm_u32_t maxMsgSize; // Max message size.
    xm_u32_t maxNoMsgs; // Max number of messages.
    xm_u32_t direction;
    xm_u32_t flags;
    xmObjDesc_t objDesc; // object descriptor, valid if the port is opened
} xmQueuingPortInfo_t;
//@ \void{</track id="queuing-info-type">}

union samplingPortCmd {
    struct createSCmd {
	char *__gParam portName; 
	xm_u32_t maxMsgSize; 
	xm_u32_t direction;
        xmTime_t refreshPeriod;
    } create;
    xmSamplingPortStatus_t status;
    xmSamplingPortInfo_t info;
};

union queuingPortCmd {
    struct createQCmd {
	char *__gParam portName; 
	xm_u32_t maxNoMsgs;
	xm_u32_t maxMsgSize;
	xm_u32_t direction;
    } create;
    xmQueuingPortStatus_t status;
    xmQueuingPortInfo_t info;
};

#define XM_COMM_MSG_VALID 0x1

#ifdef _XM_KERNEL_
#ifdef CONFIG_AMP_SUPPORT
#include <spinlock.h>
#include <list2.h>

struct channelCtrlSampling {
    spinLock_t spinLock;
    volatile xm_s32_t length;
    volatile xmTime_t timestamp;
    volatile xm_u32_t flags;
#define CHANNEL_NEW_MSG 0x1
};

struct msg {
    struct dynListNodeR listNode;
    xmAddress_t buffer;
    volatile xm_s32_t length;
    volatile xmTime_t timestamp;
}; 

struct channelCtrlQueuing {
    spinLock_t spinLock;
    struct dynListR freeMsgs, recvMsgs;
    xm_s32_t usedMsgs;
    volatile xm_u32_t flags;
};

union channel {
    struct {
        struct channelCtrlSampling *ctrl;
        kThread_t **receiverTab;
        xm_s32_t *receiverPortTab;
        xm_s32_t noReceivers;
        kThread_t *sender;
        xm_s32_t senderPort;
    } s;
    struct {
        struct channelCtrlQueuing *ctrl;
	kThread_t *receiver;
        xm_s32_t receiverPort;
        kThread_t *sender;
        xm_s32_t senderPort;
    } q;
};

#else

union channel {
    struct {
#ifdef CONFIG_SMP_SUPPORT
        spinLock_t lock;
        char *buffer[3];
	xm_s32_t length[3];
	xmTime_t timestamp[3];

        xm_s32_t sBuffer;
        xm_s32_t iBuffer;
        xm_s32_t rBuffer;
        xm_s32_t noReaders;
        xm_u32_t flags;
#define SAMPLING_ICHANNEL_USED 0x1
#else
        char *buffer;
	xm_s32_t length;
	xmTime_t timestamp;
#endif
        kThread_t **receiverTab;
        xm_s32_t *receiverPortTab;
        xm_s32_t noReceivers;
        kThread_t *sender;
        xm_s32_t senderPort;
    } s;
    struct {
        spinLock_t lock;
	struct msg {
	    struct dynListNode listNode;
	    char *buffer;
	    xm_s32_t length;
	    xmTime_t timestamp;
	} *msgPool;
	struct dynList freeMsgs, recvMsgs;
	xm_s32_t usedMsgs;
	kThread_t *receiver;
        xm_s32_t receiverPort;
        kThread_t *sender;
        xm_s32_t senderPort;
    } q;
};

#endif

struct port {   
    spinLock_t spinLock;
    xm_u32_t flags;
#define COMM_PORT_OPENED 0x1
#define COMM_PORT_EMPTY 0x0
#define COMM_PORT_NEW_MSG 0x2
#define COMM_PORT_CONSUMED_MSG 0x4
#define COMM_PORT_MSG_MASK 0x6
    xmId_t partitionId;
};

#endif

#endif
