/*
 * $FILE: xmc.h
 *
 * XMC definitions
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
*/

#ifndef _XMCDEF_H_
#define _XMCDEF_H_

#include <xm_inc/xmconf.h>

struct xmcHmSlotNoL {
    int action;
    int log;
};
/*
struct xmcTraceNoL {
    int dev;
};
*/
struct xmcMemoryRegionNoL {
    int line;
    int startAddr;
    int size;
    int flags;
};

#if defined(CONFIG_MPU) && defined(CONFIG_AT697F)
struct xmcSharedMemoryAreaNoL {
    int line;
    int startAddr;
    int size;
};
#endif

struct xmcMemoryAreaNoL {
    int name;
    int line;
    int startAddr;
    int mappedAt;
    int size;
    int flags;
    int partitionId;
};

#if defined(CONFIG_SPARCv8)
struct xmcIoPortNoL {
    union {
	struct {
	    int base;
	    int noPorts;
	} range;
	struct  {
	    int address;
	    int mask;
	} restricted;
    };
};
#endif

struct xmcSchedCyclicSlotNoL {
    int id;
    int partitionId;
    int vCpuId;
    int sExec;
    int eExec;
    int flags;
};

struct xmcPartitionNoL {
    int id;
    int name;
    int flags;
    int entryPoint;
//    xm_s32_t noPhysicalMemoryAreas;
//    xm_u32_t physicalMemoryAreasOffset;
    int consoleDev;
    struct {
	int period;
	int duration;
    } temporalRestrictions;
//    struct xmcPartitionArch arch;
    //   xm_u32_t commPortsOffset;
    //   xm_s32_t noPorts;
    struct xmcHmSlotNoL hmTab[XM_HM_MAX_EVENTS];
    //  xm_u32_t ioPortsOffset;
    //  xm_s32_t noIoPorts;
    //struct xmcTraceNoL trace;
};

struct xmcSchedCyclicPlanNoL {
    int name;
    int majorFrame; // in useconds
    int extSync;
    int interval;
};

struct xmcNoL {
    int fileVersion;
    struct {
	struct {
	    int id;
	    int features; // Enable/disable features
	    int freq; // KHz
            int consoleDev;
	    /*union {
		struct {
		    struct xmcSchedCyclicPlanNoL {
			int majorFrame; // in useconds
			//int slotsOffset;
		    } planTab[CONFIG_MAX_CYCLIC_PLANS];
		} cyclic;
                } schedParams;*/
	} cpuTab[CONFIG_NO_CPUS];
	struct xmcHmSlotNoL hmTab[XM_HM_MAX_GENERIC_EVENTS+XM_HM_MAX_ARCH_EVENTS];
//	int containerDev;
/*        int hmDev;
*/
	int consoleDev;	
	int hwIrqTab[CONFIG_NO_HWIRQS];  
	//struct xmcTraceNoL trace;
    } hpv;
    struct {
	//int consoleDev;
	int entryPoint;
    } rsw;
};

struct xmcMemBlockNoL {
    int line;
    int startAddr;
    int size;
};

struct xmcCommChannelNoL {
    int type;
    union {
	struct {
	    int maxLength;
	    int maxNoMsgs;
            int address;
            int size;
	} q;
	struct {
	    int maxLength;
            int address;
            int size;
	} s;
    };
    int refreshPeriod;
};

struct xmcCommPortNoL {
    int name;
    int direction;
    int type;
};

struct ipcPort {
    int channel;
    char *partitionName;
    char *portName;
    xm_u32_t partitionId;
    int direction;
    int flags;
    int cpuId;
};

struct ipcPortNoL {
    int partitionName;
    int portName;
    int partitionId;
    int direction;
};

#ifdef CONFIG_IPVI_SUPPORT
struct srcIpvi {
    int ipviId;
    int id;
    struct dstIpvi {
        int id;
    } *dst;
    int noDsts;
};

struct srcIpviNoL {
    int id;
    struct dstIpviNoL {
        int id;
    } *dst;
};

extern char *ipviDstTab;

extern struct srcIpviNoL *srcIpviTabNoL;
extern struct srcIpvi *srcIpviTab;
extern int noSrcIpvi;
#endif

extern struct xmc xmc;
extern struct xmcNoL xmcNoL;

extern struct xmcMemoryRegion *xmcMemRegTab;
extern struct xmcMemoryRegionNoL *xmcMemRegTabNoL;

