/*
 * $FILE: local.h
 *
 * Local processor related stuff
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_LOCAL_H_
#define _XM_LOCAL_H_

#ifndef _XM_KERNEL_
#error Kernel file, do not include.
#endif

#include <kthread.h>

typedef struct {
    struct {
        xm_u32_t flags;
#define CPU_SLOT_ENABLED (1<<0)
#define BSP_FLAG (1<<1)
        volatile xm_u32_t irqNestingCounter;
        xm_u32_t globalIrqMask;
    } cpu;
    struct localProcessorSched {
        kThread_t *idleKThread;
        kThread_t *cKThread;
        kThread_t *fpuOwner;
        kThread_t *schedThreadTab;
        // cyclic
        struct schedData {
            kTimer_t kTimer;
            struct {
                const struct xmcSchedCyclicPlan *current;
                const struct xmcSchedCyclicPlan *new;
                const struct xmcSchedCyclicPlan *prev;
            } plan;
            xm_s32_t slot; // next slot to be processed
            xmTime_t mjf;
            xmTime_t sExec;
            xmTime_t planSwitchTime;
//#ifdef CONFIG_SYSTEM_INTEGRITY
            xm_s32_t slotExec;
//#endif
            xmTime_t nextAct;
            kThread_t *kThread;
        } *data;
        
        xm_u32_t flags;
#define LOCAL_SCHED_ENABLED 0x1   
    } sched;
    struct {
        xm_u32_t flags;
#define NEXT_ACT_IS_VALID 0x1
        hwTimer_t *sysHwTimer;
        xmTime_t nextAct;
        struct dynList globalActiveKTimers;    
    } time;
} localProcessor_t;

extern localProcessor_t localProcessorInfo[];
// Processor id is missed
#ifdef CONFIG_SMP_SUPPORT
#define GET_LOCAL_PROCESSOR() (&localProcessorInfo[GET_CPU_ID()])
#else
#define GET_LOCAL_PROCESSOR() localProcessorInfo
#endif

#endif
