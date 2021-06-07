/*
 * $FILE: trace.h
 *
 * Tracing object definitions
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_OBJ_TRACE_H_
#define _XM_OBJ_TRACE_H_

#ifdef _XM_KERNEL_
#include <linkage.h>
#else
#include <xm_inc/linkage.h>
#endif

#define XM_TRACE_PAYLOAD_LENGTH 8

/* <track id="xm-trace-event"> */

struct xmTraceEvent {
    xm_u32_t timestamp; // LSB of time
    xm_u8_t  payload[XM_TRACE_PAYLOAD_LENGTH];
} __PACKED;

typedef struct xmTraceEvent xmTraceEvent_t;

/* </track id="xm-trace-event"> */

#define XM_TRACE_GET_STATUS 0x0

/* <track id="xm-trace-status"> */
typedef struct {
    xm_s32_t noEvents;
} xmTraceStatus_t;
/* </track id="xm-trace-status"> */

union traceCmd {
    xmTraceStatus_t status;
};

#endif
