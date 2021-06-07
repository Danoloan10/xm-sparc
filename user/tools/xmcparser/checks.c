/*
 * $FILE: checks.c
 *
 * checks implementation
 *
 * $VERSION$
 *
 * Authors: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

/*
  Changelog:
  - [02/09/2015:SPR-280715-01] added CheckPartitionEntryPoint.
  - [08/10/2015:SPR-081015-01] entryPoint is not required to be within 
                partition's memory areas.
  - [20/11/2015:SPR-110915-01] ipvi stuff depends on IPVI_SUPPORT
*/

#define _RSV_PHYS_PAGES_
#define _RSV_HW_IRQS_
#define REG_SIZE (32*1024)

#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <xm_inc/arch/ginfo.h>
#include <xm_inc/arch/paging.h>

#include <xm_inc/arch/asm_offsets.h>
#include "limits.h"
#include "common.h"
#include "parser.h"
#include "xmc.h"

static inline xm_s32_t DoesPartitionExist(xmId_t rId) {
    xm_s32_t e;
    for (e=0; e<xmc.noPartitions; e++)
        if (xmcPartitionTab[e].rId==rId)
            return 1;
    return 0;
}

void CheckChannelSizes(void) {
#ifdef CONFIG_AMP_SUPPORT
    int i;
    for (i=0; i<xmc.noCommChannels; i++) {
        switch(xmcCommChannelTab[i].type) {
	case XM_SAMPLING_CHANNEL:
            if (xmcCommChannelTab[i].s.size||xmcCommChannelTab[i].s.address)
                if (xmcCommChannelTab[i].s.size<(xmcCommChannelTab[i].s.maxLength+_STRUCT_CHANNELCTRLSAMPLING_SIZEOF))
                    LineError(xmcCommChannelTabNoL[i].s.size, "Sampling channel bigger than the reserved area\n");
	    break;
	case XM_QUEUING_CHANNEL:
            if (xmcCommChannelTab[i].q.size||xmcCommChannelTab[i].q.address)
                if (xmcCommChannelTab[i].q.size<((xmcCommChannelTab[i].q.maxNoMsgs*(_STRUCT_MSG_SIZEOF+xmcCommChannelTab[i].q.maxLength))+_STRUCT_CHANNELCTRLQUEUING_SIZEOF))                
                    LineError(xmcCommChannelTabNoL[i].q.size, "Queuing channel bigger than the reserved area\n");
	    break;
	}    
    }
#endif
}

void CheckMemoryRegion(int region) {
    xmAddress_t s0, e0, s1, e1;
    int e;
    s0=xmcMemRegTab[region].startAddr;
    e0=s0+xmcMemRegTab[region].size-1;
    if (s0&(PAGE_SIZE-1))
	LineError(xmcMemRegTabNoL[region].startAddr, "memory region start address (0x%x) shall be aligned to 0x%x", s0, PAGE_SIZE);
    if (xmcMemRegTab[region].size&(PAGE_SIZE-1))
	LineError(xmcMemRegTabNoL[region].line, "memory region size (%d) is not multiple of %d", xmcMemRegTab[region].size, PAGE_SIZE);
    for (e=0; e<xmc.noRegions-1; e++) {
	s1=xmcMemRegTab[e].startAddr;
	e1=s1+xmcMemRegTab[e].size-1;
	
	if ((s0>=e1)||(s1>=e0))
	    continue;
	LineError(xmcMemRegTabNoL[region].line, "memory region [0x%x - 0x%x] overlaps [0x%x - 0x%x] (line %d)", s0, e0, s1, e1, xmcMemRegTabNoL[e].line);
    }
}

