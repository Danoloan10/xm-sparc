/*
 * $FILE: mem.c
 *
 * System physical memory
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
  - [02/09/2015:SPR-020915-01] MISRA rules.9.1 violation fixed.
  - [02/09/2015:SPR-020915-04] Remove VBOOT macro and boot.h
  - [18/02/2016:SPR-180216-02] Copy Area use AlignedMemCopy
*/
#include <assert.h>
#include <hypercalls.h>
#include <objdir.h>
#include <physmm.h>
#include <sched.h>
#include <stdc.h>
#include <arch/leon.h>

#include <objects/mem.h>

static inline xm_s32_t CopyArea(xmAddress_t dstAddr, xmId_t dstId, xmAddress_t srcAddr, xmId_t srcId, xmSSize_t size) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xm_u32_t flags = 0U;

    xmId_t dstPId = PartitionId2PId(dstId);
    xmId_t srcPId = PartitionId2PId(srcId);

    if (size<=0) return 0;

    if (dstId!=GetPartitionCfg(info->sched.cKThread)->rId)
        if (!(GetPartitionCfg(info->sched.cKThread)->flags&XM_PART_SYSTEM))
            return XM_PERM_ERROR;
    
    if (srcId!=GetPartitionCfg(info->sched.cKThread)->rId)
        if (!(GetPartitionCfg(info->sched.cKThread)->flags&XM_PART_SYSTEM))
            return XM_PERM_ERROR;

    if (dstPId!=UNCHECKED_ID) {
        if ((dstPId<0)||(dstPId>=xmcTab.noPartitions))
            return XM_INVALID_PARAM;
        
        if (!PmmFindArea(dstAddr, size, (dstPId!=UNCHECKED_ID)?&partitionTab[dstPId]:0, &flags))
            return XM_INVALID_PARAM;

        if (flags&XM_MEM_AREA_READONLY)
            return XM_INVALID_PARAM;
    }

    if (srcPId!=UNCHECKED_ID) {
        if ((srcPId<0)||(srcPId>=xmcTab.noPartitions))
            return XM_INVALID_PARAM;
        
        if (!PmmFindArea(srcAddr, size, (srcPId!=UNCHECKED_ID)?&partitionTab[srcPId]:0, &flags))
            return XM_INVALID_PARAM;
    }

#if defined(CONFIG_MPU)
    memcpy((void *)dstAddr, (void*)srcAddr, size);
#else
    if ((srcAddr & (sizeof(xmWord_t) - 1)) != 0)
    {
        return XM_INVALID_PARAM;
    }
    if ((dstAddr & (sizeof(xmWord_t) - 1)) != 0)
    {
        return XM_INVALID_PARAM;
    }
    AlignedMemCpy((xm_u8_t *)dstAddr, (xm_u8_t *)srcAddr, size, (RdMem_t)ReadByPassMmuWord, (RdMem_t)ReadByPassMmuWord, (WrMem_t)WriteByPassMmuWord);
#endif
    
    return size;
}

static xm_s32_t CtrlMem(xmObjDesc_t desc, xm_u32_t cmd, union memCmd *__gParam args) {
    if (!args)
	return XM_INVALID_PARAM;

    if (CheckGParam(args, sizeof(union memCmd), 4, PFLAG_NOT_NULL)<0) 
        return XM_INVALID_PARAM;

    switch(cmd) {
    case XM_OBJ_MEM_CPY_AREA:
        return CopyArea(args->cpyArea.dstAddr, args->cpyArea.dstId, args->cpyArea.srcAddr, args->cpyArea.srcId, args->cpyArea.size);
        break;
    }

    return XM_INVALID_PARAM;
}

static const struct object memObj={
    .Ctrl=(ctrlObjOp_t)CtrlMem,
};

xm_s32_t SetupMem(void) {    
    objectTab[OBJ_CLASS_MEM]=&memObj;
    return 0;
}

REGISTER_OBJ(SetupMem);

