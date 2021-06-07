/*
 * $FILE: process_xml.c
 *
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
  - [02/09/2015:SPR-280715-01] added "entryPoint" as attribute to partitions
  - [20/11/2015:SPR-110915-01] ipvi stuff depends on IPVI_SUPPORT
  - [30/03/2016:CP-250216-01]  added watchdog node to hypervisor
*/

#include <string.h>
#include <libxml/tree.h>
#include "common.h"
#include "parser.h"
#include "conv.h"
#include "checks.h"
#include "xmc.h"

static int cPartition=0, cProcCyclicPlan=0;

static void NameMemArea_AH(xmlNodePtr node, const xmlChar *val) {
    xmcMemAreaTab[C_PHYSMEMAREA].nameOffset=AddString((char *)val);
    xmcMemAreaTabNoL[C_PHYSMEMAREA].name=node->line;
    xmcMemAreaTab[C_PHYSMEMAREA].flags|=XM_MEM_AREA_TAGGED;
}

static struct attrXml nameMemArea_A={BAD_CAST"name", NameMemArea_AH};

static void StartMemArea_AH(xmlNodePtr node, const xmlChar *val) {
    xmcMemAreaTab[C_PHYSMEMAREA].startAddr=ToU32((char *)val, 16);
    xmcMemAreaTab[C_PHYSMEMAREA].mappedAt=ToU32((char *)val, 16);
    // if (cPartition==-2)
    //  xmcMemAreaTab[C_PHYSMEMAREA].mappedAt=CONFIG_XM_OFFSET;

    xmcMemAreaTabNoL[C_PHYSMEMAREA].startAddr=node->line;
    xmcMemAreaTabNoL[C_PHYSMEMAREA].mappedAt=node->line;
}

static struct attrXml startMemArea_A={BAD_CAST"start", StartMemArea_AH};

static void VirtAddrMemArea_AH(xmlNodePtr node, const xmlChar *val) {
    xmcMemAreaTab[C_PHYSMEMAREA].mappedAt=ToU32((char *)val, 16);
    xmcMemAreaTabNoL[C_PHYSMEMAREA].mappedAt=node->line;
}

static struct attrXml mappedAtMemArea_A={BAD_CAST"mappedAt", VirtAddrMemArea_AH};

static void SizeMemArea_AH(xmlNodePtr node, const xmlChar *val) {
    xmcMemAreaTab[C_PHYSMEMAREA].size=ToSize((char *)val);
    xmcMemAreaTabNoL[C_PHYSMEMAREA].size=node->line;
}

static struct attrXml sizeMemArea_A={BAD_CAST"size", SizeMemArea_AH};

static void FlagsMemArea_AH(xmlNodePtr node, const xmlChar *val) {
    xmcMemAreaTab[C_PHYSMEMAREA].flags|=ToPhysMemAreaFlags((char *)val, node->line);
    xmcMemAreaTabNoL[C_PHYSMEMAREA].flags=node->line;
}

static struct attrXml flagsMemArea_A={BAD_CAST"flags", FlagsMemArea_AH};

static xm_s32_t *noMemAreaPtr=0;
static void Area_NH0(xmlNodePtr node) {
    if (noMemAreaPtr)
        (*noMemAreaPtr)++;
    xmc.noPhysicalMemoryAreas++;
    DO_REALLOC(xmcMemAreaTab, xmc.noPhysicalMemoryAreas*sizeof(struct xmcMemoryArea));
    DO_REALLOC(xmcMemAreaTabNoL, xmc.noPhysicalMemoryAreas*sizeof(struct xmcMemoryAreaNoL));
    memset(&xmcMemAreaTab[C_PHYSMEMAREA], 0, sizeof(struct xmcMemoryArea));
    memset(&xmcMemAreaTabNoL[C_PHYSMEMAREA], 0, sizeof(struct xmcMemoryAreaNoL));
    xmcMemAreaTabNoL[C_PHYSMEMAREA].line=node->line;
    xmcMemAreaTab[C_PHYSMEMAREA].nameOffset=0;
    if (cPartition==-2) {
        xmc.rsw.entryPoint=CONFIG_RDONLY_SEC_ADDR;
        xmcMemAreaTab[C_PHYSMEMAREA].startAddr=CONFIG_XM_LOAD_ADDR;
        xmcMemAreaTabNoL[C_PHYSMEMAREA].startAddr=node->line;
        xmcMemAreaTab[C_PHYSMEMAREA].mappedAt=CONFIG_XM_OFFSET;
        xmcMemAreaTabNoL[C_PHYSMEMAREA].mappedAt=node->line;
    }
}

static void Area_NH1(xmlNodePtr node) {
    xmcMemAreaTabNoL[C_PHYSMEMAREA].partitionId=cPartition;
    xmcMemAreaTab[C_PHYSMEMAREA].memoryRegionOffset=CheckPhysMemArea(C_PHYSMEMAREA);
}

static struct nodeXml area_N={BAD_CAST"Area", Area_NH0, Area_NH1, 0, (struct attrXml *[]){&nameMemArea_A, &startMemArea_A, &sizeMemArea_A, &flagsMemArea_A, &mappedAtMemArea_A, 0}, 0};

static struct nodeXml memArea_N={BAD_CAST"PhysicalMemoryAreas", 0, 0, 0, 0, (struct nodeXml *[]){&area_N, 0}};

static struct nodeXml hypMemArea_N={BAD_CAST"PhysicalMemoryArea", Area_NH0, Area_NH1, 0, (struct attrXml *[]){&sizeMemArea_A, &flagsMemArea_A, 0}, 0};

#if defined(CONFIG_MPU) && defined(CONFIG_AT697F)
static void StartSharedMemArea_AH(xmlNodePtr node, const xmlChar *val) {
    xmcSharedMemAreaTab[C_SHAREDMEMAREA].startAddr=ToU32((char *)val, 16);
    xmcSharedMemAreaTabNoL[C_SHAREDMEMAREA].startAddr=node->line;
}

static struct attrXml startSharedMemArea_A={BAD_CAST"start", StartSharedMemArea_AH};

static void SizeSharedMemArea_AH(xmlNodePtr node, const xmlChar *val) {
    xmcSharedMemAreaTab[C_SHAREDMEMAREA].size=ToSize((char *)val);
    xmcSharedMemAreaTabNoL[C_SHAREDMEMAREA].size=node->line;
}

static struct attrXml sizeSharedMemArea_A={BAD_CAST"size", SizeSharedMemArea_AH};

static void sharedMemArea_NH0(xmlNodePtr node) {
    xmc.hpv.sharedMemoryAreasOffset=xmc.hpv.noSharedMemoryAreas;
    xmc.hpv.noSharedMemoryAreas++;
    DO_REALLOC(xmcSharedMemAreaTab, xmc.hpv.noSharedMemoryAreas*sizeof(struct xmcSharedMemoryArea));
    DO_REALLOC(xmcSharedMemAreaTabNoL, xmc.hpv.noSharedMemoryAreas*sizeof(struct xmcSharedMemoryAreaNoL));
    xmcSharedMemAreaTabNoL[C_SHAREDMEMAREA].line=node->line;
}

static void sharedMemArea_NH2(xmlNodePtr node) {
    CheckSharedMemoryAreaAlignment(C_SHAREDMEMAREA);
}

