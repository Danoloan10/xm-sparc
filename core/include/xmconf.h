/*
 * $FILE: xmconf.h
 *
 * Config parameters for both, XM and partitions
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
  - [02/09/2015:SPR-020915-09] a compilation error is raised if 
                               CONFIG_MAX_NO_IPVI <= 0
  - [02/09/2015:SPR-280715-01] added "entryPoint" as attribute to partitions, 
                               xmcBootPart removed.
  - [20/11/2015:SPR-110915-01] ipvi stuff depends on IPVI_SUPPORT
*/
#ifndef _XMCONF_H_
#define _XMCONF_H_

#ifdef CONFIG_IPVI_SUPPORT
#if CONFIG_MAX_NO_IPVI <= 0
#error "CONFIG_MAX_NO_IPVI must be greater than 0"
#endif
#endif

#ifdef _XM_KERNEL_
#include <arch/xmconf.h>
#include <devid.h>
#include <linkage.h>
#include <xmef.h>
#else
#include <xm_inc/arch/xmconf.h>
#include <xm_inc/devid.h>
#include <xm_inc/linkage.h>
#include <xm_inc/xmef.h>
#endif

typedef struct {
    xm_u32_t id:16, subId:16;
} xmDev_t;

struct xmcHmSlot {
    xm_u32_t action:31, log:1;
  
// Logging
#define XM_HM_LOG_DISABLED 0
#define XM_HM_LOG_ENABLED 1

// Actions
//@% <track id="hm-action-list"> 
#define XM_HM_AC_IGNORE 0
#define XM_HM_AC_PARTITION_COLD_RESET 1
#define XM_HM_AC_PARTITION_WARM_RESET 2
#define XM_HM_AC_HYPERVISOR_COLD_RESET 3
#define XM_HM_AC_HYPERVISOR_WARM_RESET 4
#define XM_HM_AC_SUSPEND 5
#define XM_HM_AC_HALT 6
#define XM_HM_AC_PROPAGATE 7
#define XM_HM_AC_SWITCH_TO_MAINTENANCE 8
//@% </track id="hm-action-list"> */
#define XM_HM_MAX_ACTIONS 9
};

// Events
//@% <track id="hm-ev-xm-triggered">
#define XM_HM_EV_INTERNAL_ERROR 0
  #define XM_HM_MODID_SW_ERROR 0
  #define XM_HM_MODID_HWTIMER_ERROR 1

#define XM_HM_EV_UNEXPECTED_TRAP 1
#define XM_HM_EV_PARTITION_UNRECOVERABLE 2
#define XM_HM_EV_PARTITION_ERROR 3
#define XM_HM_EV_PARTITION_INTEGRITY 4
#define XM_HM_EV_MEM_PROTECTION 5
#define XM_HM_EV_OVERRUN 6
#define XM_HM_EV_SCHED_ERROR 7
#define XM_HM_EV_WATCHDOG_TIMER 8
#define XM_HM_EV_INCOMPATIBLE_INTERFACE 9
#define XM_HM_EV_EXTSYNC_ERROR 10
//@ </track id="hm-ev-xm-triggered">

#define XM_HM_MAX_GENERIC_EVENTS 11

//@ <track id="test-app-events"> 
#define XM_HM_EV_APP_DEADLINE_MISSED (XM_HM_MAX_GENERIC_EVENTS+XM_HM_MAX_ARCH_EVENTS+0)
#define XM_HM_EV_APP_APPLICATION_ERROR (XM_HM_MAX_GENERIC_EVENTS+XM_HM_MAX_ARCH_EVENTS+1)
#define XM_HM_EV_APP_NUMERIC_ERROR (XM_HM_MAX_GENERIC_EVENTS+XM_HM_MAX_ARCH_EVENTS+2)
#define XM_HM_EV_APP_ILLEGAL_REQUEST (XM_HM_MAX_GENERIC_EVENTS+XM_HM_MAX_ARCH_EVENTS+3)
#define XM_HM_EV_APP_STACK_OVERFLOW (XM_HM_MAX_GENERIC_EVENTS+XM_HM_MAX_ARCH_EVENTS+4)
#define XM_HM_EV_APP_MEMORY_VIOLATION (XM_HM_MAX_GENERIC_EVENTS+XM_HM_MAX_ARCH_EVENTS+5)
#define XM_HM_EV_APP_HARDWARE_FAULT (XM_HM_MAX_GENERIC_EVENTS+XM_HM_MAX_ARCH_EVENTS+6)
#define XM_HM_EV_APP_POWER_FAIL (XM_HM_MAX_GENERIC_EVENTS+XM_HM_MAX_ARCH_EVENTS+7)
//@ </track id="test-app-events"> 

