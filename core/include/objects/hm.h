/*
 * $FILE: hm.h
 *
 * Health Monitor definitions
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_OBJ_HM_H_
#define _XM_OBJ_HM_H_

#ifdef _XM_KERNEL_
#include <arch/irqs.h>
#else
#include <xm_inc/arch/irqs.h>
#endif

struct hmCpuCtxt {
    xm_u32_t pc;
    xm_u32_t nPc;
    xm_u32_t psr;
};

/* <track id="xm-hm-log-msg"> */
struct xmHmLog {
#define XM_HMLOG_SIGNATURE 0xfecf
    //    xm_u16_t signature;
//    xm_u16_t checksum;
    xm_u32_t opCode;
#define HMLOG_OPCODE_EVENT_MASK (0x1fff<<HMLOG_OPCODE_EVENT_BIT)
#define HMLOG_OPCODE_EVENT_BIT 19
// Bits 18 and 17 free
#define HMLOG_OPCODE_SYS_MASK (0x1<<HMLOG_OPCODE_SYS_BIT)
#define HMLOG_OPCODE_SYS_BIT 16
#define HMLOG_OPCODE_VALID_CPUCTXT_MASK (0x1<<HMLOG_OPCODE_VALID_CPUCTXT_BIT)
#define HMLOG_OPCODE_VALID_CPUCTXT_BIT 15
#define HMLOG_OPCODE_MODID_MASK (0x7f<<HMLOG_OPCODE_MODID_BIT)
#define HMLOG_OPCODE_MODID_BIT 8
#define HMLOG_OPCODE_PARTID_MASK (0xff<<HMLOG_OPCODE_PARTID_BIT)
#define HMLOG_OPCODE_PARTID_BIT 0
//    xm_u32_t seq;
    xmTime_t timestamp;
    union {
#define XM_HMLOG_PAYLOAD_LENGTH 4
        struct hmCpuCtxt cpuCtxt;
        xmWord_t payload[XM_HMLOG_PAYLOAD_LENGTH];
    };
}  __PACKED; 

typedef struct xmHmLog xmHmLog_t;
/* </track id="xm-hm-log-msg"> */

#define XM_HM_GET_STATUS 0x0

/* <track id="xm-hm-log-status"> */
typedef struct {
    xm_s32_t noEvents;
} xmHmStatus_t;
/* </track id="xm-hm-log-status"> */

union hmCmd {
    xmHmStatus_t status;
};

/*Value of the reset status in PCT when the partition is reset via HM*/
#define XM_RESET_STATUS_PARTITION_NORMAL_START    0
#define XM_RESET_STATUS_PARTITION_RESTART    1
#define XM_HM_RESET_STATUS_MODULE_RESTART    2
#define XM_HM_RESET_STATUS_PARTITION_RESTART 3
#define XM_HM_RESET_STATUS_USER_CODE_BIT 16
#define XM_HM_RESET_STATUS_EVENT_MASK 0xffff


#ifdef _XM_KERNEL_
#include <arch/asm.h>
#include <stdc.h>

extern xm_s32_t HmRaiseEvent(xmHmLog_t *log);
static inline void RaiseHmPartEvent(struct xmcPartition *cfg, xm_u32_t system, xm_u32_t eventId) {
    xmHmLog_t hmLog;
    memset(&hmLog, 0, sizeof(xmHmLog_t));
    hmLog.opCode=
        (eventId<<HMLOG_OPCODE_EVENT_BIT)|
        (cfg->rId<<HMLOG_OPCODE_PARTID_BIT)|
        ((system)?HMLOG_OPCODE_SYS_MASK:0);
        //HMLOG_OPCODE_VALID_CPUCTXT_MASK;
    // modID missed
    //SaveCpuHmCtxt(&hmLog.cpuCtxt);
    HmRaiseEvent(&hmLog);
}

#endif
#endif