static struct nodeXml sharedMemArea_N={BAD_CAST"Area", sharedMemArea_NH0, 0, sharedMemArea_NH2, (struct attrXml *[]){&startSharedMemArea_A, &sizeSharedMemArea_A, 0}, 0};

static struct nodeXml sharedMemAreas_N={BAD_CAST"SharedMemoryAreas", 0, 0, 0, 0, (struct nodeXml *[]){&sharedMemArea_N, 0}};
#endif

static int hmSlotNo=0, hmSlotLine=0;
static struct xmcHmSlot *hmTabPtr=0;

static void NameHMEvent_AH(xmlNodePtr node, const xmlChar *val) {
    hmSlotNo=ToHmEvent((char *)val, node->line);
}

static struct attrXml nameHMEvent_A={BAD_CAST"name", NameHMEvent_AH};

static void ActionHMEvent_AH(xmlNodePtr node, const xmlChar *val) {
    hmTabPtr[hmSlotNo].action=ToHmAction((char *)val, node->line);
}

static struct attrXml actionHMEvent_A={BAD_CAST"action", ActionHMEvent_AH};

static void LogHMEvent_AH(xmlNodePtr node, const xmlChar *val) {
    hmTabPtr[hmSlotNo].log=ToYNTF((char *)val, node->line);
}

static struct attrXml logHMEvent_A={BAD_CAST"log", LogHMEvent_AH};

static void EventHealthMonitor_NH0(xmlNodePtr node) {
    hmSlotLine=node->line;
    if (cPartition<0) 
        HmHpvIsActionPermittedOnEvent(hmSlotNo, hmTabPtr[hmSlotNo].action, hmSlotLine);
    else
        HmPartIsActionPermittedOnEvent(hmSlotNo, hmTabPtr[hmSlotNo].action, hmSlotLine);
}

static void EventHealthMonitor_NH1(xmlNodePtr node) {
    if (cPartition<0) 
        HmHpvIsActionPermittedOnEvent(hmSlotNo, hmTabPtr[hmSlotNo].action, hmSlotLine); 
    else
        HmPartIsActionPermittedOnEvent(hmSlotNo, hmTabPtr[hmSlotNo].action, hmSlotLine);
}

static struct nodeXml eventHealthMonitor_N={BAD_CAST"Event",  EventHealthMonitor_NH0, EventHealthMonitor_NH1, 0, (struct attrXml *[]){&nameHMEvent_A, &actionHMEvent_A, &logHMEvent_A, 0}, 0};

static struct nodeXml healthMonitor_N={BAD_CAST"HealthMonitor",  0, 0, 0, 0, (struct nodeXml *[]){&eventHealthMonitor_N, 0}};

/*static struct xmcTrace *tracePtr=0;
static struct xmcTraceNoL *traceNoLPtr=0;

static void DeviceTrace_AH(xmlNodePtr node, const xmlChar *val) {
    tracePtr->dev.id=AddDevName((char *)val);
    traceNoLPtr->dev=node->line;
}

static struct attrXml deviceTrace_A={BAD_CAST"device", DeviceTrace_AH};
*/
/*static void BitmaskTrace_AH(xmlNodePtr node, const xmlChar *val) {
    tracePtr->bitmap=ToU32((char *)val, 16);
}

static struct attrXml bitmaskTrace_A={BAD_CAST"bitmask", BitmaskTrace_AH};
*/
/*
static struct nodeXml trace_N={BAD_CAST"Trace",  0, 0, 0, (struct attrXml *[]){&deviceTrace_A, //&bitmaskTrace_A, 
0}, 0};
*/
static void IdSlot_AH(xmlNodePtr node, const xmlChar *val) {
    int d=ToU32((char *)val, 10);
    xmcSchedCyclicSlotTab[C_CYCLICSLOT].id=d;
    xmcSchedCyclicSlotTabNoL[C_CYCLICSLOT].id=node->line;
}

static struct attrXml idSlot_A={BAD_CAST"id", IdSlot_AH};

static void VCpuIdSlot_AH(xmlNodePtr node, const xmlChar *val) {
    int d=ToU32((char *)val, 10);
    
    xmcSchedCyclicSlotTab[C_CYCLICSLOT].vCpuId=d;
    xmcSchedCyclicSlotTabNoL[C_CYCLICSLOT].vCpuId=node->line;
}

static struct attrXml vCpuIdSlot_A={BAD_CAST"vCpuId", VCpuIdSlot_AH};

static void StartSlot_AH(xmlNodePtr node, const xmlChar *val) {
    xmcSchedCyclicSlotTab[C_CYCLICSLOT].sExec=ToTime((char *)val);
    xmcSchedCyclicSlotTabNoL[C_CYCLICSLOT].sExec=node->line;
}

static struct attrXml startSlot_A={BAD_CAST"start", StartSlot_AH};

static void DurationSlot_AH(xmlNodePtr node, const xmlChar *val) {
    xmcSchedCyclicSlotTab[C_CYCLICSLOT].eExec=ToTime((char *)val);
    xmcSchedCyclicSlotTabNoL[C_CYCLICSLOT].eExec=node->line;
}

static struct attrXml durationSlot_A={BAD_CAST"duration", DurationSlot_AH};

static void PartitionIdSlot_AH(xmlNodePtr node, const xmlChar *val) {
    xmcSchedCyclicSlotTab[C_CYCLICSLOT].partitionId=ToU32((char *)val, 10);
    xmcSchedCyclicSlotTabNoL[C_CYCLICSLOT].partitionId=node->line;    
}

static struct attrXml partitionIdSlot_A={BAD_CAST"partitionId", PartitionIdSlot_AH};

static void FlagsSlot_AH(xmlNodePtr node, const xmlChar *val) {
    //xmcMemAreaTab[C_PHYSMEMAREA].flags|=ToPhysMemAreaFlags((char *)val, node->line);
    xmcSchedCyclicSlotTab[C_CYCLICSLOT].flags|=ToSlotsFlags((char *)val, node->line);
    xmcSchedCyclicSlotTabNoL[C_CYCLICSLOT].flags=node->line;    
}

static struct attrXml flagsSlot_A={BAD_CAST"flags", FlagsSlot_AH};


static void Slot_NH0(xmlNodePtr node) {
    xmcSchedCyclicPlanTab[C_CYCLICPLAN].noSlots++;
    xmc.noSchedCyclicSlots++;
    DO_REALLOC(xmcSchedCyclicSlotTab, xmc.noSchedCyclicSlots*sizeof(struct xmcSchedCyclicSlot));
    DO_REALLOC(xmcSchedCyclicSlotTabNoL, xmc.noSchedCyclicSlots*sizeof(struct xmcSchedCyclicSlotNoL));
    memset(&xmcSchedCyclicSlotTab[C_CYCLICSLOT], 0, sizeof(struct xmcSchedCyclicSlot));
    memset(&xmcSchedCyclicSlotTabNoL[C_CYCLICSLOT], 0, sizeof(struct xmcSchedCyclicSlotNoL));
}

static void Slot_NH1(xmlNodePtr node) {
    xmcSchedCyclicSlotTab[C_CYCLICSLOT].eExec+=xmcSchedCyclicSlotTab[C_CYCLICSLOT].sExec;
}

