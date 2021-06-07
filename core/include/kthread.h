/*
 * $FILE: kthread.h
 *
 * Kernel and Guest kthreads
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
  - [02/09/2015:SPR-020915-02] 'continue' replaced with equivalent code
  - [02/10/2015:SPR-021015-01] Added entry point field to partition
  - [09/12/2015:SPR-031215-02] Entry point field removed from partition
*/

#ifndef _XM_KTHREAD_H_
#define _XM_KTHREAD_H_

#include <assert.h>
#include <guest.h>
#include <ktimer.h>
#include <xmconf.h>
#include <xmef.h>
#include <objdir.h>
#include <arch/kthread.h>
#include <arch/atomic.h>
#include <arch/irqs.h>
#include <arch/xm_def.h>
#ifdef CONFIG_OBJ_STATUS_ACC
#include <objects/status.h>
#endif

#ifndef _XM_KERNEL_
#error Kernel file, do not include.
#endif

struct guest {
#define PART_VCPU_ID2KID(partId, vCpuId) ((vCpuId)<<8)|((partId)&0xff)
#define KID2PARTID(id) ((id)&0xff)
#define KID2VCPUID(id) ((id)>>8)
    xmId_t id; // 15..8: vCpuId, 7..0: partitionId
    struct kThreadArch kArch;
    vTimer_t vTimer;
    kTimer_t kTimer;
    vClock_t vClock;
    // this field is accessible by the Guest
    partitionControlTable_t *partCtrlTab;
    xmAddress_t imgStart;
    xm_u32_t resetCounter;
    xm_u32_t resetStatus;
    xm_u32_t swTrap;
    struct trapHandler overrideTrapTab[NO_TRAPS];
};

#define PCT_SIZE (sizeof(partitionControlTable_t)+sizeof(struct xmPhysicalMemMap)*CONFIG_MAX_NO_MAREAS+(CONFIG_OBJ_MAX_NO_COMMPORTS>>XM_LOG2_WORD_SZ))

static inline xm_s32_t IsPCtrlTabPg(xmAddress_t addr, struct guest *g) {
    xmAddress_t a, b;
    a=_VIRT2PHYS(g->partCtrlTab);
    b=a+PCT_SIZE;
    return ((addr>=a)&&(addr<b))?1:0;
}

#define CHECK_KTHR_SANITY(k) ASSERT((k->ctrl.magic1==KTHREAD_MAGIC)&&(k->ctrl.magic2==KTHREAD_MAGIC))

#ifdef CONFIG_MMU
#define GET_GUEST_GCTRL_ADDR(k) (k)->ctrl.g->RAM+ROUNDUP(sizeof(partitionControlTable_t))
#else
#define GET_GUEST_GCTRL_ADDR(k) ((xmAddress_t)((k)->ctrl.g->partCtrlTab))
#endif

typedef union kThread {
    struct __kThread {
	// Harcoded, don't change it
	xm_u32_t magic1;
	// Harcoded, don't change it
	xmAddress_t *kStack;
#ifdef CONFIG_SMP_SUPPORT
        spinLock_t lock;
#endif
	volatile xm_u32_t flags;
	//  [3...0] -> scheduling bits
#define KTHREAD_HALTED_F (1<<2)  // 1:HALTED
#define KTHREAD_SUSPENDED_F (1<<3) // 1:SUSPENDED
#define KTHREAD_READY_F (1<<4) // 1:READY
#define KTHREAD_FLUSH_CACHE_B 5
#define KTHREAD_FLUSH_CACHE_W 3
#define KTHREAD_FLUSH_DCACHE_F (1<<5)
#define KTHREAD_FLUSH_ICACHE_F (1<<6)
#define KTHREAD_CACHE_ENABLED_B 7
#define KTHREAD_CACHE_ENABLED_W 3
#define KTHREAD_DCACHE_ENABLED_F (1<<7)
#define KTHREAD_ICACHE_ENABLED_F (1<<8)

#define KTHREAD_NO_PARTITIONS_FIELD (0xff<<16) // No. partitions
#define KTHREAD_TRAP_PENDING_F (1<<31) // 31: PENDING
	struct dynList localActiveKTimers;
	struct guest *g;
	void *schedData;
        cpuCtxt_t *irqCpuCtxt;
	xm_u32_t irqMask;
	xm_u32_t magic2;
    } ctrl;
    xm_u8_t kStack[CONFIG_KSTACK_SIZE];
} kThread_t;

static inline void SetKThreadFlags(kThread_t *k, xm_u32_t f) {
    k->ctrl.flags|=f;
    if (k->ctrl.g&&k->ctrl.g->partCtrlTab) 
        k->ctrl.g->partCtrlTab->flags|=f;
}

