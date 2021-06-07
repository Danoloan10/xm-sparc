/*
 * $FILE: physmm.h
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

#ifndef _XM_PHYSMM_H_
#define _XM_PHYSMM_H_

#ifndef _XM_KERNEL_
#error Kernel file, do not include.
#endif

#include <kthread.h>
#include <processor.h>
#include <hypercalls.h>
#include <arch/paging.h>

struct physPage {
};

extern void SetupPhysMM(void);
extern struct physPage *PmmFindPage(xmAddress_t pAddr, partition_t *p, xm_u32_t *flags);

extern struct physPage *PmmFindAnonymousPage(xmAddress_t pAddr);
extern xm_s32_t PmmFindAddr(xmAddress_t pAddr, partition_t *p, xm_u32_t *flags);
extern xm_s32_t PmmFindArea(xmAddress_t pAddr, xmSSize_t size, partition_t *p, xm_u32_t *flags);
extern void PmmCheckPhysMemAreaTab(partition_t *p);
extern void *VCacheMapPage(xmAddress_t pAddr, struct physPage *page);
extern void VCacheUnlockPage(struct physPage *page);

#endif