static struct nodeXml slot_N={BAD_CAST"Slot", Slot_NH0, Slot_NH1, 0, (struct attrXml *[]){&idSlot_A, &vCpuIdSlot_A, &startSlot_A, &durationSlot_A, &partitionIdSlot_A, &flagsSlot_A, 0}, 0};

static void NameCyclicPlan_AH(xmlNodePtr node, const xmlChar *val) {
    xmcSchedCyclicPlanTab[C_CYCLICPLAN].nameOffset=AddString((char *)val);
    xmcSchedCyclicPlanTabNoL[C_CYCLICPLAN].name=node->line;
}

static struct attrXml nameCyclicPlan_A={BAD_CAST"name", NameCyclicPlan_AH};

static void IdCyclicPlan_AH(xmlNodePtr node, const xmlChar *val) {
    int d=ToU32((char *)val, 10);
    if (d!=(cProcCyclicPlan-1))
	LineError(node->line, "Cyclic plan id (%d) shall be consecutive starting at 0", d);

    xmcSchedCyclicPlanTab[C_CYCLICPLAN].id=d;
}

static struct attrXml idCyclicPlan_A={BAD_CAST"id", IdCyclicPlan_AH};

static void MajorFrameCyclicPlan_AH(xmlNodePtr node, const xmlChar *val) {
    xmcSchedCyclicPlanTab[C_CYCLICPLAN].majorFrame=ToTime((char *)val);
    xmcSchedCyclicPlanTabNoL[C_CYCLICPLAN].majorFrame=node->line;
}

static struct attrXml majorFrameCyclicPlan_A={BAD_CAST"majorFrame", MajorFrameCyclicPlan_AH};

#ifdef CONFIG_PLAN_EXTSYNC
static void ExtSyncCyclicPlan_AH(xmlNodePtr node, const xmlChar *val) {
    xmcSchedCyclicPlanTab[C_CYCLICPLAN].extSync=ToU32((char *)val, 10);
    xmcSchedCyclicPlanTabNoL[C_CYCLICPLAN].extSync=node->line;
}

static struct attrXml extSyncCyclicPlan_A={BAD_CAST"extSync", ExtSyncCyclicPlan_AH};

static void IntervalCyclicPlan_AH(xmlNodePtr node, const xmlChar *val) {
    xmcSchedCyclicPlanTab[C_CYCLICPLAN].interval=ToTime((char *)val);
    xmcSchedCyclicPlanTabNoL[C_CYCLICPLAN].interval=node->line;
}

static struct attrXml intervalCyclicPlan_A={BAD_CAST"interval", IntervalCyclicPlan_AH};
#endif

static void Plan_NH0(xmlNodePtr node) {
    cProcCyclicPlan++;
    xmc.noSchedCyclicPlans++;
    xmc.hpv.cpuTab[C_CPU].noSchedCyclicPlans++;
    DO_REALLOC(xmcSchedCyclicPlanTab, xmc.noSchedCyclicPlans*sizeof(struct xmcSchedCyclicPlan));
    DO_REALLOC(xmcSchedCyclicPlanTabNoL, xmc.noSchedCyclicPlans*sizeof(struct xmcSchedCyclicPlanNoL));
    memset(&xmcSchedCyclicPlanTab[C_CYCLICPLAN], 0, sizeof(struct xmcSchedCyclicPlan));
    memset(&xmcSchedCyclicPlanTabNoL[C_CYCLICPLAN], 0, sizeof(struct xmcSchedCyclicPlanNoL));
    xmcSchedCyclicPlanTab[C_CYCLICPLAN].slotsOffset=xmc.noSchedCyclicSlots;
#ifdef CONFIG_PLAN_EXTSYNC
    xmcSchedCyclicPlanTab[C_CYCLICPLAN].extSync=-1;
    xmcSchedCyclicPlanTab[C_CYCLICPLAN].interval=-1;
#endif
}

static void Plan_NH2(xmlNodePtr node) {
    CheckSchedCyclicPlan(&xmcSchedCyclicPlanTab[C_CYCLICPLAN], &xmcSchedCyclicPlanTabNoL[C_CYCLICPLAN]);
}

static struct nodeXml plan_N={BAD_CAST"Plan", Plan_NH0, 0, Plan_NH2, (struct attrXml *[]){&nameCyclicPlan_A, &idCyclicPlan_A, &majorFrameCyclicPlan_A,
#ifdef CONFIG_PLAN_EXTSYNC
&extSyncCyclicPlan_A, &intervalCyclicPlan_A,
#endif
0}, (struct nodeXml *[]){&slot_N, 0}};


static void CyclicPlan_NH0(xmlNodePtr node) {    
    xmc.hpv.cpuTab[C_CPU].schedCyclicPlansOffset=xmc.noSchedCyclicPlans;
}

static struct nodeXml cyclicPlan_N={BAD_CAST"CyclicPlanTable", CyclicPlan_NH0, 0, 0, 0, (struct nodeXml *[]){&plan_N, 0}};

static void IdProcessor_AH(xmlNodePtr node, const xmlChar *val) {
    int d=ToU32((char *)val, 10);
    if (d!=C_CPU)
	LineError(node->line, "processor id (%d) shall be consecutive starting at 0", d);    
    xmc.hpv.cpuTab[C_CPU].id=d;
    xmcNoL.hpv.cpuTab[C_CPU].id=node->line;
}

static struct attrXml idProcessor_A={BAD_CAST"id", IdProcessor_AH};

static void FrequencyProcessor_AH(xmlNodePtr node, const xmlChar *val) {
    xmc.hpv.cpuTab[C_CPU].freq=ToFreq((char *)val);
    xmcNoL.hpv.cpuTab[C_CPU].freq=node->line;
}

static struct attrXml frequencyProcessor_A={BAD_CAST"frequency", FrequencyProcessor_AH};

static void FeaturesProcessor_AH(xmlNodePtr node, const xmlChar *val) {
#ifdef CONFIG_MPU
    if (strcmp((char *)val, "XM_CPU_LEON2_WA1")==0){
       xmc.hpv.cpuTab[C_CPU].features=XM_CPU_FEATURE_WA1;
       xmcNoL.hpv.cpuTab[C_CPU].features=node->line;
    }
#else
    if (strcmp((char *)val, "XM_CPU_LEON2_WA1")==0)
       LineError(node->line,"Feature \"%s\" does not supported on the current architecture.\n",val);
#endif
}

static struct attrXml featuresProcessor_A={BAD_CAST"features",FeaturesProcessor_AH};

static void Processor_NH0(xmlNodePtr node) {
    xmc.hpv.noCpus++;
    cProcCyclicPlan=0;
    if (xmc.hpv.noCpus>CONFIG_NO_CPUS)
        LineError(node->line, "no more than %d cpus are supported", CONFIG_NO_CPUS);
}

static void ConsoleProcessor_AH(xmlNodePtr node, const xmlChar *val) {
    xmc.hpv.cpuTab[C_CPU].consoleDev.id=AddDevName((char *)val);
    xmcNoL.hpv.cpuTab[C_CPU].consoleDev=node->line;
}

static struct attrXml consoleProcessor_A={BAD_CAST"console", ConsoleProcessor_AH};

static struct nodeXml processor_N={BAD_CAST"Processor", Processor_NH0, 0, 0, (struct attrXml *[]){&idProcessor_A, &frequencyProcessor_A, &featuresProcessor_A, &consoleProcessor_A, 0}, (struct nodeXml *[]){&cyclicPlan_N, 0}};