static inline void ClearKThreadFlags(kThread_t *k, xm_u32_t f) {
    k->ctrl.flags&=~f;
    if (k->ctrl.g&&k->ctrl.g->partCtrlTab) 
        k->ctrl.g->partCtrlTab->flags&=~f;
}

static inline xm_u32_t AreKThreadFlagsSet(kThread_t *k, xm_u32_t f) {
    xm_u32_t __r;
    __r=k->ctrl.flags&f;
    return __r;
}

typedef struct partition {
    kThread_t *kThread[CONFIG_NO_VCPUS];
    xmAddress_t pctArray;
    struct xmcPartition *cfg;
} partition_t;
extern partition_t *partitionTab;

static inline partition_t *GetPartition(kThread_t *k) {
    if (k->ctrl.g)
        return &partitionTab[KID2PARTID(k->ctrl.g->id)];
    return 0;
}

static inline struct xmcPartition *GetPartitionCfg(kThread_t *k) {
    partition_t *p=GetPartition(k);
    return (p)?p->cfg:0;
}

extern void InitIdle(kThread_t *idle, xm_s32_t cpu);

extern partition_t *CreatePartition(struct xmcPartition *cfg);
extern void ResetPartition(partition_t *p, xmAddress_t entryPoint, xm_u32_t cold, xm_u32_t status);
extern void ResetKThread(kThread_t *k, xmAddress_t entryPoint);

extern void ArchCreateKThread(kThread_t *k);
extern void FillMmPartCtrlTabWarm(partitionControlTable_t *partCtrlTab, kThread_t *k);
extern void FillMmPartCtrlTabCold(partitionControlTable_t *partCtrlTab, kThread_t *k);
extern void FillArchPartCtrlTabWarm(partitionControlTable_t *partCtrlTab, kThread_t *k);
extern void FillArchPartCtrlTabCold(partitionControlTable_t *partCtrlTab, kThread_t *k);
extern void SwitchKThreadArchPre(kThread_t *new, kThread_t *current);
extern void SwitchKThreadArchPost(kThread_t *current);

// IPI missed?
static inline void SetPartitionHwIrqPending(partition_t *p, xm_s32_t irq) {
    kThread_t *k;
    xm_s32_t e;

    ASSERT((irq>=XM_VT_HW_FIRST)&&(irq<=XM_VT_HW_LAST));
    
    for (e=0; e<CONFIG_NO_VCPUS; e++) {
        k=p->kThread[e];
        
        if (!AreKThreadFlagsSet(k, KTHREAD_HALTED_F))
        {
#ifdef CONFIG_SMP_SUPPORT
            SpinLock(&k->ctrl.lock);
#endif
            k->ctrl.g->partCtrlTab->hwIrqsPend|=(1<<irq);
#ifdef CONFIG_SMP_SUPPORT
            SpinUnlock(&k->ctrl.lock);
#endif
            SetKThreadFlags(k, KTHREAD_READY_F);
        }
    }
}

// IPI missed?
static inline void SetPartitionExtIrqPending(partition_t *p, xm_s32_t irq) {
    kThread_t *k;
    xm_s32_t e;

    ASSERT((irq>=XM_VT_EXT_FIRST)&&(irq<=XM_VT_EXT_LAST));
    irq-=XM_VT_EXT_FIRST;
    for (e=0; e<CONFIG_NO_VCPUS; e++) {
        k=p->kThread[e];
        if (!AreKThreadFlagsSet(k, KTHREAD_HALTED_F))
        {
#ifdef CONFIG_SMP_SUPPORT
            SpinLock(&k->ctrl.lock);
#endif
            k->ctrl.g->partCtrlTab->extIrqsPend|=(1<<irq);
#ifdef CONFIG_SMP_SUPPORT
            SpinUnlock(&k->ctrl.lock);
#endif
            SetKThreadFlags(k, KTHREAD_READY_F);
        }
    }
}

static inline void SetExtIrqPending(kThread_t *k, xm_s32_t irq) {

    ASSERT((irq>=XM_VT_EXT_FIRST)&&(irq<=XM_VT_EXT_LAST));
    irq-=XM_VT_EXT_FIRST;
    if (AreKThreadFlagsSet(k, KTHREAD_HALTED_F))
            return;
#ifdef CONFIG_SMP_SUPPORT
    SpinLock(&k->ctrl.lock);
#endif
    k->ctrl.g->partCtrlTab->extIrqsPend|=(1<<irq);
#ifdef CONFIG_SMP_SUPPORT
    SpinUnlock(&k->ctrl.lock);
#endif
    SetKThreadFlags(k, KTHREAD_READY_F);
}


extern void SetupKStack(kThread_t *k, void *StartUp, xmAddress_t entry);
extern void KThreadArchInit(kThread_t *k);
extern void StartUpGuest(xmAddress_t entry);
extern void RsvPartFrames(void);
extern xmId_t PartitionId2PId(xmId_t partitionId);

#endif
