/*
 * $FILE: status.c
 *
 * Status functionality
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <xm.h>
#include <status.h>
#include <xm_inc/objects/status.h>

xm_s32_t XM_get_system_status(xmSystemStatus_t *status) {
    return XM_ctrl_object(OBJDESC_BUILD(OBJ_CLASS_STATUS, XM_HYPERVISOR_ID, 0), XM_GET_SYSTEM_STATUS, status);
}

xm_s32_t XM_get_partition_status(xmId_t id, xmPartitionStatus_t *status) {
    if (id&~OBJDESC_PARTITIONID_MASK)
       return XM_INVALID_PARAM;
    return XM_ctrl_object(OBJDESC_BUILD(OBJ_CLASS_STATUS, id, 0), XM_GET_SYSTEM_STATUS, status);
}

xm_s32_t XM_get_plan_status(xmPlanStatus_t *status) {
    return XM_ctrl_object(OBJDESC_BUILD(OBJ_CLASS_STATUS, XM_PARTITION_SELF, 0), XM_GET_SCHED_PLAN_STATUS, status);
}