static struct nodeXml processorTable_N={BAD_CAST"ProcessorTable", 0, 0, 0, 0, (struct nodeXml *[]){&processor_N, 0}};

static void TypeMemReg_AH(xmlNodePtr node, const xmlChar *val) {
    xmcMemRegTab[C_REGION].flags=ToRegionFlags((char *)val);
    xmcMemRegTabNoL[C_REGION].flags=node->line;
}

static struct attrXml typeMemReg_A={BAD_CAST"type", TypeMemReg_AH};

static void StartMemReg_AH(xmlNodePtr node, const xmlChar *val) {
    xmcMemRegTab[C_REGION].startAddr=ToU32((char *)val, 16);
    xmcMemRegTabNoL[C_REGION].startAddr=node->line;
}

static struct attrXml startMemReg_A={BAD_CAST"start", StartMemReg_AH};

static void SizeMemReg_AH(xmlNodePtr node, const xmlChar *val) {
    xmcMemRegTab[C_REGION].size=ToSize((char *)val);
    xmcMemRegTabNoL[C_REGION].size=node->line;
}

static struct attrXml sizeMemReg_A={BAD_CAST"size", SizeMemReg_AH};

static void Region_NH0(xmlNodePtr node) {
    xmc.noRegions++;
    DO_REALLOC(xmcMemRegTab, xmc.noRegions*sizeof(struct xmcMemoryRegion));
    DO_REALLOC(xmcMemRegTabNoL, xmc.noRegions*sizeof(struct xmcMemoryRegionNoL)); 
    xmcMemRegTabNoL[C_REGION].line=node->line;
}

static void Region_NH1(xmlNodePtr node) {
    CheckMemoryRegion(C_REGION);
}

static struct nodeXml region_N={BAD_CAST"Region", Region_NH0, Region_NH1, 0, (struct attrXml *[]){&typeMemReg_A, &startMemReg_A, &sizeMemReg_A, 0}, 0};

static struct nodeXml memoryLayout_N={BAD_CAST"MemoryLayout", 0, 0, 0, 0, (struct nodeXml *[]){&region_N, 0}};

#if defined(CONFIG_DEV_MEMBLOCK)||defined(CONFIG_DEV_MEMBLOCK_MODULE)
static void nameMemBlockDev_AH(xmlNodePtr node, const xmlChar *val) {
    RegisterDev((char *)val, (xmDev_t){XM_DEV_LOGSTORAGE_ID, xmc.deviceTab.noMemBlocks-1}, node->line);
}

static struct attrXml nameMemBlockDev_A={BAD_CAST"name", nameMemBlockDev_AH};

static void startMemBlockDev_AH(xmlNodePtr node, const xmlChar *val) {
    xmcMemAreaTab[C_PHYSMEMAREA].flags=0;
    xmcMemAreaTab[C_PHYSMEMAREA].startAddr=ToU32((char *)val, 16);
    xmcMemAreaTabNoL[C_PHYSMEMAREA].startAddr=node->line;

//    xmcMemBlockTab[C_MEMORYBLOCK_DEV].startAddr=ToU32((char *)val, 16);
//    xmcMemBlockTabNoL[C_MEMORYBLOCK_DEV].startAddr=node->line;
}

static struct attrXml startMemBlockDev_A={BAD_CAST"start", startMemBlockDev_AH};

static void sizeMemBlockDev_AH(xmlNodePtr node, const xmlChar *val) {
    xmcMemAreaTab[C_PHYSMEMAREA].size=ToSize((char *)val);
    xmcMemAreaTabNoL[C_PHYSMEMAREA].size=node->line;

    //xmcMemBlockTab[C_MEMORYBLOCK_DEV].size=ToSize((char *)val);
    //xmcMemBlockTabNoL[C_MEMORYBLOCK_DEV].size=node->line;
}

static struct attrXml sizeMemBlockDev_A={BAD_CAST"size", sizeMemBlockDev_AH};

static void MemoryBlockD_NH0(xmlNodePtr node) {
    xmc.deviceTab.noMemBlocks++;
    DO_REALLOC(xmcMemBlockTab, xmc.deviceTab.noMemBlocks*sizeof(struct xmcMemBlock));
    DO_REALLOC(xmcMemBlockTabNoL, xmc.deviceTab.noMemBlocks*sizeof(struct xmcMemBlockNoL));
    memset(&xmcMemBlockTab[C_MEMORYBLOCK_DEV], 0, sizeof(struct xmcMemBlock));
    memset(&xmcMemBlockTabNoL[C_MEMORYBLOCK_DEV], 0, sizeof(struct xmcMemBlockNoL));
    xmcMemBlockTabNoL[C_MEMORYBLOCK_DEV].line=node->line;
    xmcMemBlockTab[C_MEMORYBLOCK_DEV].physicalMemoryAreasOffset=xmc.noPhysicalMemoryAreas;
    ////////////////
    xmc.noPhysicalMemoryAreas++;
    DO_REALLOC(xmcMemAreaTab, xmc.noPhysicalMemoryAreas*sizeof(struct xmcMemoryArea));
    DO_REALLOC(xmcMemAreaTabNoL, xmc.noPhysicalMemoryAreas*sizeof(struct xmcMemoryAreaNoL));
    memset(&xmcMemAreaTab[C_PHYSMEMAREA], 0, sizeof(struct xmcMemoryArea));
    memset(&xmcMemAreaTabNoL[C_PHYSMEMAREA], 0, sizeof(struct xmcMemoryAreaNoL));
    xmcMemAreaTabNoL[C_PHYSMEMAREA].line=node->line;

}

static void MemoryBlockD_NH1(xmlNodePtr node) {
    //CheckMemBlock(C_MEMORYBLOCK_DEV);
    
    /////////////
    xmcMemAreaTabNoL[C_PHYSMEMAREA].partitionId=cPartition;
    xmcMemAreaTab[C_PHYSMEMAREA].memoryRegionOffset=CheckPhysMemArea(C_PHYSMEMAREA);
}

static struct nodeXml memoryBlockD_N={BAD_CAST"MemoryBlock", MemoryBlockD_NH0, MemoryBlockD_NH1, 0, (struct attrXml *[]){&nameMemBlockDev_A, &startMemBlockDev_A, &sizeMemBlockDev_A, 0}, 0};
#endif
#if defined(CONFIG_DEV_UART)||defined(CONFIG_DEV_UART_MODULE)
static int uartId;

static void IdUart_AH(xmlNodePtr node, const xmlChar *val) {
    uartId=ToU32((char *)val, 10);
    CheckUartId(uartId, node->line);
}

static struct attrXml idUart_A={BAD_CAST"id", IdUart_AH};

static void NameUart_AH(xmlNodePtr node, const xmlChar *val) {
    RegisterDev((char *)val, (xmDev_t){XM_DEV_UART_ID, uartId}, node->line);
}

static struct attrXml nameUart_A={BAD_CAST"name", NameUart_AH};

static void BaudRateUart_AH(xmlNodePtr node, const xmlChar *val) {
    xmc.deviceTab.uart[uartId].baudRate=ToU32((char *)val, 10);
}

static struct attrXml baudRateUart_A={BAD_CAST"baudRate", BaudRateUart_AH};

