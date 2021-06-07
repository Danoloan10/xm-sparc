/*
 * $FILE: xmc.c
 *
 * xmc implementation
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
  - [20/11/2015:SPR-110915-01] ipvi stuff depends on IPVI_SUPPORT
*/
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "limits.h"
#include "common.h"
#include "parser.h"
#include "xmc.h"

struct xmc xmc;
struct xmcNoL xmcNoL;
struct xmcMemoryRegion *xmcMemRegTab=0;
struct xmcMemoryRegionNoL *xmcMemRegTabNoL=0;
struct xmcMemoryArea *xmcMemAreaTab=0;
struct xmcMemoryAreaNoL *xmcMemAreaTabNoL=0;
#if defined(CONFIG_MPU) && defined(CONFIG_AT697F)
struct xmcSharedMemoryArea *xmcSharedMemAreaTab=0;
struct xmcSharedMemoryAreaNoL *xmcSharedMemAreaTabNoL;
#endif
struct xmcSchedCyclicSlot *xmcSchedCyclicSlotTab=0;
struct xmcSchedCyclicSlotNoL *xmcSchedCyclicSlotTabNoL=0;
struct xmcPartition *xmcPartitionTab=0;
struct xmcPartitionNoL *xmcPartitionTabNoL=0;
struct xmcIoPort *xmcIoPortTab=0;
struct xmcIoPortNoL *xmcIoPortTabNoL=0;
struct xmcCommPort *xmcCommPortTab=0;
struct xmcCommPortNoL *xmcCommPortTabNoL=0;
struct xmcCommChannel *xmcCommChannelTab=0;
struct xmcCommChannelNoL *xmcCommChannelTabNoL=0;
struct xmcMemBlock *xmcMemBlockTab=0;
struct xmcMemBlockNoL *xmcMemBlockTabNoL=0;
struct ipcPort *ipcPortTab=0;
struct ipcPortNoL *ipcPortTabNoL=0;
struct xmcSchedCyclicPlan *xmcSchedCyclicPlanTab=0;
struct xmcSchedCyclicPlanNoL *xmcSchedCyclicPlanTabNoL=0;
#ifdef CONFIG_IPVI_SUPPORT
struct srcIpvi *srcIpviTab=0;
struct srcIpviNoL *srcIpviTabNoL=0;
int noSrcIpvi=0;
char *ipviDstTab=0;
#endif
int noIpcPorts=0;

char *strTab=NULL;

static struct devTab {
    char *name;
    xmDev_t id;
    int line;
} *devTab=0;

static int devTabLen=0;

xmDev_t LookUpDev(char *name, int line) {
    int e;
    for (e=0; e<devTabLen; e++)
	if (!strcmp(name, devTab[e].name))
	    return devTab[e].id;
    LineError(line, "\"%s\" not found in the device table", name);
    return (xmDev_t){.id=XM_DEV_INVALID_ID, .subId=0};
}

void RegisterDev(char *name, xmDev_t id, int line) {
    int e;
    for (e=0; e<devTabLen; e++)
	if (!strcmp(name, devTab[e].name)) 
	    LineError(line, "device name already registered (line %d)", devTab[e].line);
    
    e=devTabLen++;
    DO_REALLOC(devTab, devTabLen*sizeof(struct devTab));
    devTab[e].name=strdup(name);
    devTab[e].id=id;
    devTab[e].line=line;
}

static char **devNameTab=0;
static int devNameTabLen=0;

int AddDevName(char *name) {
    int e=devNameTabLen++;
    DO_REALLOC(devNameTab, sizeof(char *)*devNameTabLen);
    devNameTab[e]=strdup(name);
    return e;
}

void LinkDevices(void) {
#define IS_DEFINED(_x) (((_x).id!=XM_DEV_INVALID_ID)&&((_x).id<devNameTabLen))
    int e;
    if (IS_DEFINED(xmc.hpv.consoleDev))
	xmc.hpv.consoleDev=LookUpDev(devNameTab[xmc.hpv.consoleDev.id], xmcNoL.hpv.consoleDev);
    /*if (IS_DEFINED(xmc.hpv.trace.dev))
	xmc.hpv.trace.dev=LookUpDev(devNameTab[xmc.hpv.trace.dev.id], xmcNoL.hpv.trace.dev);
    if (IS_DEFINED(xmc.hpv.hmDev))
	xmc.hpv.hmDev=LookUpDev(devNameTab[xmc.hpv.hmDev.id], xmcNoL.hpv.hmDev);
    */
    for (e=0; e<xmc.hpv.noCpus; e++)
        if (IS_DEFINED(xmc.hpv.cpuTab[e].consoleDev)) {
            xmc.hpv.cpuTab[e].consoleDev=LookUpDev(devNameTab[xmc.hpv.cpuTab[e].consoleDev.id], xmcNoL.hpv.cpuTab[e].consoleDev);
        } else {
            xmc.hpv.cpuTab[e].consoleDev=xmc.hpv.consoleDev;
        }

    for (e=0; e<xmc.noPartitions; e++) {
	if (IS_DEFINED(xmcPartitionTab[e].consoleDev))
	    xmcPartitionTab[e].consoleDev=LookUpDev(devNameTab[xmcPartitionTab[e].consoleDev.id], xmcPartitionTabNoL[e].consoleDev);
	
	/*if (IS_DEFINED(xmcPartitionTab[e].trace.dev))
          xmcPartitionTab[e].trace.dev=LookUpDev(devNameTab[xmcPartitionTab[e].trace.dev.id], xmcPartitionTabNoL[e].trace.dev);*/
    }
}