#define XM_HM_MAX_APP_EVENTS 8


struct xmcCommPort {
    xm_u32_t nameOffset;
    xm_s32_t channelId;
#define XM_NULL_CHANNEL -1
    xm_s32_t direction;
#define XM_SOURCE_PORT 0x2
#define XM_DESTINATION_PORT 0x1
    xm_s32_t type;
#define XM_SAMPLING_PORT 0
#define XM_QUEUING_PORT 1
    xm_u32_t flags;
#define XM_FIFO_PORT 0
#define XM_PRIORITY_PORT 1
#define XM_EXTERNAL_PORT 2
    xm_u32_t cpuId;
};

//@% <track id="sched-cyclic-slot"> 
struct xmcSchedCyclicSlot {
    xmId_t id;
    xmId_t partitionId;
    xmId_t vCpuId;
    xm_u32_t sExec; // offset (usec)
    xm_u32_t eExec; // offset+duration (usec)
    xm_u32_t flags;
#define XM_PERIOD_START_SLOT 0x1
};
//@% </track id="sched-cyclic-slot"> 

struct xmcSchedCyclicPlan {
    xm_u32_t nameOffset;
    xmId_t id;
    xm_u32_t majorFrame; // in useconds   
    xm_s32_t noSlots;
#ifdef CONFIG_PLAN_EXTSYNC
    xm_s32_t extSync; // -1 means no sync
    xm_s32_t interval; // sync signal timeout
#endif
    xm_u32_t slotsOffset;
};

//@% <track id="doc-xmc-memory-area">
struct xmcMemoryArea {
    xm_u32_t nameOffset;
    xmAddress_t startAddr;
    xmAddress_t mappedAt;
    xmSize_t size;
#define XM_MEM_AREA_UNMAPPED (1<<0)
#define XM_MEM_AREA_READONLY (1<<1)
#define XM_MEM_AREA_UNCACHEABLE (1<<2)
#define XM_MEM_AREA_ROM (1<<3)
#define XM_MEM_AREA_FLAG0 (1<<4)
#define XM_MEM_AREA_FLAG1 (1<<5)
#define XM_MEM_AREA_FLAG2 (1<<6)
#define XM_MEM_AREA_FLAG3 (1<<7)
#define XM_MEM_AREA_TAGGED (1<<8)
    xm_u32_t flags;
    xm_u32_t memoryRegionOffset;
};
//@% </track id="doc-xmc-memory-area">

#if defined(CONFIG_MPU) && defined(CONFIG_AT697F)
struct xmcSharedMemoryArea {
    xmAddress_t startAddr;
    xmSize_t size;
};
#endif

struct xmcRsw {
    xm_s32_t noPhysicalMemoryAreas;    
    xm_u32_t physicalMemoryAreasOffset;
    xmAddress_t entryPoint;
};

