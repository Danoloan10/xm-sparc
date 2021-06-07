/*
 * $FILE: status.h
 *
 * System/partition status
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */
/*
  Changelog:
  - [18/11/2015:SPR-221015-01] edacErrorCounter added to xmSystemStatus_t
*/

#ifndef _XM_OBJ_STATUS_H_
#define _XM_OBJ_STATUS_H_

/* <track id="system-status"> */
typedef struct {
    xm_u32_t resetStatus;
    xm_u32_t resetCounter;
    /* Number of HM events emitted. */
    xm_u64_t noHmEvents;                /* [[OPTIONAL]] */
    /* Number of HW interrupts received. */
    xm_u64_t noIrqs;                    /* [[OPTIONAL]] */
    /* Current major cycle interation. */
    xm_u64_t currentMaf;                /* [[OPTIONAL]] */
    /* Total number of system messages: */
    xm_u64_t noSamplingPortMsgsRead;    /* [[OPTIONAL]] */
    xm_u64_t noSamplingPortMsgsWritten; /* [[OPTIONAL]] */
    xm_u64_t noQueuingPortMsgsSent;     /* [[OPTIONAL]] */
    xm_u64_t noQueuingPortMsgsReceived; /* [[OPTIONAL]] */
    xmTime_t irqLastOccurence[CONFIG_NO_HWIRQS];
#ifdef CONFIG_LEON_EDAC_SUPPORT
    xm_u32_t edacErrorCounter;
#endif
} xmSystemStatus_t;
/* </track id="system-status"> */

//@ \void{<track id="plan-status">}
typedef struct {
    xmTime_t switchTime;
    xm_s32_t next;
    xm_s32_t current;
    xm_s32_t prev;
} xmPlanStatus_t;
//@ \void{</track id="plan-status">} 

/* <track id="partition-status"> */
typedef struct {
    /* Current state of the partition: ready, suspended ... */
    xm_u32_t state[CONFIG_NO_VCPUS];
#define XM_STATUS_IDLE 0x0
#define XM_STATUS_READY 0x1
#define XM_STATUS_SUSPENDED 0x2
#define XM_STATUS_HALTED 0x3
    /* Number of virtual interrupts received. */
    xm_u64_t noVIrqs;                   /* [[OPTIONAL]] */
    /* Reset information */
    xm_u32_t resetCounter;
    xm_u32_t resetStatus;
    xmTime_t execClock[CONFIG_NO_VCPUS];
    /* Total number of partition messages: */
    xm_u64_t noSamplingPortMsgsRead;    /* [[OPTIONAL]] */
    xm_u64_t noSamplingPortMsgsWritten; /* [[OPTIONAL]] */
    xm_u64_t noQueuingPortMsgsSent;     /* [[OPTIONAL]] */
    xm_u64_t noQueuingPortMsgsReceived; /* [[OPTIONAL]] */
    xmTime_t irqLastOccurence[CONFIG_NO_HWIRQS];
} xmPartitionStatus_t;
/* </track id="partition-status"> */

#define XM_GET_SYSTEM_STATUS 0x0
#define XM_GET_SCHED_PLAN_STATUS 0x1

union statusCmd {
    union {
	xmSystemStatus_t system;
	xmPartitionStatus_t partition;
        xmPlanStatus_t plan;
    } status;
};

#ifdef _XM_KERNEL_
extern xmSystemStatus_t systemStatus;
extern xmPartitionStatus_t *partitionStatus;
#endif
#endif