xm_u32_t AddString(char *s) {
    xm_u32_t offset;
    offset=xmc.stringTabLength;
    xmc.stringTabLength+=strlen(s)+1;
    DO_REALLOC(strTab, xmc.stringTabLength*sizeof(char));
    strcpy(&strTab[offset], s);
    return offset;
}

xmId_t PartitionId2PId(xmId_t id) {
    int e;
    for (e=0; e<xmc.noPartitions; e++) {
        if (xmcPartitionTab[e].rId==id)
            return e;
    }
    return xmc.noPartitions;
}

void TranslatePartitionId(void) {
    int i;
#ifdef CONFIG_IPVI_SUPPORT
    int j;
#endif
    
    for (i=0; i<noIpcPorts; i++)
        ipcPortTab[i].partitionId=PartitionId2PId(ipcPortTab[i].partitionId);
    
    for (i=0; i<xmc.noSchedCyclicSlots; i++)
        xmcSchedCyclicSlotTab[i].partitionId=PartitionId2PId(xmcSchedCyclicSlotTab[i].partitionId);
#ifdef CONFIG_IPVI_SUPPORT
    for (i=0; i<noSrcIpvi; i++) {
        srcIpviTab[i].id=PartitionId2PId(srcIpviTab[i].id);
        for (j=0; j<srcIpviTab[i].noDsts; j++)
            srcIpviTab[i].dst[j].id=PartitionId2PId(srcIpviTab[i].dst[j].id);
    }
#endif
}

void LinkChannels2Ports(void) {
    struct xmcCommPort *port=0;
    int e, i;
    for (i=0; i<noIpcPorts; i++) {
        if (ipcPortTab[i].flags&XM_EXTERNAL_PORT) {
            DO_REALLOC(xmcCommPortTab, (xmc.noCommPorts+1)*sizeof(struct xmcCommPort));
            port=&xmcCommPortTab[xmc.noCommPorts];
            //port->nameOffset=ipcPortTab[i].nameOffset;
            port->channelId=ipcPortTab[i].channel;
            port->direction=ipcPortTab[i].direction;
            port->type=xmcCommChannelTab[ipcPortTab[i].channel].type;
            port->flags=ipcPortTab[i].flags;
            port->cpuId=ipcPortTab[i].cpuId;
            xmc.noCommPorts++;
            if ((xmcCommChannelTab[port->channelId].type==XM_SAMPLING_CHANNEL)&&(port->direction==XM_DESTINATION_PORT))
                xmcCommChannelTab[port->channelId].s.noReceivers++;
            continue;
        }

	if (ipcPortTab[i].partitionId>=xmc.noPartitions)
	    LineError(ipcPortTabNoL[i].partitionId, "incorrect partition id (%d)", ipcPortTab[i].partitionId);	
	if (ipcPortTab[i].partitionName&&strcmp(ipcPortTab[i].partitionName, &strTab[xmcPartitionTab[ipcPortTab[i].partitionId].nameOffset]))
	    LineError(ipcPortTabNoL[i].partitionName, "partition name \"%s\" mismatches with the expected one \"%s\"", ipcPortTab[i].partitionName, &strTab[xmcPartitionTab[ipcPortTab[i].partitionId].nameOffset]);
	for (e=0; e<xmcPartitionTab[ipcPortTab[i].partitionId].noPorts; e++) {
	    port=&xmcCommPortTab[xmcPartitionTab[ipcPortTab[i].partitionId].commPortsOffset+e];
	    if (!strcmp(ipcPortTab[i].portName, &strTab[port->nameOffset])) {
		port->channelId=ipcPortTab[i].channel;
                if ((xmcCommChannelTab[port->channelId].type==XM_SAMPLING_CHANNEL)&&(port->direction==XM_DESTINATION_PORT))
                    xmcCommChannelTab[port->channelId].s.noReceivers++;
		break;
	    }
	}
	if (e>=xmcPartitionTab[ipcPortTab[i].partitionId].noPorts)
	    LineError(ipcPortTabNoL[i].portName, "port \"%s\" not found", ipcPortTab[i].portName);
	if (xmcCommChannelTab[ipcPortTab[i].channel].type!=port->type)
	    LineError(xmcCommChannelTabNoL[ipcPortTab[i].channel].type, "channel type mismatches with the type of the port");
	if (ipcPortTab[i].direction!=port->direction)
	    LineError(ipcPortTabNoL[i].direction, "channel direction mismatches with the direction of the port");
    }    
}

