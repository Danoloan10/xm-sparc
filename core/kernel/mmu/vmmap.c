/*
 * $FILE: vmmap.c
 *
 * Virtual memory map
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
  - [02/09/2015:SPR-020915-02] 'continue' replaced with equivalent code
  [05/07/2016] Angel Esquinas
  - SPR-160609-01 The PCT is mapped only one time.
*/

#include <assert.h>
#include <kthread.h>
#include <physmm.h>
#include <rsvmem.h>
#include <sched.h>
#include <vmmap.h>
#include <arch/paging.h>
#include <arch/physmm.h>

static inline xmAddress_t VAddr2PAddr(struct xmcMemoryArea *mAreas, xm_s32_t noAreas, xmAddress_t vAddr) {
    xm_s32_t e;
    for (e=0; e<noAreas; e++)
        if ((mAreas[e].mappedAt<=vAddr)&&(((mAreas[e].mappedAt+mAreas[e].size)-1)>=vAddr))
            return vAddr-mAreas[e].mappedAt+mAreas[e].startAddr;
    return -1;
}

xmWord_t SetupPageTable(struct xmcPartition *cfg, xmAddress_t pct) {
    xmAddress_t size, vAddr, addr;
    xmWord_t attr, *ptdL1;
    xm_s32_t e;
    GET_MEMAZ(ptdL1, PTDL1SIZE, PTDL1SIZE);
    SetupPtdL1(ptdL1, cfg);
    for (e=0; e<cfg->noPhysicalMemoryAreas; e++) {
        if (!(xmcPhysMemAreaTab[e+cfg->physicalMemoryAreasOffset].flags&XM_MEM_AREA_UNMAPPED)) {
            addr=xmcPhysMemAreaTab[e+cfg->physicalMemoryAreasOffset].startAddr;
            size=xmcPhysMemAreaTab[e+cfg->physicalMemoryAreasOffset].size;
            vAddr=xmcPhysMemAreaTab[e+cfg->physicalMemoryAreasOffset].mappedAt;
            attr=_PG_ATTR_PRESENT|_PG_ATTR_USER;
            
            if (!(xmcPhysMemAreaTab[e+cfg->physicalMemoryAreasOffset].flags&XM_MEM_AREA_UNCACHEABLE))
                attr|=_PG_ATTR_CACHED;
        
            if (!(xmcPhysMemAreaTab[e+cfg->physicalMemoryAreasOffset].flags&XM_MEM_AREA_READONLY))
                attr|=_PG_ATTR_RW;
            if (VmMapUser(ptdL1, addr, vAddr, size, attr)<0)
                return 0;
        }
    }
    attr=_PG_ATTR_PRESENT|_PG_ATTR_USER;
    if (VmMapUser(ptdL1, _VIRT2PHYS(pct), CONFIG_XM_OFFSET+16*1024*1024, PAGE_SIZE, attr)<0)
    {
    	return 0;
    }
    return _VIRT2PHYS(ptdL1);
}
