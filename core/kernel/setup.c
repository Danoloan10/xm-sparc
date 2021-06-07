/*
 * $FILE: setup.c
 *
 * Setting up and starting up the kernel
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
  Changelog:
  - [02/09/2015:SPR-020915-04] Remove VBOOT macro and boot.h
  - [02/09/2015:SPR-280715-01] xmcBootTab removed
  - [20/11/2015:SPR-110915-01] ipvi stuff depends on IPVI_SUPPORT
  - [24/11/2015:SPR-241115-02] ResetSystem requires a hmLog and calls an 
       external function if required by the hypervisor configuration.
*/

#include <assert.h>
#include <comp.h>
#include <rsvmem.h>
#include <kdevice.h>
#include <ktimer.h>
#include <stdc.h>
#include <irqs.h>
#include <objdir.h>
#include <physmm.h>
#include <processor.h>
#include <sched.h>
#include <kthread.h>
#include <vmmap.h>
#include <virtmm.h>
#include <xmconf.h>
#include <kwatchdog.h>

#include <local.h>

#include <objects/status.h>
#include <objects/console.h>
#include <arch/paging.h>
#include <arch/xm_def.h>


// CPU's frequency
xm_u32_t cpuKhz;
xm_u16_t __nrCpus=0;

struct xmcPartition *xmcPartitionTab;
struct xmcMemoryRegion *xmcMemRegTab;
struct xmcMemoryArea *xmcPhysMemAreaTab;
#if defined(CONFIG_MPU) && defined(CONFIG_AT697F)
struct xmcSharedMemoryArea *xmcSharedMemAreaTab;
#endif
struct xmcCommChannel *xmcCommChannelTab;
struct xmcCommPort *xmcCommPorts;
struct xmcIoPort *xmcIoPortTab;
struct xmcSchedCyclicSlot *xmcSchedCyclicSlotTab;
struct xmcSchedCyclicPlan *xmcSchedCyclicPlanTab;
struct xmcRsvMem *xmcRsvMemTab;
struct xmcVCpu *xmcVCpuTab;
#ifdef CONFIG_IPVI_SUPPORT
xm_u8_t *xmcDstIpvi;
#endif
xm_s8_t *xmcStringTab;

#if defined(CONFIG_DEV_MEMBLOCK)||defined(CONFIG_DEV_MEMBLOCK_MODULE)
struct xmcMemBlock *xmcMemBlockTab;
#endif

localProcessor_t localProcessorInfo[CONFIG_NO_CPUS];

extern xm_u32_t resetStatusInit[];

static volatile xm_s32_t procWaiting = 0;

extern xm_u8_t _sxm[], _exm[], physXmcTab[];
extern void start(void);

struct xmHdr xmHdr __XMHDR={
    .sSignature=XMEF_XM_MAGIC,
    .compilationXmAbiVersion=XM_SET_VERSION(XM_ABI_VERSION, XM_ABI_SUBVERSION, XM_ABI_REVISION),
    .compilationXmApiVersion=XM_SET_VERSION(XM_API_VERSION, XM_API_SUBVERSION, XM_API_REVISION),
    .noCustomFiles=1,
    .customFileTab={
	[0]={.sAddr=(xmAddress_t)physXmcTab, .size=0,},
    },
    .eSignature=XMEF_XM_MAGIC,
};

extern __NOINLINE void FreeBootMem(void);

void IdleTask(void) {
    while(1) {
	DoPreemption();
    }
}

void HaltSystem(void) {
    extern void __HaltSystem(void);

    __HaltSystem();
}

#ifdef CONFIG_JMP_USR_FUNC_COLD_RESET_SYSTEM
extern void UsrReset(xmHmLog_t *log);
#endif

