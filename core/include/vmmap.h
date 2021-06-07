/*
 * $FILE: vmmap.h
 *
 * Virtual memory map manager
 *
 * Version: XtratuM-2.1.0
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */
/*
  Changelog:
  - [02/09/2015:SPR-020915-05] Remove WARN_UNUSED_RESULT macro
*/

#ifndef _XM_VMMAP_H_
#define _XM_VMMAP_H_

#ifndef _XM_KERNEL_
#error Kernel file, do not include.
#endif

#define _PG_ATTR_PRESENT (1<<16)
#define _PG_ATTR_USER (1<<17)
#define _PG_ATTR_RW (1<<18)
#define _PG_ATTR_CACHED (1<<19)
//#define _PG_ATTR_PARTITION (1<<20)

#ifdef CONFIG_MMU
extern void SetupVmMap(xmAddress_t *stFrameArea, xm_s32_t *noFrames);
#elif CONFIG_MPU
extern void SetupVmMap(void);
#endif
extern xmWord_t SetupPageTable(struct xmcPartition *cfg, xmAddress_t pct);
extern void VmMapPage(xmAddress_t pAddr, xmAddress_t vAddr, xmWord_t flags);
extern void VmUnmapPage(xmAddress_t vAddr);
extern void SetupPtdL1(xmWord_t *ptdL1, struct xmcPartition *cfg);
extern xm_s32_t VmMapUser(xmWord_t *ptdL1, xmAddress_t pAddr, xmAddress_t vAddr, xmSize_t size, xm_u32_t flags);
extern xm_u32_t VmAttr2ArchAttr(xm_u32_t entry);
extern xmAddress_t VmMapXm(xmAddress_t pAddr, xmAddress_t vAddr, xmSize_t size, xm_u32_t flags);

#define ROUNDUP(addr, _pS) ((((~(addr))+1)&((_pS)-1))+(addr))
#define ROUNDDOWN(addr, _pS) ((addr)&~((_pS)-1))
#define SIZE2PAGES(size) \
    (((((~(size))+1)&(PAGE_SIZE-1))+(size))>>PAGE_SHIFT)

#endif