struct xmcPartition {
    xmId_t id, rId;
    xm_u32_t nameOffset;
    xm_u32_t flags;
#define XM_PART_BOOT 0x1
#define XM_PART_SYSTEM 0x100
#define XM_PART_FP 0x200
#define XM_PART_ICACHE_DISABLED 0x400
#define XM_PART_DCACHE_DISABLED 0x800
    xmAddress_t entryPoint;
    xm_u32_t hwIrqs;
    xm_s32_t noPhysicalMemoryAreas;
    xm_u32_t physicalMemoryAreasOffset;
    xmDev_t consoleDev;
    struct xmcTemporalRestrictions {
	xm_u32_t period;
	xm_u32_t duration;
    } temporalRestrictions;
    xm_u32_t commPortsOffset;
    xm_s32_t noPorts;
    struct xmcHmSlot hmTab[XM_HM_MAX_EVENTS];
    xm_u32_t ioPortsOffset;
    xm_s32_t noIoPorts;
    //struct xmcTrace trace;
#ifdef CONFIG_IPVI_SUPPORT
    struct xmcPartIpvi {
        xm_u32_t dstOffset;
        xm_s32_t noDsts;
    } ipviTab[CONFIG_MAX_NO_IPVI];
#endif
};

/* <track id="test-channel-struct"> */
struct xmcCommChannel {
#define XM_SAMPLING_CHANNEL 0
#define XM_QUEUING_CHANNEL 1
    xm_s32_t type;
  
    union {
	struct {
	    xm_s32_t maxLength;
	    xm_s32_t maxNoMsgs;
            xm_u32_t maxTimeExpiration;
            xmAddress_t address;
            xmSize_t size;
	} q;
	struct {
	    xm_s32_t maxLength;
	    xm_u32_t refreshPeriod;
            xm_s32_t noReceivers;
            xmAddress_t address;
            xmSize_t size;
	} s;
    };
};
/* </track id="test-channel-struct"> */

struct xmcMemoryRegion {
    xmAddress_t startAddr;
    xmSize_t size;
#define XMC_REG_FLAG_PGTAB (1<<0)
#define XMC_REG_FLAG_ROM (1<<1)
    xm_u32_t flags;
};

struct xmcHwIrq {
    xm_s32_t owner;
#define XM_IRQ_NO_OWNER -1
};

struct xmcHpv
{
    xm_s32_t noPhysicalMemoryAreas;
    xm_u32_t physicalMemoryAreasOffset;
#if defined(CONFIG_MPU) && defined(CONFIG_AT697F)
    xm_u32_t sharedMemoryAreasOffset;
    xm_s32_t noSharedMemoryAreas;
#endif
    xm_s32_t noCpus;
    struct cpu
    {
        xmId_t id;
        xm_u32_t features; // Enable/disable features
        xm_u32_t freq; // KHz
#define XM_CPUFREQ_AUTO 0
        xm_u32_t schedCyclicPlansOffset;
        xm_s32_t noSchedCyclicPlans;
        xmDev_t consoleDev;
    } cpuTab[CONFIG_NO_CPUS];
//    struct {
//        xmDev_t dev;
//        xm_u32_t offset;
//    } container;
    struct xmcHmSlot hmTab[XM_HM_MAX_GENERIC_EVENTS + XM_HM_MAX_ARCH_EVENTS];
    /*
     xmDev_t hmDev;
     #ifdef CONFIG_APP_EVENT
     xmDev_t hmAppDev;
     #endif
     */
    xmDev_t consoleDev;
    struct xmcHwIrq hwIrqTab[CONFIG_NO_HWIRQS];
    //struct xmcTrace trace;
    struct
    {
        xm_u32_t wdInitialValue;
        xm_u32_t hwWdReloadValue;
    } watchdog;
};

#if defined(CONFIG_DEV_MEMBLOCK)||defined(CONFIG_DEV_MEMBLOCK_MODULE)
struct xmcMemBlock {
    xm_u32_t physicalMemoryAreasOffset;
};
#endif

struct xmcDevice {
#if defined(CONFIG_DEV_MEMBLOCK)||defined(CONFIG_DEV_MEMBLOCK_MODULE)
    xmAddress_t memBlocksOffset;
    xm_s32_t noMemBlocks;
#endif
#if defined(CONFIG_DEV_UART)||defined(CONFIG_DEV_UART_MODULE)
    struct xmcUartCfg {
	xm_u32_t baudRate;
    } uart[CONFIG_DEV_NO_UARTS];
#endif
#ifdef CONFIG_DEV_VGA
    struct xmcVgaCfg {
	
    } vga;
#endif
};

