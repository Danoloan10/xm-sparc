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
  Changelog
  - [02/09/2015:SPR-020915-04] Remove boot.h
*/
#include <assert.h>
#include <list.h>
#include <rsvmem.h>
#include <physmm.h>
#include <processor.h>
#include <sched.h>
#include <stdc.h>
#include <virtmm.h>
#include <vmmap.h>
#include <arch/paging.h>
#include <arch/physmm.h>
#include <arch/xm_def.h>

xm_s32_t PmmFindArea(xmAddress_t pAddr, xmSSize_t size, partition_t *p, xm_u32_t *flags) {
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
