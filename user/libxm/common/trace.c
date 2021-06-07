/*
 * $FILE: trace.c
 *
 * Tracing functionality
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
  - [18/11/2015:SPR-290915-01] '/' replaced by Div32
*/
#include <xm.h>
#include <xm_inc/objdir.h>
#include <xm_inc/objects/trace.h>
extern void *memcpy(void *dest, const void *src, unsigned long n);

static inline xm_s32_t __trace_open(xmId_t id) {
    return (id>OBJDESC_ID_MASK)?XM_INVALID_PARAM:OBJDESC_BUILD(OBJ_CLASS_TRACE, id, 0);
}

static xm_u32_t Div32(xm_u32_t numer, xm_u32_t denom) {
    xm_u32_t quotient = 0, quotbit = 1;

    if (denom == 0) {
        return 0;
    }

    while (((xm_s32_t) denom >= 0) && (denom < numer)) {
        denom <<= 1;
        quotbit <<= 1;
    }

    while (quotbit && (numer != 0)) {
        if (denom <= numer) {
            numer -= denom;
            quotient += quotbit;
        }
        denom >>= 1;
        quotbit >>= 1;
    }
    
    return quotient;
}

xm_s32_t XM_trace_event(xm_u8_t *trace) {
    xm_s32_t ret;
    xm_s32_t size = sizeof(xmTraceEvent_t);
    if (!trace)
	return XM_INVALID_PARAM;
    ret = XM_write_object(OBJDESC_BUILD(OBJ_CLASS_TRACE, XM_PARTITION_SELF, 0), trace, sizeof(xmTraceEvent_t), 0);
    return (ret<size)?XM_INVALID_PARAM:XM_OK;
}

xm_s32_t XM_trace_read(xmId_t id, xmTraceEvent_t *traceEventPtr, xm_s32_t noTraces) {
    xm_u32_t traceStream=__trace_open(id);
    if (traceStream==XM_INVALID_PARAM)
        return XM_INVALID_PARAM;
    xm_s32_t ret;
    if (OBJDESC_GET_CLASS(traceStream)!=OBJ_CLASS_TRACE)
        return XM_INVALID_PARAM;
    if (!traceEventPtr)
	return XM_INVALID_PARAM;
    
    ret=XM_read_object(traceStream, traceEventPtr, noTraces*sizeof(xmTraceEvent_t), 0);
    return (ret>0)?(Div32(ret, sizeof(xmTraceEvent_t))):ret;
}

xm_s32_t XM_trace_status(xmId_t id, xmTraceStatus_t *traceStatusPtr) {
    xm_u32_t traceStream=__trace_open(id);
    if (traceStream==XM_INVALID_PARAM)
        return XM_INVALID_PARAM;
    if (OBJDESC_GET_CLASS(traceStream)!=OBJ_CLASS_TRACE)
        return XM_INVALID_PARAM;

    if (!traceStatusPtr)
	return XM_INVALID_PARAM;
    return XM_ctrl_object(traceStream, XM_TRACE_GET_STATUS, traceStatusPtr);
}
