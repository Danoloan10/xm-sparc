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

#ifndef _LIB_XM_STATUS_H_
#define _LIB_XM_STATUS_H_

#include <xm_inc/config.h>
#include <xm_inc/objdir.h>
#include <xm_inc/objects/status.h>

extern __stdcall xm_s32_t XM_get_partition_status(xmId_t id, xmPartitionStatus_t *status);
extern __stdcall xm_s32_t XM_get_system_status (xmSystemStatus_t  *status);
extern __stdcall xm_s32_t XM_get_plan_status(xmPlanStatus_t *status);

#endif