extern struct xmcMemoryArea *xmcMemAreaTab;
extern struct xmcMemoryAreaNoL *xmcMemAreaTabNoL;

#if defined(CONFIG_MPU) && defined(CONFIG_AT697F)
extern struct xmcSharedMemoryArea *xmcSharedMemAreaTab;
extern struct xmcSharedMemoryAreaNoL *xmcSharedMemAreaTabNoL;
#endif

extern struct xmcSchedCyclicSlot *xmcSchedCyclicSlotTab;
extern struct xmcSchedCyclicSlotNoL *xmcSchedCyclicSlotTabNoL;

extern struct xmcPartition *xmcPartitionTab;
extern struct xmcPartitionNoL *xmcPartitionTabNoL;

extern struct xmcIoPort *xmcIoPortTab;
extern struct xmcIoPortNoL *xmcIoPortTabNoL;

extern struct xmcCommPort *xmcCommPortTab;
extern struct xmcCommPortNoL *xmcCommPortTabNoL;
extern struct xmcCommChannel *xmcCommChannelTab;
extern struct xmcCommChannelNoL *xmcCommChannelTabNoL;
extern struct ipcPort *ipcPortTab;
extern struct ipcPortNoL *ipcPortTabNoL;
extern int noIpcPorts;
//extern struct hmSlot hmSlot;
extern struct xmcMemBlock *xmcMemBlockTab;
extern struct xmcMemBlockNoL *xmcMemBlockTabNoL;

extern struct xmcSchedCyclicPlan *xmcSchedCyclicPlanTab;
extern struct xmcSchedCyclicPlanNoL *xmcSchedCyclicPlanTabNoL;

extern char *strTab;

#define C_SHLIB (xmc.noSharedLibs-1)
#define C_PARTSHLIB (xmc.noPartShLibs-1)
#define C_PARTITION (xmc.noPartitions-1)
#define C_COMM_CHANNEL (xmc.noCommChannels-1)
#define C_HPV_PHYSMEMAREA (xmc.hpv.noPhysicalMemoryAreas-1)
#define C_PART_PHYSMEMAREA (partitionTab[C_PARTITION].noPhysicalMemoryAreas-1)
#define C_RSW_PHYSMEMAREA (xmc.rsw.noPhysicalMemoryAreas-1)
#define C_PART_VIRTMEMAREA (partitionTab[C_PARTITION].noVirtualMemoryAreas-1)
#define C_PART_IOPORT (partitionTab[C_PARTITION].noIoPorts-1)
#define C_PART_COMMPORT (partitionTab[C_PARTITION].noPorts-1)
#define C_CPU (xmc.hpv.noCpus-1)
#define C_REGION (xmc.noRegions-1)

#if defined(CONFIG_MPU) && defined(CONFIG_AT697F)
#define C_SHAREDMEMAREA (xmc.hpv.noSharedMemoryAreas-1)
#endif

#define C_PHYSMEMAREA (xmc.noPhysicalMemoryAreas-1)
#define C_COMMPORT (xmc.noCommPorts-1)
#define C_IOPORT (xmc.noIoPorts-1)
#define C_CYCLICSLOT (xmc.noSchedCyclicSlots-1)
#define C_CYCLICPLAN (xmc.noSchedCyclicPlans-1)
#define C_MEMORYBLOCK_DEV (xmc.deviceTab.noMemBlocks-1)
#define C_IPCPORT (noIpcPorts-1)
//#define C_HPV_PLAN_SLOT (xmc.hpv.cpuTab[C_CPU].schedParams.cyclic.planTab[C_HPV_SCHED_PLAN].noSlots-1)

extern xm_u32_t AddString(char *s);
extern xmDev_t LookUpDev(char *name, int line);
extern void RegisterDev(char *name, xmDev_t id, int line);
extern int AddDevName(char *name);
extern void LinkChannels2Ports(void);
extern void LinkDevices(void);

extern void SetupDefaultHpvHmActions(struct xmcHmSlot *hmTab);
extern void SetupDefaultPartHmActions(struct xmcHmSlot *hmTab);
extern void SortPhysMemAreas(void);
extern void SortMemRegions(void);
extern void ProcessIpviTab(void);
extern void TranslatePartitionId(void);
extern void BindVCpu2Cpu(void);

#endif