#if defined(CONFIG_MPU) && defined(CONFIG_AT697F)
//Handler to check the hypervisor shared memory area alignment
void CheckSharedMemoryAreaAlignment(int area) {
    int line;
    xm_u32_t size, startAddr;

    startAddr = xmcSharedMemAreaTab[area].startAddr;
    size = xmcSharedMemAreaTab[area].size;
    line = xmcSharedMemAreaTabNoL[area].line;
    
    //Size must be greater than 32 KB (REG_SIZE)
    if (size <  (REG_SIZE))
    {
        LineError(line, "Size of shared memory area of XtratuM is equal to %d KB. It must be greater than or equal to 32 KB.", size/1024);
    }

    //Size must be a power of 2
    if (size & (size-1))
    {
        LineError(line, "Size of shared memory area of XtratuM is equal to %d KB (0x%x). It must be a power of 2.", size/1024, size);
    }

    //Start address must be a multiple of size
    if (startAddr % size)
    {
        LineError(line, "Start address of shared memory area of XtratuM is equal to 0x%x and its size is %d KB. The start address must be a multiple of size.", startAddr, size/1024);
    }
}

//Handler to check the hypervisor shared memory area overlapping with memory regions, XtratuM memory areas and Resident Software memory areas
void CheckSharedMemoryAreaOverlapping() {
    extern xm_s32_t noRsvPhysPages;
    xmAddress_t s0, e0, s1, e1; //s --> start; e --> end
    int i, e, found;
    int line;

    for(i=0; i<xmc.hpv.noSharedMemoryAreas; i++)
    {
        line = xmcSharedMemAreaTabNoL[i].line;
        s0 = xmcSharedMemAreaTab[i].startAddr;
        e0 = s0 + xmcSharedMemAreaTab[i].size - 1;

        //Check that the shared memory area is covered by some memory region
        for (e=0, found=-1; e<xmc.noRegions; e++)
        {
            s1 = xmcMemRegTab[e].startAddr;
            e1 = s1+xmcMemRegTab[e].size-1;
            if ((s0>=s1) && (e0<=e1))
            {
                found=e;
            }
        }
        if (found<0)
        {
            LineError(line, "Shared memory area of Xtratum [0x%x - 0x%x] is not covered by any memory region.", s0, e0);
        }

        //Check that the shared memory area does not overlap the memory reserved for XtratuM
        for (e=0; e<noRsvPhysPages; e++)
        {
            s1 = rsvPhysPages[e].address;
            e1 = s1+(rsvPhysPages[e].noPag*PAGE_SIZE)-1;
            if (!((e1<s0) || (s1>=e0)))
            {
                LineError(line, "Shared memory area of Xtratum [0x%x - 0x%x] overlaps a memory area [0x%x - 0x%x] reserved only for Xtratum.", s0, e0, s1, e1);
            }
        }
        for (e=0; e<xmc.hpv.noPhysicalMemoryAreas; e++)
        {
            s1 = xmcMemAreaTab[xmc.hpv.physicalMemoryAreasOffset+e].startAddr;
            e1 = s1+xmcMemAreaTab[xmc.hpv.physicalMemoryAreasOffset+e].size-1;
            if (!((e1<s0) || (s1>=e0)))
            {
                LineError(line, "Shared memory area of Xtratum [0x%x - 0x%x] overlaps a memory area [0x%x - 0x%x] reserved only for Xtratum.", s0, e0, s1, e1);
            }
        }

        //Check that the shared memory area does not overlap the memory reserved for the Resident Software
        for (e=0; e<xmc.rsw.noPhysicalMemoryAreas; e++)
        {
            s1 = xmcMemAreaTab[xmc.rsw.physicalMemoryAreasOffset+e].startAddr;
            e1 = s1+xmcMemAreaTab[xmc.rsw.physicalMemoryAreasOffset+e].size-1;
            if (!((e1<s0) || (s1>=e0)))
            {
                LineError(line, "Shared memory area of Xtratum [0x%x - 0x%x] overlaps a memory area [0x%x - 0x%x] reserved only for Resident Software.", s0, e0, s1, e1);
            }
        }
    }
}
#endif