void SetupHwIrqMask(void) {
    int e, i;
    for (e=0; e<xmc.noPartitions; e++) {
        for (i=0; i<CONFIG_NO_HWIRQS; i++) {
            if (xmc.hpv.hwIrqTab[i].owner==xmcPartitionTab[e].id)
                xmcPartitionTab[e].hwIrqs|=(1<<i);
        }
    }
        
}

static int CmpMemArea(struct xmcMemoryArea *m1, struct xmcMemoryArea *m2) {
    if (m1->startAddr>m2->startAddr) return 1;
    if (m1->startAddr<m2->startAddr) return -1;
    return 0;
}

void SortPhysMemAreas(void) {
    xm_s32_t e;
    for (e=0; e<xmc.noPartitions; e++)
        qsort(&xmcMemAreaTab[xmcPartitionTab[e].physicalMemoryAreasOffset], xmcPartitionTab[e].noPhysicalMemoryAreas, sizeof(struct xmcMemoryArea), (int(*)(const void *, const void *))CmpMemArea);
}

static int CmpMemRegions(struct xmcMemoryRegion *m1, struct xmcMemoryRegion *m2) {
    if (m1->startAddr>m2->startAddr) return 1;
    if (m1->startAddr<m2->startAddr) return -1;
    return 0;
}

void SortMemRegions(void) {
    qsort(xmcMemRegTab, xmc.noRegions, sizeof(struct xmcMemoryRegion), (int(*)(const void *, const void *))CmpMemRegions);
}

#ifdef CONFIG_IPVI_SUPPORT
void ProcessIpviTab(void) {
    int e, i, j;
    struct srcIpvi *src;
    struct dstIpvi *dst;

    for (e=0; e<xmc.noPartitions; e++) {
        memset(&xmcPartitionTab[e].ipviTab, 0,  sizeof(struct xmcPartIpvi)*CONFIG_MAX_NO_IPVI);
        for (src=0, i=0; i<noSrcIpvi; i++){
            src=&srcIpviTab[i];
            if (xmcPartitionTab[e].id==src->id) {
                xmcPartitionTab[e].ipviTab[src->ipviId].dstOffset=xmc.noIpviDsts;
                for (j=0; j<src->noDsts; j++) {
                    dst=&src->dst[j];
                    xmc.noIpviDsts++;
                    xmcPartitionTab[e].ipviTab[src->ipviId].noDsts++;
                    DO_REALLOC(ipviDstTab, xmc.noIpviDsts);
                    ipviDstTab[xmc.noIpviDsts-1]=dst->id;
                }
            }
        }
    }      
}
#endif
void BindVCpu2Cpu(void) {
    struct xmcSchedCyclicPlan *plan;
    struct xmcSchedCyclicSlot *slot;
    int e, i, j;

    vCpuTab=malloc(xmc.noPartitions*sizeof(struct vCpu2Cpu *));   
    for (e=0; e<xmc.noPartitions; e++) {
        vCpuTab[e]=malloc(CONFIG_NO_VCPUS*sizeof(struct vCpu2Cpu));
        memset(vCpuTab[e], -1, CONFIG_NO_VCPUS*sizeof(struct vCpu2Cpu));
    }
    for (e=0; e<xmc.hpv.noCpus; e++) {
        for (i=0; i<xmc.hpv.cpuTab[e].noSchedCyclicPlans; i++) {
            plan=&xmcSchedCyclicPlanTab[i+xmc.hpv.cpuTab[e].schedCyclicPlansOffset];
            for (j=0; j<plan->noSlots; j++) {
                slot=&xmcSchedCyclicSlotTab[j+plan->slotsOffset];
                if (vCpuTab[slot->partitionId][slot->vCpuId].cpu==-1) {
                    vCpuTab[slot->partitionId][slot->vCpuId].cpu=e;
                    vCpuTab[slot->partitionId][slot->vCpuId].line=xmcSchedCyclicSlotTabNoL[j+plan->slotsOffset].vCpuId;
                    continue;
                }
                /*if (vCpuTab[slot->partitionId][slot->vCpuId].cpu!=e)
                  LineError(xmcSchedCyclicSlotTabNoL[j+plan->slotsOffset].vCpuId, "vCpu (%d, %d) assigned to CPU %d and %d (line %d)", slot->partitionId, slot->vCpuId, vCpuTab[slot->partitionId][slot->vCpuId].cpu, e, vCpuTab[slot->partitionId][slot->vCpuId].line);*/
            }
        }
    }
}