static struct nodeXml uartD_N={BAD_CAST"Uart", 0, 0, 0, (struct attrXml *[]){&idUart_A, &nameUart_A, &baudRateUart_A, 0}, 0};
#endif

static void Devices_NH0(xmlNodePtr node) {
    RegisterDev((char *)"Null", (xmDev_t){XM_DEV_INVALID_ID, 0}, node->line);
}

static struct nodeXml devices_N={BAD_CAST"Devices", Devices_NH0, 0, 0, 0, (struct nodeXml *[]){
#if defined(CONFIG_DEV_MEMBLOCK)||defined(CONFIG_DEV_MEMBLOCK_MODULE)
&memoryBlockD_N, 
#endif
#if defined(CONFIG_DEV_UART)||defined(CONFIG_DEV_UART_MODULE)
&uartD_N, 
#endif
0}};

static void PartitionIdIpc_AH(xmlNodePtr node, const xmlChar *val) {
    ipcPortTab[C_IPCPORT].partitionId=ToU32((char *)val, 10);
    ipcPortTabNoL[C_IPCPORT].partitionId=node->line;
}

static struct attrXml partitionIdIpc_A={BAD_CAST"partitionId", PartitionIdIpc_AH};

static void PartitionNameIpc_AH(xmlNodePtr node, const xmlChar *val) {
    ipcPortTab[C_IPCPORT].partitionName=strdup((char *)val);
    ipcPortTabNoL[C_IPCPORT].partitionName=node->line;
}

static struct attrXml partitionNameIpc_A={BAD_CAST"partitionName", PartitionNameIpc_AH};

static void PortNameIpc_AH(xmlNodePtr node, const xmlChar *val) {
    ipcPortTab[C_IPCPORT].portName=strdup((char *)val);
    ipcPortTabNoL[C_IPCPORT].portName=node->line;
}

static struct attrXml portNameIpc_A={BAD_CAST"portName", PortNameIpc_AH};

static void CpuIdIpc_AH(xmlNodePtr node, const xmlChar *val) {
    ipcPortTab[C_IPCPORT].cpuId=ToU32((char *)val, 10);
    //ipcPortTabNoL[C_IPCPORT].portName=node->line;
}

static struct attrXml cpuIdIpc_A={BAD_CAST"cpuId", CpuIdIpc_AH};

static void IpcPort_NH0(xmlNodePtr node) {
    noIpcPorts++;
    DO_REALLOC(ipcPortTab, noIpcPorts*sizeof(struct ipcPort));
    DO_REALLOC(ipcPortTabNoL, noIpcPorts*sizeof(struct ipcPortNoL));
    memset(&ipcPortTab[C_IPCPORT], 0, sizeof(struct ipcPort));
    memset(&ipcPortTabNoL[C_IPCPORT], 0, sizeof(struct ipcPortNoL));
    ipcPortTab[C_IPCPORT].channel=C_COMM_CHANNEL;
}

static void SourceIpcPort_NH1(xmlNodePtr node) {
    ipcPortTab[C_IPCPORT].direction=XM_SOURCE_PORT;
}

static void ExternalSourceIpcPort_NH1(xmlNodePtr node) {
    ipcPortTab[C_IPCPORT].direction=XM_SOURCE_PORT;
    ipcPortTab[C_IPCPORT].flags|=XM_EXTERNAL_PORT;
}

static void ExternalDestinationIpcPort_NH1(xmlNodePtr node) {
    ipcPortTab[C_IPCPORT].direction=XM_DESTINATION_PORT;
    ipcPortTab[C_IPCPORT].flags|=XM_EXTERNAL_PORT;
}

static struct nodeXml externalDestinationIpcPort_N={BAD_CAST"ExternalDestination", IpcPort_NH0, ExternalDestinationIpcPort_NH1, 0, (struct attrXml *[]){&portNameIpc_A, &cpuIdIpc_A, 0}, 0};

static struct nodeXml externalSourceIpcPort_N={BAD_CAST"ExternalSource", IpcPort_NH0, ExternalSourceIpcPort_NH1, 0, (struct attrXml *[]){&portNameIpc_A, &cpuIdIpc_A, 0}, 0};

static struct nodeXml sourceIpcPort_N={BAD_CAST"Source", IpcPort_NH0, SourceIpcPort_NH1, 0, (struct attrXml *[]){&partitionIdIpc_A, &partitionNameIpc_A, &portNameIpc_A, 0}, 0};

static void DestinationIpcPort_NH1(xmlNodePtr node) {
    ipcPortTab[C_IPCPORT].direction=XM_DESTINATION_PORT;
}

static struct nodeXml destinationIpcPort_N={BAD_CAST"Destination", IpcPort_NH0, DestinationIpcPort_NH1, 0, (struct attrXml *[]){&partitionIdIpc_A, &partitionNameIpc_A, &portNameIpc_A, 0}, 0};

static void MaxMessageLengthSChannel_AH(xmlNodePtr node, const xmlChar *val) {
    xmcCommChannelTab[C_COMM_CHANNEL].s.maxLength=ToSize((char *)val);
    xmcCommChannelTabNoL[C_COMM_CHANNEL].s.maxLength=node->line;
}

static struct attrXml maxMessageLengthSChannel_A={BAD_CAST"maxMessageLength", MaxMessageLengthSChannel_AH};

static void AddressSChannel_AH(xmlNodePtr node, const xmlChar *val) {
    xmcCommChannelTab[C_COMM_CHANNEL].s.address=ToU32((char *)val, 16);
    xmcCommChannelTabNoL[C_COMM_CHANNEL].s.address=node->line;
}

static struct attrXml addressSChannel_A={BAD_CAST"address", AddressSChannel_AH};

static void SizeSChannel_AH(xmlNodePtr node, const xmlChar *val) {
    xmcCommChannelTab[C_COMM_CHANNEL].s.size=ToSize((char *)val);
    xmcCommChannelTabNoL[C_COMM_CHANNEL].s.size=node->line;
}

static struct attrXml sizeSChannel_A={BAD_CAST"size", SizeSChannel_AH};

static void RefreshPeriodSChannel_AH(xmlNodePtr node, const xmlChar *val) {
    xmcCommChannelTab[C_COMM_CHANNEL].s.refreshPeriod=ToTime((char *)val);
    //xmcCommChannelTabNoL[C_COMM_CHANNEL].s.refreshPeriod=node->line;
}

static struct attrXml refreshPeriodSChannel_A={BAD_CAST"refreshPeriod", RefreshPeriodSChannel_AH};

static void Channel_NH0(xmlNodePtr node) {
    xmc.noCommChannels++;
    DO_REALLOC(xmcCommChannelTab, xmc.noCommChannels*sizeof(struct xmcCommChannel));
    DO_REALLOC(xmcCommChannelTabNoL, xmc.noCommChannels*sizeof(struct xmcCommChannelNoL));
    memset(&xmcCommChannelTab[C_COMM_CHANNEL], 0, sizeof(struct xmcCommChannel));
    memset(&xmcCommChannelTabNoL[C_COMM_CHANNEL], 0, sizeof(struct xmcCommChannelNoL));    
}

static void SamplingChannel_NH1(xmlNodePtr node) {
    xmcCommChannelTab[C_COMM_CHANNEL].type=XM_SAMPLING_CHANNEL;
    xmcCommChannelTabNoL[C_COMM_CHANNEL].type=node->line;
}