int CheckPhysMemArea(int memArea) {
    extern xm_s32_t noRsvPhysPages;
    xmAddress_t s0, e0, s1, e1;
    int e, found;
   
    s0=xmcMemAreaTab[memArea].startAddr;
    e0=s0+xmcMemAreaTab[memArea].size-1;

    if (s0&(PAGE_SIZE-1))
	LineError(xmcMemAreaTabNoL[memArea].startAddr, "memory area start address (0x%x) shall be aligned to 0x%x", s0, PAGE_SIZE);
    if (xmcMemAreaTab[memArea].size&(PAGE_SIZE-1))
	LineError(xmcMemAreaTabNoL[memArea].line, "memory area size (%d) is not multiple of %d", xmcMemAreaTab[memArea].size, PAGE_SIZE);
    
    for (e=0; e<noRsvPhysPages; e++) {
        s1=rsvPhysPages[e].address;
        e1=s1+(rsvPhysPages[e].noPag*PAGE_SIZE)-1;
        if (!((e1<s0)||(s1>=e0)))
            LineError(xmcMemAreaTabNoL[memArea].line, "memory area [0x%x - 0x%x] overlaps a memory area [0x%x - 0x%x] reserved for XM", s0, e0, s1, e1);

    }

    for (e=0, found=-1; e<xmc.noRegions; e++) {
	s1=xmcMemRegTab[e].startAddr;
	e1=s1+xmcMemRegTab[e].size-1;
	if ((s0>=s1)&&(e0<=e1))
	    found=e;
    }
    if (found<0)
	LineError(xmcMemAreaTabNoL[memArea].line, "memory area [0x%x - 0x%x] is not covered by any memory region", s0, e0);

    for (e=0; e<xmc.noPhysicalMemoryAreas-1; e++) {
	s1=xmcMemAreaTab[e].startAddr;
	e1=s1+xmcMemAreaTab[e].size-1;
	if ((s0>=e1)||(s1>=e0))
	    continue;

        //if ((xmcMemAreaTab[e].flags&xmcMemAreaTab[memArea].flags&XM_MEM_AREA_SHARED)==XM_MEM_AREA_SHARED)
        //if (xmcMemAreaTabNoL[memArea].partitionId!=xmcMemAreaTabNoL[e].partitionId)
        //  	continue;
        LineWarning(xmcMemAreaTabNoL[memArea].line, "memory area [0x%x - 0x%x] overlaps [0x%x - 0x%x] (line %d)", s0, e0, s1, e1, xmcMemAreaTabNoL[e].line);
    }

    return found;
}

