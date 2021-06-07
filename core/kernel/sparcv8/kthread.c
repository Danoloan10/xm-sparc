/*
 * $FILE: kthread.c
 *
 * Kernel, Guest or L0 context (ARCH dependent part)
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
  - [02/09/2015:SPR-020915-03] 'goto' replaced with equivalent code
*/
#include <assert.h>
#include <gaccess.h>
#include <kthread.h>
#include <processor.h>
#include <sched.h>
#include <stdc.h>
#include <arch/xm_def.h>

#ifdef CONFIG_MPU
#define WPR1_REG 0x0
#define WPR2_REG 0x4
#define WPR_EN 0x80000000
#define WPR_BP 0x40000000
#define WPR_TAG_OFFS 15
#define WPR_MASK_OFFS 0
#define WPR_TAG_MASK 0x3FFF8000
#define WPR_MASK_MASK 0x7FFF
#define REG_SIZE (32*1024)
#define REG_OFF 15
#define REG_MASK 0x3FFF8000
#endif


xm_s32_t irq2Vector[CONFIG_NO_HWIRQS]={
    0, // RESERVED
    INTERRUPT_LEVEL_1,
    INTERRUPT_LEVEL_2,
    INTERRUPT_LEVEL_3,
    INTERRUPT_LEVEL_4,
    INTERRUPT_LEVEL_5,
    INTERRUPT_LEVEL_6,
    INTERRUPT_LEVEL_7,
    INTERRUPT_LEVEL_8,
    INTERRUPT_LEVEL_9,
    INTERRUPT_LEVEL_10,
    INTERRUPT_LEVEL_11,
    INTERRUPT_LEVEL_12,
    INTERRUPT_LEVEL_13,
    INTERRUPT_LEVEL_14,
    INTERRUPT_LEVEL_15,
};

#ifdef CONFIG_FLUSH_CACHE_AFTER_CS
static kThread_t *cSPrevPart=0;
#endif

void SwitchKThreadArchPre(kThread_t *new, kThread_t *current) {
    ASSERT(!HwIsSti());
#ifdef CONFIG_ENABLE_CACHE
    if ((new->ctrl.flags&(PARTITION_DCACHE_ENABLED_F|PARTITION_ICACHE_ENABLED_F))!=(current->ctrl.flags&(PARTITION_DCACHE_ENABLED_F|PARTITION_ICACHE_ENABLED_F)))
        SetKThreadFlags(new, KTHREAD_FLUSH_DCACHE_F|KTHREAD_FLUSH_ICACHE_F);
#endif
#ifdef CONFIG_MMU
    if (new->ctrl.g) {
        if (new->ctrl.g->kArch.ptdL1) {
            SetMmuCtxt(GetPartitionCfg(new)->id+1);
            FlushTlbCtxt();
        }
    }
#else
    if (new->ctrl.g) {
        StoreIoReg(LEON_MEMORY_WPR_BASE+WPR1_REG, new->ctrl.g->kArch.wpReg);

#if defined(CONFIG_MPU) && defined(CONFIG_AT697F)
        //In case there is no shared memory area configured in xm_cf.xml file, WPR2 is programmed with the same value as WPR1
        if (xmcTab.hpv.noSharedMemoryAreas == 0) {
            StoreIoReg(LEON_MEMORY_WPR_BASE+WPR2_REG, new->ctrl.g->kArch.wpReg);
        }
#endif
    }
#endif
#ifdef CONFIG_FLUSH_CACHE_AFTER_CS
    if (current->ctrl.g)
	cSPrevPart=current;
#endif
}

void SwitchKThreadArchPost(kThread_t *current) {
    ASSERT(!HwIsSti());
    // Disabling FPU
    HwDisableFpu();
    if (current->ctrl.g) {
        cpuCtxt_t *cpuCtxt=current->ctrl.irqCpuCtxt;
        if (cpuCtxt) {
            ASSERT((cpuCtxt->psr&0xff000000)==(GetPsr()&0xff000000));
            cpuCtxt->psr&=~PSR_EF_BIT;
        }
    }
    // Flushing the cache
#ifdef CONFIG_FLUSH_CACHE_AFTER_CS
    if (current->ctrl.g)
        SetKThreadFlags(current, KTHREAD_FLUSH_DCACHE_F|KTHREAD_FLUSH_ICACHE_F);
#endif
}

void KThreadArchInit(kThread_t *k) {
}

void SetupPartCtrlTabArch(kThread_t *k) {
}

void SetupKStack(kThread_t *k, void *StartUp, xmAddress_t entry) {
    extern xm_u32_t ArchStartupGuest;
    k->ctrl.kStack=(xm_u32_t *)&k->kStack[CONFIG_KSTACK_SIZE-MIN_STACK_FRAME-8];
    *--(k->ctrl.kStack)=(xm_u32_t)0; /* o1 */
    *--(k->ctrl.kStack)=(xm_u32_t)entry; /* o0 */
    *--(k->ctrl.kStack)=(xm_u32_t)&ArchStartupGuest;  /* %g5 */
    *--(k->ctrl.kStack)=(xm_u32_t)StartUp;  /* %g4 */
    *--(k->ctrl.kStack)=(xm_u32_t)GetPsr()&~(PSR_CWP_MASK|PSR_ICC_MASK); /*  %PSR (%g7) */
    *--(k->ctrl.kStack)=(xm_u32_t)2; /* %WIM (%g6) */
}