static struct nodeXml samplingChannel_N={BAD_CAST"SamplingChannel", Channel_NH0, SamplingChannel_NH1, 0, (struct attrXml *[]){&maxMessageLengthSChannel_A, &refreshPeriodSChannel_A, &addressSChannel_A, &sizeSChannel_A, 0}, (struct nodeXml *[]){&sourceIpcPort_N, &destinationIpcPort_N, &externalSourceIpcPort_N, &externalDestinationIpcPort_N, 0}};

static void MaxMessageLengthQChannel_AH(xmlNodePtr node, const xmlChar *val) {
    xmcCommChannelTab[C_COMM_CHANNEL].q.maxLength=ToSize((char *)val);
    xmcCommChannelTabNoL[C_COMM_CHANNEL].q.maxLength=node->line;
}

static void MaxNoMessagesQChannel_AH(xmlNodePtr node, const xmlChar *val) {
    xmcCommChannelTab[C_COMM_CHANNEL].q.maxNoMsgs=ToU32((char *)val, 10);
    xmcCommChannelTabNoL[C_COMM_CHANNEL].q.maxNoMsgs=node->line;
}

static void MaxTimeExpirationQChannel_AH(xmlNodePtr node, const xmlChar *val) {
    xmcCommChannelTab[C_COMM_CHANNEL].q.maxTimeExpiration=ToTime((char *)val);
//ToU32((char *)val, 10);
    //xmcCommChannelTabNoL[C_COMM_CHANNEL].q.maxNoMsgs=node->line;
}

static void AddressQChannel_AH(xmlNodePtr node, const xmlChar *val) {
    xmcCommChannelTab[C_COMM_CHANNEL].s.address=ToU32((char *)val, 16);
    xmcCommChannelTabNoL[C_COMM_CHANNEL].s.address=node->line;
}

static struct attrXml addressQChannel_A={BAD_CAST"address", AddressQChannel_AH};

static void SizeQChannel_AH(xmlNodePtr node, const xmlChar *val) {
    xmcCommChannelTab[C_COMM_CHANNEL].s.size=ToSize((char *)val);
    xmcCommChannelTabNoL[C_COMM_CHANNEL].s.size=node->line;
}

static struct attrXml sizeQChannel_A={BAD_CAST"size", SizeQChannel_AH};

static struct attrXml maxMessageLengthQChannel_A={BAD_CAST"maxMessageLength", MaxMessageLengthQChannel_AH};

static struct attrXml maxNoMessagesQChannel_A={BAD_CAST"maxNoMessages", MaxNoMessagesQChannel_AH};

static struct attrXml maxTimeExpirationQChannel_A={BAD_CAST"maxTimeExpiration", MaxTimeExpirationQChannel_AH};

static void QueuingChannel_NH1(xmlNodePtr node) {
    xmcCommChannelTab[C_COMM_CHANNEL].type=XM_QUEUING_CHANNEL;
    xmcCommChannelTabNoL[C_COMM_CHANNEL].type=node->line;
}

static struct nodeXml queuingChannel_N={BAD_CAST"QueuingChannel", Channel_NH0, QueuingChannel_NH1, 0, (struct attrXml *[]){&maxMessageLengthQChannel_A, &maxNoMessagesQChannel_A, &maxTimeExpirationQChannel_A, &addressQChannel_A, &sizeQChannel_A, 0}, (struct nodeXml *[]){&sourceIpcPort_N, &destinationIpcPort_N, &externalSourceIpcPort_N, &externalDestinationIpcPort_N, 0}};

#ifdef CONFIG_IPVI_SUPPORT
static xm_s32_t ipviId;
struct srcIpvi *srcIpvi;
struct dstIpvi *dstIpvi;
struct srcIpviNoL *srcIpviNoL;
struct dstIpviNoL *dstIpviNoL;

static int ipviId;

static void SrcIdIpvi_AH(xmlNodePtr node, const xmlChar *val) {
    int id=ToU32((char *)val, 10), e;
    for (e=0; e<noSrcIpvi; e++) {
        srcIpvi=&srcIpviTab[e];
        srcIpviNoL=&srcIpviTabNoL[e];
//        if (srcIpvi->id==id)
//            break;
    }
    if (e>=noSrcIpvi) {
        noSrcIpvi++;
        DO_REALLOC(srcIpviTab, noSrcIpvi*sizeof(struct srcIpvi));
        DO_REALLOC(srcIpviTabNoL, noSrcIpvi*sizeof(struct srcIpviNoL));
        srcIpvi=&srcIpviTab[noSrcIpvi-1];
        srcIpviNoL=&srcIpviTabNoL[noSrcIpvi-1];
        memset(srcIpvi, 0, sizeof(struct srcIpvi));
        memset(srcIpviNoL, 0, sizeof(struct srcIpviNoL));
        srcIpvi->id=id;
        srcIpviNoL->id=node->line;
    }
    srcIpvi->ipviId=ipviId;
}

static struct attrXml srcIdIpvi_A={BAD_CAST"sourceId", SrcIdIpvi_AH};

static void DstIdIpvi_AH(xmlNodePtr node, const xmlChar *val) {
    void _CB(int line, char *val) {
        int id=ToU32((char *)val, 10), e;

        for (e=0; e<srcIpvi->noDsts; e++) {
            dstIpvi=&srcIpvi->dst[e];
            dstIpviNoL=&srcIpviNoL->dst[e];
            if (dstIpvi->id==id)
                break;
        }
        
        if (e>=srcIpvi->noDsts) {
            srcIpvi->noDsts++;
            DO_REALLOC(srcIpvi->dst, srcIpvi->noDsts*sizeof(struct dstIpvi));
            DO_REALLOC(srcIpviNoL->dst, srcIpvi->noDsts*sizeof(struct dstIpviNoL));
            dstIpvi=&srcIpvi->dst[srcIpvi->noDsts-1];
            dstIpviNoL=&srcIpviNoL->dst[srcIpvi->noDsts-1];
            memset(dstIpvi, 0, sizeof(struct dstIpvi));
            memset(dstIpviNoL, 0, sizeof(struct dstIpviNoL));
            dstIpvi->id=id;
            dstIpviNoL->id=node->line;
        } else {
            LineError(node->line, "Duplicated ipvi partition destination");
        }
    }
    ProcessIdList((char *)val, _CB, node->line);
}

static struct attrXml dstIdIpvi_A={BAD_CAST"destinationId", DstIdIpvi_AH};

static void IdIpvi_AH(xmlNodePtr node, const xmlChar *val) {    
    ipviId=ToU32((char *)val, 10);
    if ((ipviId<0)||(ipviId>CONFIG_MAX_NO_IPVI))
        LineError(node->line, "Invalid ipvi id %d", ipviId);
}

static struct attrXml idIpvi_A={BAD_CAST"id", IdIpvi_AH};

static struct nodeXml ipviChannel_N={BAD_CAST"Ipvi", 0, 0, 0, (struct attrXml *[]){&idIpvi_A, &srcIdIpvi_A, &dstIdIpvi_A, 0}, (struct nodeXml *[]){0}};
#endif

