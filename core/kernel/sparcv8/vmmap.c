/*
 * $FILE: vmmap.c
 *
 * Virtual memory map management
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 *
 * Changelog:
 * - [02/09/2015:SPR-020915-02] 'continue' replaced with equivalent code
 * - [02/09/2015:SPR-020915-04] Remove boot.h
 *
 * [05/07/2016] Angel Esquinas
 * - SPR-160609-01 User Page Table memory mapping algorithm improved
 */
#include <assert.h>
#include <rsvmem.h>
#include <kthread.h>
#include <stdc.h>
#include <vmmap.h>
#include <virtmm.h>
#include <physmm.h>
#include <arch/xm_def.h>
#include <arch/processor.h>
#include <arch/paging.h>
#include <arch/physmm.h>

static inline xmAddress_t MapAddr(xmWord_t *ptdL1, xmAddress_t vAddr, xmAddress_t pAddr, xmSize_t size, xmWord_t attr) {
    xmAddress_t addr;
    xmWord_t *ptdL2, *ptdL3;
    xm_s32_t l1e, l2e, l3e;

    for (addr=vAddr; (addr>=vAddr)&&addr<(vAddr+size); ){
        l1e=VA2PtdL1(addr);
        l2e=VA2PtdL2(addr);
        l3e=VA2PtdL3(addr);
        if (!(ptdL1[l1e]&0x3)) {
            if (!l2e&&!l3e&&((addr+(16*1024*1024))<=(vAddr+size))&&!(pAddr&((16*1024*1024)-1))) {
                ptdL1[l1e]=((pAddr&~((16*1024*1024)-1))>>4)|attr;
                pAddr+=16*1024*1024;
                addr+=16*1024*1024;
            } else {
                GET_MEMAZ(ptdL2, PTDL2SIZE, PTDL2SIZE);
                ptdL1[l1e]=(_VIRT2PHYS(ptdL2))>>4|_PG_ARCH_PTD_PRESENT;
            }
        } else {
            ptdL2=(xmWord_t *)_PHYS2VIRT((ptdL1[l1e]&~0x3)<<4);
            
            if (!(ptdL2[l2e]&0x3)) {
                if (!l3e&&((addr+(256*1024))<=(vAddr+size))&&!(pAddr&((256*1024)-1))) {
                    ptdL2[l2e]=((pAddr&~((256*1024)-1))>>4)|attr;
                    pAddr+=256*1024;
                    addr+=256*1024;
                } else {
                    GET_MEMAZ(ptdL3, PTDL3SIZE, PTDL3SIZE);
                    ptdL2[l2e]=(_VIRT2PHYS(ptdL3))>>4|_PG_ARCH_PTD_PRESENT;
                }
            } else {
                ptdL3=(xmWord_t *)_PHYS2VIRT((ptdL2[l2e]&~0x3)<<4);
                ptdL3[l3e]=((pAddr&PAGE_MASK)>>4)|attr;
                pAddr+=PAGE_SIZE;
                addr+=PAGE_SIZE;
            }
        }
    }
    return addr;
}

#ifdef CONFIG_AMP_SUPPORT
xmAddress_t VmMapXm(xmAddress_t pAddr, xmAddress_t vAddr, xmSize_t size, xm_u32_t flags)  {
    return MapAddr(_ptdL1, vAddr, pAddr, size, VmAttr2ArchAttr(flags));
}
#endif

void SetupVmMap(xmAddress_t *stFrameArea, xm_s32_t *noFrames) {
    xmAddress_t end, vAddr, pAddr;
    xmWord_t attr;
    xm_s32_t flags;
    
    attr=_PG_ARCH_PTE_PRESENT|_PG_ARCH_RW;
    pAddr=xmcPhysMemAreaTab[xmcTab.hpv.physicalMemoryAreasOffset].startAddr;
    end=pAddr+xmcPhysMemAreaTab[xmcTab.hpv.physicalMemoryAreasOffset].size-1;
    flags=xmcPhysMemAreaTab[xmcTab.hpv.physicalMemoryAreasOffset].flags;
    if (!(flags&XM_MEM_AREA_UNCACHEABLE))
        attr|=_PG_ARCH_CACHE;
    *stFrameArea=ROUNDUP(_PHYS2VIRT(end+1), LPAGE_SIZE);
    *noFrames=((XM_VMAPEND-*stFrameArea)+1)/PAGE_SIZE;
    vAddr=xmcPhysMemAreaTab[xmcTab.hpv.physicalMemoryAreasOffset].mappedAt;
    _ptdL1[VA2PtdL1(vAddr)]=0;
    vAddr=MapAddr(_ptdL1, vAddr, xmcPhysMemAreaTab[xmcTab.hpv.physicalMemoryAreasOffset].startAddr, xmcPhysMemAreaTab[xmcTab.hpv.physicalMemoryAreasOffset].size, attr);
    FlushTlbGlobal();
}

void SetupPtdL1(xmWord_t *ptdL1, struct xmcPartition *cfg) {
    xm_s32_t l1e;
    l1e=VA2PtdL1(CONFIG_XM_OFFSET);    
    ptdL1[l1e]=_pgTables[l1e];
    contextTab[cfg->id+1]=(_VIRT2PHYS(ptdL1)>>4)|_PG_ARCH_PTD_PRESENT;
}