#define IS_POW_TWO(x) (x&&!(x&(x-1)))
void ArchCreateKThread(kThread_t *k) {
#ifdef CONFIG_MPU
    xm_u32_t tag, mask;
    struct xmPhysicalMemMap memMap;
    struct xmcPartition *conf=k->ctrl.g->cfg;
    // Checking that the memory area of the guest

    // 1) For this configuration (CONFIG_MPU), only one area is allowed
    if (conf->noPhysicalMemoryAreas!=1) PartitionPanic(0, "[Guest] Only one memory area expected\n");

    memMap.startAddr=xmcPhysMemAreaTab[k->ctrl.g->cfg->physicalMemoryAreasOffset].startAddr;
    memMap.size=xmcPhysMemAreaTab[k->ctrl.g->cfg->physicalMemoryAreasOffset].size;

    // 2) The area size must be at _least_ 32KB and power of two
    if (((memMap.size&(REG_SIZE-1))&&!IS_POW_TWO(memMap.size))||
        // 3) The start address must be greater than or equal to the size or must be power of two
        ((memMap.startAddr<memMap.size)&&!IS_POW_TWO(memMap.startAddr)) ||
        // 4) Size must be multiple of 32 KB (REG_SIZE)
        //and start address must be multiple of 32 KB (REG_SIZE)
        //and size must be less or equal to start address value        
        ((memMap.size & (REG_SIZE-1)) ||(memMap.startAddr & (REG_SIZE-1)) ||(memMap.size > memMap.startAddr)) ||
        // 5) Start address must be a multiple of size
        (memMap.startAddr % memMap.size)) {
        PartitionPanic(0, "[Guest] Bad memory area [0x%x - 0x%x]\n", memMap.startAddr, memMap.startAddr+memMap.size);
    } else {
        //Configuration of WPR1 with BP = 0
        //Make writable region from (memMap.startAddr) to (memMap.startAddr + memMap.size)
        mask = ((~(memMap.size-1)) & REG_MASK);
        tag = (memMap.startAddr & REG_MASK);
        k->ctrl.g->kArch.wpReg = (mask>>REG_OFF)|tag;
    }
    //WPR2 configuration will depend on the board target. In case it is used, it will be configured in the XtratuM setup.    
#endif
}

void FillArchPartCtrlTabWarm(partitionControlTable_t *partCtrlTab, kThread_t *k) {
}

void FillArchPartCtrlTabCold(partitionControlTable_t *partCtrlTab, kThread_t *k) {
    xm_s32_t e;
    partCtrlTab->trap2Vector[0]=DATA_STORE_ERROR;
    partCtrlTab->trap2Vector[1]=INSTRUCTION_ACCESS_MMU_MISS;    
    partCtrlTab->trap2Vector[2]=INSTRUCTION_ACCESS_ERROR;
    partCtrlTab->trap2Vector[3]=R_REGISTER_ACCESS_ERROR;
    partCtrlTab->trap2Vector[4]=INSTRUCTION_ACCESS_EXCEPTION;
    partCtrlTab->trap2Vector[5]=PRIVILEGED_INSTRUCTION;
    partCtrlTab->trap2Vector[6]=ILLEGAL_INSTRUCTION;
    partCtrlTab->trap2Vector[7]=FP_DISABLED;
    partCtrlTab->trap2Vector[8]=CP_DISABLED;
    partCtrlTab->trap2Vector[9]=UNIMPLEMENTED_FLUSH;
    partCtrlTab->trap2Vector[10]=WATCHPOINT_DETECTED;
    partCtrlTab->trap2Vector[11]=MEM_ADDRESS_NOT_ALIGNED;
    partCtrlTab->trap2Vector[12]=FP_EXCEPTION;
    partCtrlTab->trap2Vector[13]=CP_EXCEPTION;
    partCtrlTab->trap2Vector[14]=DATA_ACCESS_ERROR;
    partCtrlTab->trap2Vector[15]=DATA_ACCESS_MMU_MISS;
    partCtrlTab->trap2Vector[16]=DATA_ACCESS_EXCEPTION;
    partCtrlTab->trap2Vector[17]=TAG_OVERFLOW;
    partCtrlTab->trap2Vector[18]=DIVISION_BY_ZERO;

    for (e=0; e<CONFIG_NO_HWIRQS; e++)
        partCtrlTab->hwIrq2Vector[e]=irq2Vector[e];

    for (e=0; e<XM_VT_EXT_MAX; e++)
	partCtrlTab->extIrq2Vector[e]=224+e;
}