void CheckMemAreaPerPart(void) {
    xmAddress_t s0, e0, s1, e1;
#ifdef CONFIG_MMU
    xmAddress_t s2, e2;
#endif
    int i, j, offset;

    offset=xmcPartitionTab[C_PARTITION].physicalMemoryAreasOffset;
    
#ifdef CONFIG_MMU
    s2=CONFIG_XM_OFFSET;
    e2=s2-1+16*1024*1024;
    for (i=0; i<xmcPartitionTab[C_PARTITION].noPhysicalMemoryAreas; i++) {
        s0=xmcMemAreaTab[i+offset].mappedAt;
        e0=s0+xmcMemAreaTab[i+offset].size-1;
        if ((s0<e2)&&(s2<e0))
            LineError(xmcMemAreaTabNoL[i+offset].line, "virtual memory area [0x%x - 0x%x] overlaps [0x%x - 0x%x] (XtratuM virtual address) (line %d)", s0, e0, s2, e2, xmcMemAreaTabNoL[i+offset].line);
    }
#endif

#ifdef CONFIG_MPU
    //Checks to do in case CONFIG_MPU is used. Note that some of the checks are already checked in CheckPhysMemArea handler, but it is not harmful (for the performance of XM) to do it again
    xm_u32_t size, startAddr;
    int line, physicalMemoryAreaIndex;

    physicalMemoryAreaIndex = xmcPartitionTab[C_PARTITION].noPhysicalMemoryAreas - 1;

    // 1) Only one area is allowed in case CONFIG_MPU is defined
    if (xmcPartitionTab[C_PARTITION].noPhysicalMemoryAreas > 1)
    {
        LineError(xmcMemAreaTabNoL[physicalMemoryAreaIndex+offset].line, "Partition id %d defines more than one memory area. The current configuration does not allow more than one memory area.", xmcPartitionTab[C_PARTITION].id);
    }

    //NOTE: physicalMemoryAreaIndex should be equal to 0 at this point since CONFIG_MPU configuration only allows to have one memory area. XML schema forces to have at least one memory area.

    startAddr=xmcMemAreaTab[physicalMemoryAreaIndex+offset].mappedAt;
    size=xmcMemAreaTab[physicalMemoryAreaIndex+offset].size;
    line=xmcMemAreaTabNoL[physicalMemoryAreaIndex+offset].line;

    // 2) Size must be greater than 32 KB (REG_SIZE)
    if (size <  (REG_SIZE))
    {
        LineError(line, "Size memory area of partition id %d is equal to %d KB. It must be greater than or equal to 32 KB for the current configuration.", xmcPartitionTab[C_PARTITION].id, size/1024);
    }

    // 3) Size must be a power of 2
    if (size & (size-1))
    {
        LineError(line, "Size memory area of partition id %d is equal to %d KB (0x%x). It must be power of 2 for the current configuration.", xmcPartitionTab[C_PARTITION].id, size/1024, size);
    }
    
    // 4) The start address must be greater than or equal to the size or must be power of two
    if ((startAddr < size) && (startAddr & (startAddr-1)))
    {
        LineError(line, "Start address of memory area of partition id %d is equal to 0x%x and its size is %d KB. The start address must be greater than or equal to the size or must be power of two for the current configuration.", xmcPartitionTab[C_PARTITION].id, startAddr, size/1024);
    }

    // 5) Size must be multiple of 32 KB (REG_SIZE) 
    //and start address must be multiple of 32 KB (REG_SIZE)
    //and size must be less or equal to start address value
    if (size & (REG_SIZE-1))
    {
        LineError(line, "Size memory area of partition id %d is equal to %d KB (0x%x). It must be multiple of 32 KB for the current configuration.", xmcPartitionTab[C_PARTITION].id, size/1024, size);
    }
    if (startAddr & (REG_SIZE-1))
    {
        LineError(line, "Start address of memory area of partition id %d is equal to 0x%x. The start address must be multiple of 32 KB for the current configuration.", xmcPartitionTab[C_PARTITION].id, startAddr);
    }
    if (size > startAddr)
    {
        LineError(line, "Start address of memory area of partition id %d is equal to 0x%x and its size is %d KB. The start address must be greater than or equal to the size for the current configuration.", xmcPartitionTab[C_PARTITION].id, startAddr, size/1024);
    }

    // 6) Start address must be a multiple of size
    if (startAddr % size)
    {
        LineError(line, "Start address of memory area of partition id %d is equal to 0x%x and its size is %d KB. The start address must be a multiple of size for the current configuration.", xmcPartitionTab[C_PARTITION].id, startAddr, size/1024);
    }
#endif

    for (i=0; i<xmcPartitionTab[C_PARTITION].noPhysicalMemoryAreas-1; i++)
    {
        s0=xmcMemAreaTab[i+offset].mappedAt;
        e0=s0+xmcMemAreaTab[i+offset].size-1;
        for (j=i+1; j<xmcPartitionTab[C_PARTITION].noPhysicalMemoryAreas; j++)
        {
            s1=xmcMemAreaTab[j+offset].mappedAt;
            e1=s1+xmcMemAreaTab[j+offset].size-1;
            if ((s0>=e1)||(s1>=e0))
                continue;
            LineError(xmcMemAreaTabNoL[i+offset].line, "virtual memory area [0x%x - 0x%x] overlaps [0x%x - 0x%x] (line %d)", s0, e0, s1, e1, xmcMemAreaTabNoL[j+offset].line);
        }
    }
}

void CheckHwIrq(int line, int lineNo) {
    int e;
    for (e=0; e<noRsvHwIrqs; e++)
	if (line==rsvHwIrqs[e])
	    LineError(lineNo, "hw interrupt line %d reserved for XM", rsvHwIrqs[e]);
}

