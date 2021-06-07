/*
 * $FILE: physmm.c
 *
 * Physical memory manager
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
  - [02/09/2015:SPR-020915-04] Remove boot.h
  - [02/09/2015:SPR-020915-07] PmmFindArea updated
*/
#include <assert.h>
#include <list.h>
#include <physmm.h>
#include <processor.h>
#include <sched.h>
#include <smp.h>
#include <stdc.h>
#include <arch/xm_def.h>

xm_s32_t PmmFindArea(xmAddress_t pAddr, xmSSize_t size, kThread_t *k, xm_u32_t *flags) {
    struct xmcMemoryArea *memArea;
    xm_s32_t l, r, c;
    xmAddress_t a, b;

    if (p) {
        for (l=0, r=p->cfg->noPhysicalMemoryAreas-1; l<=r; ) {
            c=(l+r)>>1;
            memArea=&xmcPhysMemAreaTab[c+p->cfg->physicalMemoryAreasOffset];
            a=memArea->startAddr;
            b=a+memArea->size-1;
            if (pAddr<a) {
                r=c-1;
            } else {
                if ((pAddr+size-1)>b) {
                    l=c+1;
                } else {
                    ASSERT((pAddr>=a)&&((pAddr+size-1)<=b));
                    if (flags)
                        *flags=memArea->flags;
                    return 1;
                }
            }
        }
    } else {
        for (l=0, r=xmcTab.noRegions-1; l<=r; ) {
            c=(l+r)>>1;
            a=xmcMemRegTab[c].startAddr;
            b=a+xmcMemRegTab[c].size-1;
            if (pAddr<a) {
                r=c-1;
            } else {
                if ((pAddr+size-1)>b) {
                l=c+1;
                } else {
                    ASSERT((pAddr>=a)&&((pAddr+size-1)<=b));
                    if (flags)
                        *flags=xmcMemRegTab[c].flags;
                    return 1;
                }
            }
        }
    }

    return 0;
}

xm_s32_t CheckGParam(void *param, xmSize_t size, xm_u32_t aligment, xm_s32_t flags) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    struct xmcMemoryArea *memArea;
    struct xmcPartition *cfg = NULL;
    xm_s32_t l, r, c;
    xmAddress_t a, b, pAddr;

    if (!(flags&PFLAG_NOT_NULL)&&!param)
        return 0;
    
    if (!(flags&PFLAG_RW))
        return 0;

    cfg=GetPartitionCfg(info->sched.cKThread);
    pAddr=(xmAddress_t)param;

    for (l=0, r=cfg->noPhysicalMemoryAreas-1; l<=r; ) {
        c=(l+r)>>1;
        memArea=&xmcPhysMemAreaTab[c+cfg->physicalMemoryAreasOffset];
        a=memArea->startAddr;
        b=a+memArea->size-1;
        if (pAddr<a) {
            r=c-1;
        } else {
            if (pAddr>b) {
                l=c+1;
            } else {
                ASSERT((pAddr>=a)&&((pAddr+(size-1))<=b));
                return 0;
            }
        }
    }

    return -1;

}