static struct nodeXml channels_N={BAD_CAST"Channels", 0, 0, 0, 0, (struct nodeXml *[]){&samplingChannel_N, &queuingChannel_N, 
#ifdef CONFIG_IPVI_SUPPORT
    &ipviChannel_N, 
#endif
0}};

static void IntLines_AH(xmlNodePtr node, const xmlChar *val) {
    ToHwIrqLines((char *)val, node->line);
}

static struct attrXml intLines_A={BAD_CAST"lines", IntLines_AH};

static struct nodeXml hwInterrupts_N={BAD_CAST"Interrupts", 0, 0, 0, (struct attrXml *[]){&intLines_A, 0}, 0};

extern struct nodeXml ioPorts_N;

static struct nodeXml hwResourcesPartition_N={BAD_CAST"HwResources", 0, 0, 0, 0, (struct nodeXml *[]){
&ioPorts_N, 
&hwInterrupts_N, 0}};

static void NameCommPort_AH(xmlNodePtr node, const xmlChar *val) {
    xmcCommPortTab[C_COMMPORT].nameOffset=AddString((char *)val);
    xmcCommPortTabNoL[C_COMMPORT].name=node->line;
}

static struct attrXml nameCommPort_A={BAD_CAST"name", NameCommPort_AH};

static void DirectionCommPort_AH(xmlNodePtr node, const xmlChar *val) {
    xmcCommPortTab[C_COMMPORT].direction=ToCommPortDirection((char *)val, node->line);
    xmcCommPortTabNoL[C_COMMPORT].direction=node->line;
}

static struct attrXml directionCommPort_A={BAD_CAST"direction", DirectionCommPort_AH};

static void TypeCommPort_AH(xmlNodePtr node, const xmlChar *val) {
    xmcCommPortTab[C_COMMPORT].type=ToCommPortType((char *)val, node->line);
    xmcCommPortTabNoL[C_COMMPORT].type=node->line;
}

static struct attrXml typeCommPort_A={BAD_CAST"type", TypeCommPort_AH};

static void CommPort_NH0(xmlNodePtr node) {
    xmc.noCommPorts++;
    xmcPartitionTab[C_PARTITION].noPorts++;
    DO_REALLOC(xmcCommPortTab, xmc.noCommPorts*sizeof(struct xmcCommPort));
    DO_REALLOC(xmcCommPortTabNoL, xmc.noCommPorts*sizeof(struct xmcCommPortNoL));
    memset(&xmcCommPortTab[C_COMMPORT], 0, sizeof(struct xmcCommPort));
    memset(&xmcCommPortTabNoL[C_COMMPORT], 0, sizeof(struct xmcCommPortNoL));
    xmcCommPortTab[C_COMMPORT].channelId=-1;
}

static void CommPort_NH1(xmlNodePtr node) {
    CheckPortName(C_COMMPORT, C_PARTITION);
}

static void DisciplineQPort_AH(xmlNodePtr node, const xmlChar *val) {
    if ((xmcCommPortTab[C_COMMPORT].type&XM_QUEUING_PORT)!=XM_QUEUING_PORT)
        LineError(node->line, "Discipline attribute can be only used by queuing ports");

    if (strcmp((char *)val, "FIFO"))
        xmcCommPortTab[C_COMMPORT].flags|=XM_PRIORITY_PORT;
    else
        xmcCommPortTab[C_COMMPORT].flags|=XM_FIFO_PORT;
}

static struct attrXml disciplineQPort_A={BAD_CAST"discipline", DisciplineQPort_AH};

static struct nodeXml commPort_N={BAD_CAST"Port", CommPort_NH0, CommPort_NH1, 0, (struct attrXml *[]){&nameCommPort_A, &directionCommPort_A, &typeCommPort_A, &disciplineQPort_A, 0}, 0};

static void PortTable_NH0(xmlNodePtr node) {
    xmcPartitionTab[C_PARTITION].commPortsOffset=xmc.noCommPorts;
}

static struct nodeXml portTable_N={BAD_CAST"PortTable", PortTable_NH0, 0, 0, 0, (struct nodeXml *[]){&commPort_N, 0}};

static void IdPartition_AH(xmlNodePtr node, const xmlChar *val) {    
    int d=ToU32((char *)val, 10);
    
    //  if (d!=C_PARTITION)
//	LineError(node->line, "partition id (%d) shall be consecutive starting at 0", d);

    xmcPartitionTab[C_PARTITION].rId=d;
    xmcPartitionTab[C_PARTITION].id=C_PARTITION;
    
    xmcPartitionTabNoL[C_PARTITION].id=node->line;
}

static struct attrXml idPartition_A={BAD_CAST"id", IdPartition_AH};

static void NamePartition_AH(xmlNodePtr node, const xmlChar *val) {
    CheckPartitionName((char *)val, node->line);
    xmcPartitionTab[C_PARTITION].nameOffset=AddString((char *)val);
    xmcPartitionTabNoL[C_PARTITION].name=node->line;
}

static struct attrXml namePartition_A={BAD_CAST"name", NamePartition_AH};

static void ConsolePartition_AH(xmlNodePtr node, const xmlChar *val) {
    xmcPartitionTab[C_PARTITION].consoleDev.id=AddDevName((char *)val);
    xmcPartitionTabNoL[C_PARTITION].consoleDev=node->line;
}

static struct attrXml consolePartition_A={BAD_CAST"console", ConsolePartition_AH};

static void FlagsPartition_AH(xmlNodePtr node, const xmlChar *val) {
    xmcPartitionTab[C_PARTITION].flags=ToPartitionFlags((char *)val, node->line);
    xmcPartitionTabNoL[C_PARTITION].flags=node->line;
}

static struct attrXml flagsPartition_A={BAD_CAST"flags", FlagsPartition_AH};

static void EntryPointPartition_AH(xmlNodePtr node, const xmlChar *val) {
    xmcPartitionTab[C_PARTITION].entryPoint = ToU32((char *)val, 16);
    xmcPartitionTabNoL[C_PARTITION].entryPoint = node->line;
}

static struct attrXml entryPointPartition_A={BAD_CAST"entryPoint", EntryPointPartition_AH};

static void Partition_NH0(xmlNodePtr node) {
    xmc.noPartitions++;
    cPartition=C_PARTITION;
    DO_REALLOC(xmcPartitionTab, xmc.noPartitions*sizeof(struct xmcPartition));
    DO_REALLOC(xmcPartitionTabNoL, xmc.noPartitions*sizeof(struct xmcPartitionNoL));
    memset(&xmcPartitionTab[C_PARTITION], 0, sizeof(struct xmcPartition));
    memset(&xmcPartitionTabNoL[C_PARTITION], 0, sizeof(struct xmcPartitionNoL));
    xmcPartitionTab[C_PARTITION].consoleDev.id=XM_DEV_INVALID_ID;
    xmcPartitionTab[C_PARTITION].physicalMemoryAreasOffset=xmc.noPhysicalMemoryAreas;
    noMemAreaPtr=&xmcPartitionTab[C_PARTITION].noPhysicalMemoryAreas;
    hmTabPtr=xmcPartitionTab[C_PARTITION].hmTab;    
    //tracePtr=&xmcPartitionTab[C_PARTITION].trace;
    //traceNoLPtr=&xmcPartitionTabNoL[C_PARTITION].trace;
    //tracePtr->dev.id=XM_DEV_INVALID_ID;
    SetupDefaultPartHmActions(xmcPartitionTab[C_PARTITION].hmTab);
}