void ResetSystem(xm_u32_t resetMode, xmHmLog_t *log) {
    extern xm_u32_t sysResetCounter[];
    extern xm_u32_t sysResetStatus[];
    extern void start(void);
    extern void _Reset(xmAddress_t);
#ifdef CONFIG_SMP_SUPPORT
    extern volatile xm_u32_t resetMask[];
#endif
    ASSERT(!HwIsSti());
    sysResetCounter[0]++;
    sysResetStatus[0]=resetMode;

#ifdef CONFIG_SMP_SUPPORT
    if ((resetMode&XM_RESET_MODE)==XM_WARM_RESET) {
        SendAllIpi(WARM_RESET_SYSTEM_IPI_MSG);
    } else {
        SendAllIpi(COLD_RESET_SYSTEM_IPI_MSG);
    }
	if (GET_CPU_ID()==0) {
		while (resetMask[0]);
    }
#endif
    if ((resetMode&XM_RESET_MODE)==XM_WARM_RESET) {
	_Reset((xmAddress_t)start);
    } else { // Cold Reset
#ifdef CONFIG_JMP_USR_FUNC_COLD_RESET_SYSTEM
        UsrReset(log);
#else
        _Reset(xmcTab.rsw.entryPoint);
#endif
    }
    while(1);
}

static void CreateLocalInfo(void) {
    xm_s32_t e;
    if (!GET_NRCPUS()) {
        cpuCtxt_t ctxt;
        GetCpuCtxt(&ctxt);
        SystemPanic(&ctxt, "No cpu found in the system\n");    
    }
    memset(localProcessorInfo, 0, sizeof(localProcessor_t)*CONFIG_NO_CPUS);
    for (e=0; e<CONFIG_NO_CPUS; e++)
        localProcessorInfo[e].cpu.globalIrqMask=~0;

}

static void LocalSetup(xm_s32_t cpuId, kThread_t *idle) {
    ASSERT(!HwIsSti());
    ASSERT(xmcTab.hpv.noCpus>cpuId);
    SetupCpu();
    SetupArchLocal(cpuId);
    SetupHwTimer();
    SetupKTimers();
#ifdef CONFIG_SMP_SUPPORT
    SetupLocalSmp();
#endif
    InitSchedLocal(idle);
#if defined(CONFIG_MPU) && defined(CONFIG_AT697F)
    SetupSharedMemoryArea();
#endif
}

static void SetupPartitions(void) {
    xmAddress_t st, end, vSt, vEnd;
    partition_t *k;
    xm_s32_t e, a;

    kprintf ("%d Partition(s) created\n", xmcTab.noPartitions);
    // Creating the partitions
    for (e=0; e<xmcTab.noPartitions; e++) {
        if ((k=CreatePartition(&xmcPartitionTab[e]))) {
	    kprintf("P%d (\"%s\":%d) flags: [", e, &xmcStringTab[xmcPartitionTab[e].nameOffset], xmcPartitionTab[e].rId);
	    if (xmcPartitionTab[e].flags&XM_PART_SYSTEM)
                kprintf(" SYSTEM");
	    if (xmcPartitionTab[e].flags&XM_PART_FP)
               kprintf(" FP");
	    kprintf(" ]:\n");
	    for (a=0; a<xmcPartitionTab[e].noPhysicalMemoryAreas; a++) {
		st=xmcPhysMemAreaTab[a+xmcPartitionTab[e].physicalMemoryAreasOffset].startAddr;
		end=st+xmcPhysMemAreaTab[a+xmcPartitionTab[e].physicalMemoryAreasOffset].size-1;
                vSt=xmcPhysMemAreaTab[a+xmcPartitionTab[e].physicalMemoryAreasOffset].mappedAt;
		vEnd=vSt+xmcPhysMemAreaTab[a+xmcPartitionTab[e].physicalMemoryAreasOffset].size-1;
                
		kprintf("    [0x%lx:0x%lx - 0x%lx:0x%lx]", st, vSt, end, vEnd);
		kprintf(" flags: 0x%x", xmcPhysMemAreaTab[a+xmcPartitionTab[e].physicalMemoryAreasOffset].flags);
		kprintf("\n");
	    }

            if (xmcPartitionTab[e].flags&XM_PART_BOOT) 
                ResetPartition(k, xmcPartitionTab[e].entryPoint, 1, resetStatusInit[0]);  
	} else {
            cpuCtxt_t ctxt;
            GetCpuCtxt(&ctxt);
	    SystemPanic(&ctxt, "[LoadGuests] Error creating partition");
        }
    }
}