void CheckPortName(int port, int partition) {
    int e, offset;
    offset=xmcPartitionTab[partition].commPortsOffset;
    for (e=0; e<xmcPartitionTab[partition].noPorts-1; e++)
	if (!strcmp(&strTab[xmcCommPortTab[e+offset].nameOffset], &strTab[xmcCommPortTab[port].nameOffset]))
	    LineError(xmcCommPortTabNoL[port].name, "port name \"%s\" duplicated (line %d)", &strTab[xmcCommPortTab[port].nameOffset], xmcCommPortTabNoL[e+offset].name);
}

void CheckPartitionEntryPoint(void) {
    xmAddress_t ePoint;
    ePoint = xmcPartitionTab[C_PARTITION].entryPoint;
    if (ePoint&0x3) {
        LineError(xmcPartitionTabNoL[C_PARTITION].entryPoint,  "invalid partition entry point 0x%x", ePoint);
    }
    /* 
       The check whether entryPoint is within partition's memory areas
       is removed by SPR-081015-01 @r6243.
     */
}

/*
  void CheckMemBlockReg(void) {
  xmAddress_t s0, e0, s1, e1;
  int i, j, found;
  for (i=0;i<xmc.deviceTab.noMemBlocks; i++) {
  s0=xmcMemBlockTab[i].startAddr;
  e0=s0+xmcMemBlockTab[i].size-1;
  for (j=0, found=-1; j<xmc.noRegions; j++) {
  s1=xmcMemRegTab[j].startAddr;
  e1=s1+xmcMemRegTab[j].size-1;
  if ((s0>=s1)&&(e0<=e1))
  found=j;
  }
  if (found<0)
  LineError(xmcMemBlockTabNoL[i].line, "memory block [0x%x - 0x%x] is not covered by any memory region", s0, e0);
  }
  }
*/
/*
  void CheckMemBlock(int mB) {
  xmAddress_t s0, e0, s1, e1;
  int e;
    
  s0=xmcMemBlockTab[mB].startAddr;
  e0=s0+xmcMemBlockTab[mB].size-1;

  if (s0&(PAGE_SIZE-1))
  LineError(xmcMemBlockTabNoL[mB].startAddr, "memory block start address (0x%x) shall be aligned to 0x%x", s0, PAGE_SIZE);
  if (xmcMemBlockTab[mB].size&(PAGE_SIZE-1))
  LineError(xmcMemBlockTabNoL[mB].line, "memory block size (%d) is not multiple of %d", xmcMemBlockTab[mB].size, PAGE_SIZE);

  for (e=0; e<xmc.deviceTab.noMemBlocks-1; e++) {
  s1=xmcMemBlockTab[e].startAddr;
  e1=s1+xmcMemBlockTab[e].size-1;
  if ((s0>=e1)||(s1>=e0))
  continue;
  LineError(xmcMemBlockTabNoL[mB].line, "memory block [0x%x - 0x%x] overlaps [0x%x - 0x%x] (line %d)", s0, e0, s1, e1, xmcMemBlockTabNoL[e].startAddr);
  }
  }
*/
void CheckHpvMemAreaFlags(void) {
    xm_u32_t flags;
    int e, line;

    for (e=0; e<xmc.hpv.noPhysicalMemoryAreas; e++) {
	flags=xmcMemAreaTab[e+xmc.hpv.physicalMemoryAreasOffset].flags;
	line=xmcMemAreaTabNoL[e+xmc.hpv.physicalMemoryAreasOffset].flags;
        if (flags&XM_MEM_AREA_READONLY)
	    LineError(line, "\"read-only\" flag not permitted");
	
	if (flags&XM_MEM_AREA_UNMAPPED)
	    LineError(line, "\"unmaped\" flag not permitted");
    }
}

#if defined(CONFIG_DEV_UART)||defined(CONFIG_DEV_UART_MODULE)
void CheckUartId(int uartId, int line) {
    if ((uartId<0)||(uartId>=CONFIG_DEV_NO_UARTS))
	LineError(line, "invalid uart id %d", uartId);
}
#endif