xm_u32_t VmAttr2ArchAttr(xm_u32_t flags) {
    xm_u32_t attr=0;

    if (flags&_PG_ATTR_PRESENT) attr|=_PG_ARCH_PTE_PRESENT;
    if (flags&_PG_ATTR_USER) {        
        if (flags&_PG_ATTR_RW) 
            attr|=_PG_ARCH_RW_USER;
        else
            attr|=_PG_ARCH_R_USER;
    } else {
        if (flags&_PG_ATTR_RW) 
            attr|=_PG_ARCH_RW;
        else
            attr|=_PG_ARCH_R;
    }
    
    if (flags&_PG_ATTR_CACHED) attr|=_PG_ARCH_CACHE;
    return attr|(flags&0xffff);
}

#define ET_INVALID (0x0)
#define ET_PT_DESCRIPTOR 0x1
#define ET_PT_ENTRY 0x2
#define ET_RESERVED 0x3

#define ROOT_MAPPING_SIZE	(0xFFFFFFFF) /* 0 - 2^32-1 Bytes addressables */
#define LEVEL1_MAPPING_SIZE (1 << 24) /*  16MB */
#define LEVEL2_MAPPING_SIZE (1 << 18) /* 256KB */
#define LEVEL3_MAPPING_SIZE (1 << 12) /*   4KB */

#define ET_MASK 0x3

xm_s32_t VmMapUser(
    xmWord_t *ptdL1, xmAddress_t pAddr, xmAddress_t vAddr, xmSize_t size,
    xm_u32_t flags)
{
    xmWord_t *ptdL2, *ptdL3;
    xm_s32_t l1e, l2e, l3e;
    xmAddress_t addr;

    /* Check that memory area is aligned and multiple with the size */

    for (addr = vAddr; (addr >= vAddr) && addr <= (vAddr + size - 1);)
    {
        l1e = VA2PtdL1(addr);
        l2e = VA2PtdL2(addr);
        l3e = VA2PtdL3(addr);

        xmAddress_t endPAddr;
        xmAddress_t endVAddr;

        /* If no ptdl1 entrie */
        if ((ptdL1[l1e] & ET_MASK) == ET_INVALID)
        {
            /* Check that virtual address and physical address is aligned to 16MB
             * The size <= 16MB (Mapping a 16MB page). This is an error
             */
            endPAddr = pAddr + LEVEL1_MAPPING_SIZE - 1;
            endVAddr = addr + LEVEL1_MAPPING_SIZE - 1;

            if ((l2e == 0) && (l3e == 0)
                && (endVAddr <= (vAddr + size - 1))
                && !(pAddr & ((16 * 1024 * 1024) - 1)) /* Check physical address is aligned */)
            {
                /* Map 16MB or less or virtual address to 16MB of physical address
                 * TODO: Error. The area is mapping more area
                 */
                ptdL1[l1e] = ((pAddr & ~((16 * 1024 * 1024) - 1)) >> 4)
                    | VmAttr2ArchAttr(flags);

                /* Increment counters */
                pAddr += LEVEL1_MAPPING_SIZE;
                addr += LEVEL1_MAPPING_SIZE;
            }
            else
            {
                /*
                 * Create a new Page table of level 2
                 */
                GET_MEMAZ(ptdL2, PTDL2SIZE, PTDL2SIZE);
                ptdL1[l1e] = (_VIRT2PHYS(ptdL2)) >> 4 | _PG_ARCH_PTD_PRESENT;
            }
            /* PTDL1 exists */
        }
        else
        {
            ptdL2 = (xmWord_t *) _PHYS2VIRT((ptdL1[l1e] & ~0x3) << 4);

            if (!(ptdL2[l2e] & 0x3))
            {
                endPAddr = pAddr + LEVEL2_MAPPING_SIZE - 1;
                endVAddr = addr + LEVEL2_MAPPING_SIZE - 1;

                if (!l3e /* Virtual address aligned to 256KB */
                    && (endVAddr <= (vAddr + size - 1))
                    && !(pAddr & ((256 * 1024) - 1)))
                {
                    if ((ptdL2[l2e] & ET_MASK) != ET_INVALID)
                    {
                        cpuCtxt_t ctxt;
                        GetCpuCtxt(&ctxt);
                        SystemPanic(&ctxt, __XM_FILE__
                            ":%u: Virtual address already mapped: 0x%x",
                            __LINE__, vAddr);
                    }


                    ptdL2[l2e] = ((pAddr & ~((256 * 1024) - 1)) >> 4)
                        | VmAttr2ArchAttr(flags);
                    pAddr += 256 * 1024;
                    addr += 256 * 1024;
                }
                else
                {
                    GET_MEMAZ(ptdL3, PTDL3SIZE, PTDL3SIZE);
                    ptdL2[l2e] =
                        (_VIRT2PHYS(ptdL3)) >> 4 | _PG_ARCH_PTD_PRESENT;
                }
            }
            else
            {
                /* Map physical addres in page table of level 3
                 * 4KB of memory
                 */
                ptdL3 = (xmWord_t *) _PHYS2VIRT((ptdL2[l2e] & (~0x3)) << 4);

                /* Check if previous area was mapped in this page
                 * New areas must be mapped only in unmapped areas
                 */
                if ((ptdL3[l3e] & ET_MASK) != ET_INVALID)
                {
                    cpuCtxt_t ctxt;
                    GetCpuCtxt(&ctxt);
                    SystemPanic(&ctxt, __XM_FILE__
                        ":%u: Virtual address already mapped: 0x%x",
                       __LINE__, vAddr);
                }

                ptdL3[l3e] = ((pAddr & PAGE_MASK) >> 4)
                    | VmAttr2ArchAttr(flags);

                /* Increment counters */
                pAddr += PAGE_SIZE;
                addr += PAGE_SIZE;
            }
        }
    }

    return 0;
}

