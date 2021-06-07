/*
 * $FILE: comm.c
 *
 * Communication wrappers
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
#include <comm.h>
#include <hypervisor.h>
#include <xmhypercalls.h>

#include <xm_inc/hypercalls.h>

xm_s32_t XM_create_sampling_port(char *portName, xm_u32_t maxMsgSize, xm_u32_t direction, xmTime_t refreshPeriod) {
    xmObjDesc_t desc=OBJDESC_BUILD(OBJ_CLASS_SAMPLING_PORT, XM_PARTITION_SELF, 0);
    union samplingPortCmd cmd;
    xm_s32_t id;

    cmd.create.portName=portName;
    cmd.create.maxMsgSize=maxMsgSize;
    cmd.create.direction=direction;
    cmd.create.refreshPeriod=refreshPeriod;
   
    if ((id=XM_ctrl_object(desc, XM_COMM_CREATE_PORT, &cmd))<0)
        return id;
    desc=OBJDESC_SET_ID(desc, id);
    return desc;
}

xm_s32_t XM_read_sampling_message(xm_s32_t portDesc, void *msgPtr, xm_u32_t msgSize, xm_u32_t *flags) {
    if ((OBJDESC_GET_CLASS(portDesc)!=OBJ_CLASS_SAMPLING_PORT) || (OBJDESC_GET_PARTITIONID(portDesc)!=XM_PARTITION_SELF))
	return XM_INVALID_PARAM;
    return XM_read_object(portDesc, msgPtr, msgSize, flags);
}

xm_s32_t XM_write_sampling_message(xm_s32_t portDesc, void *msgPtr, xm_u32_t msgSize) {
    if ((OBJDESC_GET_CLASS(portDesc)!=OBJ_CLASS_SAMPLING_PORT) || (OBJDESC_GET_PARTITIONID(portDesc)!=XM_PARTITION_SELF))
	return XM_INVALID_PARAM;
    return XM_write_object(portDesc, msgPtr, msgSize, 0);
}

xm_s32_t XM_create_queuing_port(char *portName, xm_u32_t maxNoMsgs, xm_u32_t maxMsgSize, xm_u32_t direction) {
    xmObjDesc_t desc=OBJDESC_BUILD(OBJ_CLASS_QUEUING_PORT, XM_PARTITION_SELF, 0);
    union queuingPortCmd cmd;
    xm_s32_t id;

    cmd.create.portName=portName;
    cmd.create.maxNoMsgs=maxNoMsgs;
    cmd.create.maxMsgSize=maxMsgSize;
    cmd.create.direction=direction;

    if ((id=XM_ctrl_object(desc, XM_COMM_CREATE_PORT, &cmd))<0)
        return id;
    desc=OBJDESC_SET_ID(desc, id);

    return desc;
}

xm_s32_t XM_send_queuing_message(xm_s32_t portDesc, void *msgPtr, xm_u32_t msgSize) {
    if ((OBJDESC_GET_CLASS(portDesc)!=OBJ_CLASS_QUEUING_PORT) || (OBJDESC_GET_PARTITIONID(portDesc)!=XM_PARTITION_SELF))
      return XM_INVALID_PARAM;
    return XM_write_object(portDesc, msgPtr, msgSize, 0);
}

xm_s32_t XM_receive_queuing_message(xm_s32_t portDesc, void *msgPtr, xm_u32_t msgSize) {
    if ((OBJDESC_GET_CLASS(portDesc)!=OBJ_CLASS_QUEUING_PORT) || (OBJDESC_GET_PARTITIONID(portDesc)!=XM_PARTITION_SELF))
	return XM_INVALID_PARAM;
    return XM_read_object(portDesc, msgPtr, msgSize, 0);
}

xm_s32_t XM_get_queuing_port_info(char *portName, xmQueuingPortInfo_t *info) {
    int i;
    xmObjDesc_t desc=OBJDESC_BUILD(OBJ_CLASS_QUEUING_PORT, XM_PARTITION_SELF, 0);

    for (i=0; i<libXmParams.partCtrlTab->noPhysicalMemAreas; i++) {
        if ((xmAddress_t )info >= libXmParams.partMemMap[i].mappedAt) {
            if (((xmAddress_t) info + sizeof(xmQueuingPortInfo_t)) <= (libXmParams.partMemMap[i].mappedAt + libXmParams.partMemMap[i].size)) {
                if (!(libXmParams.partMemMap[i].flags&XM_MEM_AREA_READONLY)) {
                    break;
                }
            }
        }
    }
    if (i>=libXmParams.partCtrlTab->noPhysicalMemAreas) {
        return XM_INVALID_PARAM;
    }

    info->portName=portName;
    return XM_ctrl_object(desc, XM_COMM_GET_PORT_INFO, (union queuingPortCmd *)info);
}

xm_s32_t XM_get_queuing_port_status(xm_u32_t portDesc, xmQueuingPortStatus_t *status) {
    if ((OBJDESC_GET_CLASS(portDesc)!=OBJ_CLASS_QUEUING_PORT) || (OBJDESC_GET_PARTITIONID(portDesc)!=XM_PARTITION_SELF))
	return XM_INVALID_PARAM;
    if (!status)
	return XM_INVALID_PARAM;
    
    return XM_ctrl_object(portDesc, XM_COMM_GET_PORT_STATUS, status);
}

xm_s32_t XM_get_sampling_port_info(char *portName, xmSamplingPortInfo_t *info) {
    int i;
    xmObjDesc_t desc=OBJDESC_BUILD(OBJ_CLASS_SAMPLING_PORT, XM_PARTITION_SELF, 0);

    for (i=0; i<libXmParams.partCtrlTab->noPhysicalMemAreas; i++) {
        if ((xmAddress_t )info >= libXmParams.partMemMap[i].mappedAt) {
            if (((xmAddress_t) info + sizeof(xmSamplingPortInfo_t)) <= (libXmParams.partMemMap[i].mappedAt + libXmParams.partMemMap[i].size)) {
                if (!(libXmParams.partMemMap[i].flags&XM_MEM_AREA_READONLY)) {
                    break;
                }
            }
        }
    }

    if (i>=libXmParams.partCtrlTab->noPhysicalMemAreas) {
        return XM_INVALID_PARAM;
    }

    info->portName=portName;
    return XM_ctrl_object(desc, XM_COMM_GET_PORT_INFO, (union samplingPortCmd *)info);
}

xm_s32_t XM_get_sampling_port_status(xm_u32_t portDesc, xmSamplingPortStatus_t *status) {
    if ((OBJDESC_GET_CLASS(portDesc)!=OBJ_CLASS_SAMPLING_PORT) || (OBJDESC_GET_PARTITIONID(portDesc)!=XM_PARTITION_SELF))
	return XM_INVALID_PARAM;
    if (!status)
	return XM_INVALID_PARAM;
    
    return XM_ctrl_object(portDesc, XM_COMM_GET_PORT_STATUS, status);
}