void CheckSchedCyclicPlan(struct xmcSchedCyclicPlan *plan, struct xmcSchedCyclicPlanNoL *planNoL) {
    xm_u32_t t;
    int e;
    
    for (t=0, e=0; e<plan->noSlots; e++) {
	if (t>xmcSchedCyclicSlotTab[plan->slotsOffset+e].sExec) 
	    LineError(xmcSchedCyclicSlotTabNoL[plan->slotsOffset+e].sExec, "slot %d ([%lu - %lu] usec) overlaps slot %d ([%lu - %lu] usec)", e, xmcSchedCyclicSlotTab[plan->slotsOffset+e].sExec, xmcSchedCyclicSlotTab[plan->slotsOffset+e].eExec, e-1, xmcSchedCyclicSlotTab[plan->slotsOffset+e-1].sExec, xmcSchedCyclicSlotTab[plan->slotsOffset+e-1].eExec);
	if ((t=xmcSchedCyclicSlotTab[plan->slotsOffset+e].eExec)>plan->majorFrame)
	    LineError(xmcSchedCyclicSlotTabNoL[plan->slotsOffset+e].eExec, "last slot ([%lu - %lu] usec) overlaps major frame (%lu usec)", xmcSchedCyclicSlotTab[plan->slotsOffset+e].sExec, xmcSchedCyclicSlotTab[plan->slotsOffset+e].eExec, plan->majorFrame);

    }
}

void CheckCyclicPlanPartitionId(void) {        
    struct xmcSchedCyclicPlan *plan;
    int j, k, *part;
    DO_MALLOCZ(part, sizeof(int)*xmc.noPartitions);
    
    for (j=0; j<xmc.noSchedCyclicPlans; j++) {
        plan=&xmcSchedCyclicPlanTab[j];
        for (k=0; k<plan->noSlots; k++) {
            if ((xmcSchedCyclicSlotTab[k+plan->slotsOffset].partitionId<0)||(xmcSchedCyclicSlotTab[k+plan->slotsOffset].partitionId>=xmc.noPartitions))
                LineError(xmcSchedCyclicSlotTabNoL[k+plan->slotsOffset].partitionId, "incorrect partition id (%d)", xmcSchedCyclicSlotTab[k+plan->slotsOffset].partitionId);
            part[xmcSchedCyclicSlotTab[k+plan->slotsOffset].partitionId]=1;
        }
    }
    for (j=0; j<xmc.noPartitions; j++)
        if (!part[j])
        LineError(xmcPartitionTabNoL[j].id, "Partition %d is never scheduled", j);
    free(part);
}

void CheckPartitionName(char *name, int line) {
    int e;
    for (e=0; e<xmc.noPartitions; e++)
	if (!strcmp(&strTab[xmcPartitionTab[e].nameOffset], name))
	    LineError(line, "partition name \"%s\" duplicated (line %d)", name, xmcPartitionTabNoL[e].name);
}

#ifdef CONFIG_IPVI_SUPPORT
void CheckIpviTab(void) {
    struct srcIpvi *src;
    struct dstIpvi *dst;
    struct srcIpviNoL *srcNoL;
    struct dstIpviNoL *dstNoL;
    int i, j;

    for (i=0; i<noSrcIpvi; i++) {
        src=&srcIpviTab[i];
        srcNoL=&srcIpviTabNoL[i];
        if ((src->id<0)||(src->id>=xmc.noPartitions))
            LineError(srcNoL->id, "source partition %d doesn't exist", src->id);
        for (j=0; j<src->noDsts; j++) {
            dst=&src->dst[j];
            dstNoL=&srcNoL->dst[j];
            if ((dst->id<0)||(dst->id>=xmc.noPartitions))
                LineError(dstNoL->id, "destination partition %d doesn't exist", dst->id);
        }
    }
}
#endif
/*
void CheckAddrInsideMemArea(char *name, int line, xmAddress_t addr, xm_u32_t offset, int noMA) {
    xmAddress_t s, e;
    int i;

    for (i=0; i<noMA; i++) {
	s=xmcMemAreaTab[i+offset].startAddr;
	e=s+xmcMemAreaTab[i+offset].size-1;
	if((addr>=s)&&(addr<=e))
	    return;
    }
    LineError(line, "%s (0x%lx) outside of the defined physical memory areas", name, addr);
}
*/