static void Partition_NH2(xmlNodePtr node) {
    CheckMemAreaPerPart();
    CheckPartitionEntryPoint();
}

static struct nodeXml partition_N={BAD_CAST"Partition", Partition_NH0, 0, Partition_NH2, (struct attrXml *[]){&idPartition_A, &namePartition_A, &consolePartition_A, &flagsPartition_A, &entryPointPartition_A, 0}, (struct nodeXml *[]){&memArea_N, &healthMonitor_N, //&trace_N, 
&hwResourcesPartition_N, &portTable_N, 0}};

static struct nodeXml partitionTable_N={BAD_CAST"PartitionTable", 0, 0, 0, 0, (struct nodeXml *[]){&partition_N, 0}};

static void  ResidentSw_NH0(xmlNodePtr node) {
    xmc.rsw.physicalMemoryAreasOffset=xmc.noPhysicalMemoryAreas;
    noMemAreaPtr=&xmc.rsw.noPhysicalMemoryAreas; 
    cPartition=-1;
}

static struct nodeXml residentSw_N={BAD_CAST"ResidentSw", ResidentSw_NH0, 0, 0, 0, (struct nodeXml *[]){&memArea_N, 0}};

static void ConsoleH_AH(xmlNodePtr node, const xmlChar *val) {
    xmc.hpv.consoleDev.id=AddDevName((char *)val);
    xmcNoL.hpv.consoleDev=node->line;
}

static struct attrXml consoleH_A={BAD_CAST"console", ConsoleH_AH};

/*
 * Watchdog Node
 */
static void wdInitialValue_AH(xmlNodePtr node, const xmlChar *val)
{
    xmc.hpv.watchdog.wdInitialValue = ToTime((char *)val);
}

static void wdHwReload_AH(xmlNodePtr node, const xmlChar *val)
{
    xmc.hpv.watchdog.hwWdReloadValue = ToTime((char *)val);
}

static struct attrXml wdInitVal_A = {
    .name = BAD_CAST"initialValue",
    .handler = wdInitialValue_AH
};

static struct attrXml wdHwRelVal_A = {
    .name = BAD_CAST"hwReloadValue",
    .handler = wdHwReload_AH
};

static struct nodeXml watchdog_N = {
    BAD_CAST"Watchdog", 0, 0, 0, (struct attrXml *[]){&wdInitVal_A, &wdHwRelVal_A, 0}, 0
};

/*
static void HealthMonitorDevH_AH(xmlNodePtr node, const xmlChar *val) {
    xmc.hpv.hmDev.id=AddDevName((char *)val);
    xmcNoL.hpv.hmDev=node->line;
}

static struct attrXml healthMonitorDevH_A={BAD_CAST"healthMonitorDevice", HealthMonitorDevH_AH};

*/
static void  XmHypervisor_NH0(xmlNodePtr node) {
    xmc.hpv.physicalMemoryAreasOffset=xmc.noPhysicalMemoryAreas;
    noMemAreaPtr=&xmc.hpv.noPhysicalMemoryAreas;
    SetupDefaultHpvHmActions(xmc.hpv.hmTab);
    hmTabPtr=xmc.hpv.hmTab;    
    //tracePtr=&xmc.hpv.trace;
    //traceNoLPtr=&xmcNoL.hpv.trace;
    //tracePtr->dev.id=XM_DEV_INVALID_ID;
    cPartition=-2;
}

static void  XmHypervisor_NH2(xmlNodePtr node) {
    CheckHpvMemAreaFlags();
}

#if defined(CONFIG_MPU) && defined(CONFIG_AT697F)
static struct nodeXml xmHypervisor_N={BAD_CAST"XMHypervisor",  XmHypervisor_NH0, 0, XmHypervisor_NH2, (struct attrXml *[]){&consoleH_A, 0}, (struct nodeXml *[]){&hypMemArea_N, &sharedMemAreas_N, &healthMonitor_N, &watchdog_N 0}};
#else
static struct nodeXml xmHypervisor_N={BAD_CAST"XMHypervisor",  XmHypervisor_NH0, 0, XmHypervisor_NH2, (struct attrXml *[]){&consoleH_A, 0}, (struct nodeXml *[]){&hypMemArea_N, &healthMonitor_N, &watchdog_N, 0}};
#endif

static void  HwDescription_NH2(xmlNodePtr node) {
    //CheckMemBlockReg();	
}

static struct nodeXml hwDescription_N={BAD_CAST"HwDescription", 0, 0, HwDescription_NH2, 0, (struct nodeXml *[]){&processorTable_N, &memoryLayout_N, &devices_N, 0}};

static void VersionSysDesc_AH(xmlNodePtr node, const xmlChar *val) {
    unsigned int version, subversion, revision;

    sscanf((char *)val, "%u.%u.%u", &version, &subversion, &revision);
    xmc.fileVersion=XMC_SET_VERSION(version, subversion, revision);
    xmcNoL.fileVersion=node->line;
}

static struct attrXml versionSysDesc_A={BAD_CAST"version", VersionSysDesc_AH};

static void NameSysDesc_AH(xmlNodePtr node, const xmlChar *val) {
    xmc.nameOffset=AddString((char *)val);
}

static struct attrXml nameSysDesc_A={BAD_CAST"name", NameSysDesc_AH};

static void SystemDescription_NH0(xmlNodePtr node) {
    int e;
    memset(&xmc, 0, sizeof(struct xmc));
    memset(&xmcNoL, 0, sizeof(struct xmcNoL));
    xmc.hpv.consoleDev.id=XM_DEV_INVALID_ID;
    //xmc.hpv.hmDev.id=XM_DEV_INVALID_ID;
//    xmc.hpv.trace.dev.id=XM_DEV_INVALID_ID;
    for (e=0; e<CONFIG_NO_HWIRQS; e++)
	xmc.hpv.hwIrqTab[e].owner=-1;    
}

static void SystemDescription_NH2(xmlNodePtr node) {
    TranslatePartitionId();
    CheckCyclicPlanPartitionId();
    //UnwindCyclicPlans();
    CheckChannelSizes();
    CheckIoPorts();
    LinkDevices();
    LinkChannels2Ports();
#ifdef CONFIG_IPVI_SUPPORT
    CheckIpviTab();
    ProcessIpviTab();
#endif
    SetupHwIrqMask();   
    HmCheckExistMaintenancePlan();
    SortPhysMemAreas();
    SortMemRegions();
    BindVCpu2Cpu();
#if defined(CONFIG_MPU) && defined(CONFIG_AT697F)
    CheckSharedMemoryAreaOverlapping();
#endif
}

static struct nodeXml systemDescription_N={BAD_CAST"SystemDescription", SystemDescription_NH0, 0, SystemDescription_NH2, (struct attrXml *[]){&versionSysDesc_A, &nameSysDesc_A, 0}, (struct nodeXml *[]){&hwDescription_N, &xmHypervisor_N, &residentSw_N, &partitionTable_N, &channels_N, 0}};

struct nodeXml *rootHandlers[]={&systemDescription_N, 0};

//static struct attrXml _A={BAD_CAST"", 0};
//static struct nodeXml _N={BAD_CAST"", 0, 0, (struct attrXml *[]){0}, (struct nodeXml *[]){0}};


