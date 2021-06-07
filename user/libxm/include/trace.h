/*
 * $FILE: trace.h
 *
 * tracing subsystem
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _LIB_XM_TRACE_H_
#define _LIB_XM_TRACE_H_

#include <xm_inc/config.h>
#include <xm_inc/objdir.h>
#include <xm_inc/objects/trace.h>

extern __stdcall xm_s32_t XM_trace_event(xm_u8_t *trace);
extern __stdcall xm_s32_t XM_trace_read(xmId_t id, xmTraceEvent_t *traceEventPtr, xm_s32_t noTraces);
extern __stdcall xm_s32_t XM_trace_status(xmId_t id, xmTraceStatus_t *traceStatusPtr);

#endif