static void LoadCfgTab(void) {
    // Check configuration file
    if (xmcTab.signature!=XMC_SIGNATURE)
	HaltSystem();
#define CALC_ABS_ADDR_XMC(_offset) (void *)(xmcTab._offset+(xmAddress_t)&xmcTab)
    
    xmcPartitionTab=CALC_ABS_ADDR_XMC(partitionTabOffset);
    xmcMemRegTab=CALC_ABS_ADDR_XMC(memoryRegionsOffset);
    xmcPhysMemAreaTab=CALC_ABS_ADDR_XMC(physicalMemoryAreasOffset);
#if defined(CONFIG_MPU) && defined(CONFIG_AT697F)
    xmcSharedMemAreaTab=CALC_ABS_ADDR_XMC(sharedMemoryAreasOffset);
#endif
    xmcCommChannelTab=CALC_ABS_ADDR_XMC(commChannelTabOffset);
    xmcCommPorts=CALC_ABS_ADDR_XMC(commPortsOffset);
    xmcIoPortTab=CALC_ABS_ADDR_XMC(ioPortsOffset);
    xmcSchedCyclicSlotTab=CALC_ABS_ADDR_XMC(schedCyclicSlotsOffset);
    xmcSchedCyclicPlanTab=CALC_ABS_ADDR_XMC(schedCyclicPlansOffset);
    xmcStringTab=CALC_ABS_ADDR_XMC(stringsOffset);   
    xmcRsvMemTab=CALC_ABS_ADDR_XMC(rsvMemTabOffset);
#ifdef CONFIG_IPVI_SUPPORT
    xmcDstIpvi=CALC_ABS_ADDR_XMC(ipviDstOffset);
#endif
#if defined(CONFIG_DEV_MEMBLOCK)||defined(CONFIG_DEV_MEMBLOCK_MODULE)
    xmcMemBlockTab=CALC_ABS_ADDR_XMC(deviceTab.memBlocksOffset);
#endif
    xmcVCpuTab=CALC_ABS_ADDR_XMC(vCpuTabOffset);
}

#ifdef CONFIG_SMP_SUPPORT
static spinLock_t sLBarrier=SPINLOCK_INIT;
static volatile xm_u32_t syncProc0=0, syncProc=0;

static void SyncSmp0(void) {
    SpinLock(&sLBarrier);
    syncProc0++;
    SpinUnlock(&sLBarrier);
    while(syncProc0!=GET_NRCPUS());
}

static void SyncSmp(void) {
    SpinLock(&sLBarrier);
    syncProc++;
    SpinUnlock(&sLBarrier);
    while(syncProc!=GET_NRCPUS());
}

#endif

void Setup(xm_s32_t cpuId, kThread_t *idle) {
#ifdef CONFIG_EARLY_OUTPUT
    extern void SetupEarlyOutput(void);
#endif

    ASSERT(!HwIsSti());
#ifdef CONFIG_EARLY_OUTPUT
    SetupEarlyOutput();
#endif
    LoadCfgTab();
    InitRsvMem();
    EarlySetupArchCommon();
    SetupVirtMM();
    SetupArchCommon();
    SetupIrqs();
    SetupKDev();
    SetupObjDir();
    kprintf("XM Hypervisor (%x.%x r%x)\n", (XM_VERSION>>16)&0xFF, (XM_VERSION>>8)&0xFF, XM_VERSION&0xFF);
    kprintf("Detected %lu.%luMHz processor.\n", (xm_u32_t)(cpuKhz/1000), (xm_u32_t)(cpuKhz%1000));
    InitSched();
    CreateLocalInfo();
    SetupSysClock();
    LocalSetup(cpuId, idle);
    /* Setup Watchdog after timers and clock have been setup */
    KWatchdogSetup();
#ifdef CONFIG_SMP_SUPPORT    
    SetupSmp();
    SyncSmp0();
#endif
    SetupPartitions();
#ifdef CONFIG_SMP_SUPPORT
    SyncSmp();
#endif
    /*
     * Initialize the XtratuM watchdog.
     * The initial budget is set. This call should be closest to first schedule
     * call.
     */
    KWatchdogInit();
    FreeBootMem();
}

#ifdef CONFIG_SMP_SUPPORT
void InitSecondaryCpu(xm_s32_t cpuId, kThread_t *idle) {
    ASSERT(GET_CPU_ID()!=0);
    LocalSetup(cpuId, idle);
    GET_LOCAL_PROCESSOR()->sched.flags|=LOCAL_SCHED_ENABLED;
    SyncSmp0();
    SyncSmp();
    Schedule();
    IdleTask();
}
#endif