struct xmcRsvMem {
    void *obj;
    xm_u32_t usedAlign;
#define RSV_MEM_USED 0x80000000
    xm_u32_t size;
} __PACKED;

struct xmcVCpu {
    xmId_t cpu;
};

#define XMC_VERSION 1
#define XMC_SUBVERSION 0
#define XMC_REVISION 0

struct xmc {
#define XMC_SIGNATURE 0x24584d43 // $XMC
    xm_u32_t signature;
    xm_u8_t digest[XM_DIGEST_BYTES];
    xmSize_t dataSize;
    xmSize_t size;
// Reserved(8).VERSION(8).SUBVERSION(8).REVISION(8)
#define XMC_SET_VERSION(_ver, _subver, _rev) ((((_ver)&0xFF)<<16)|(((_subver)&0xFF)<<8)|((_rev)&0xFF))
#define XMC_GET_VERSION(_v) (((_v)>>16)&0xFF)
#define XMC_GET_SUBVERSION(_v) (((_v)>>8)&0xFF)
#define XMC_GET_REVISION(_v) ((_v)&0xFF)
    xm_u32_t version;
    xm_u32_t fileVersion;
    xmAddress_t rsvMemTabOffset;
    xmAddress_t nameOffset;
    struct xmcHpv hpv;
    struct xmcRsw rsw;
    xmAddress_t partitionTabOffset;
    xm_s32_t noPartitions;
    xmAddress_t memoryRegionsOffset;
    xm_u32_t noRegions;    
    xmAddress_t schedCyclicSlotsOffset;
    xm_s32_t noSchedCyclicSlots;
    xmAddress_t schedCyclicPlansOffset;
    xm_s32_t noSchedCyclicPlans;
    xmAddress_t commChannelTabOffset;
    xm_s32_t noCommChannels;
    xmAddress_t physicalMemoryAreasOffset;
    xm_s32_t noPhysicalMemoryAreas;
#if defined(CONFIG_MPU) && defined(CONFIG_AT697F)
    xmAddress_t sharedMemoryAreasOffset;
    xm_u32_t noSharedMemoryAreas;
#endif    
    xmAddress_t commPortsOffset;
    xm_s32_t noCommPorts;
    xmAddress_t ioPortsOffset;
    xm_s32_t noIoPorts;
#ifdef CONFIG_IPVI_SUPPORT
    xmAddress_t ipviDstOffset;
    xm_s32_t noIpviDsts;
#endif
    xmAddress_t vCpuTabOffset;
    xmAddress_t stringsOffset;
    xm_s32_t stringTabLength;
    struct xmcDevice deviceTab;
} __PACKED;

#ifdef _XM_KERNEL_
extern const struct xmc xmcTab;
extern struct xmcPartition *xmcPartitionTab;
extern struct xmcMemoryRegion *xmcMemRegTab;
extern struct xmcCommChannel *xmcCommChannelTab;
extern struct xmcMemoryArea *xmcPhysMemAreaTab;
#if defined(CONFIG_MPU) && defined(CONFIG_AT697F)
extern struct xmcSharedMemoryArea *xmcSharedMemAreaTab;
#endif
extern struct xmcCommPort *xmcCommPorts;
extern struct xmcIoPort *xmcIoPortTab;
extern struct xmcSchedCyclicSlot *xmcSchedCyclicSlotTab;
extern struct xmcSchedCyclicPlan *xmcSchedCyclicPlanTab;
extern struct xmcVCpu *xmcVCpuTab;
#ifdef CONFIG_IPVI_SUPPORT
extern xm_u8_t *xmcDstIpvi;
#endif
extern xm_s8_t *xmcStringTab;
extern struct xmcRsvMem *xmcRsvMemTab;
#if defined(CONFIG_DEV_MEMBLOCK)||defined(CONFIG_DEV_MEMBLOCK_MODULE)
extern struct xmcMemBlock *xmcMemBlockTab;
#endif
#endif

#endif
