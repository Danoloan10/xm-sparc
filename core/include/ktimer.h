/*
 * $FILE: ktimer.h
 *
 * XM's timer interface
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:  
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 *
 */

#ifndef _XM_KTIMERS_H_
#define _XM_KTIMERS_H_

#ifndef _XM_KERNEL_
#error Kernel file, do not include.
#endif

#ifndef __ASSEMBLY__
#include <list.h>
#include <smp.h>

#define NSECS_PER_SEC 1000000000ULL
#define USECS_PER_SEC 1000000UL

typedef xm_s32_t (*timerHandler_t)(void);

typedef struct hwClock {
    char *name;
    xm_u32_t flags;
#define HWCLOCK_ENABLED (1<<0)
#define PER_CPU (1<<1)
    xm_u32_t freqKhz;
    xm_s32_t (*InitClock)(void);
    xmTime_t (*GetTimeUsec)(void);
} hwClock_t;

extern hwClock_t *sysHwClock;

// The following structure defines each hwtimer existing in the system
typedef struct hwTimer {
    xm_s8_t *name;
    xm_u32_t flags;
#define HWTIMER_ENABLED (1<<0)
    xm_u32_t freqKhz;    
    xm_s32_t irq;
    xm_s32_t (*InitHwTimer)(void);
    void (*SetHwTimer)(xmTime_t);
    // This is the maximum value to be programmed
    xmTime_t (*GetMaxInterval)(void);
    xmTime_t (*GetMinInterval)(void);
    timerHandler_t (*SetTimerHandler)(timerHandler_t);
} hwTimer_t;

typedef struct kTimer {
    struct dynListNode dynListPtrs; // hard-coded, don't touch
    hwTime_t value;
    hwTime_t interval;
    xm_u32_t flags;
#define KTIMER_ARMED (1<<0)
    void *actionArgs;
    void (*Action)(struct kTimer *, void *);
} kTimer_t;

typedef struct {
    xmTime_t value;
    xmTime_t interval;
#define VTIMER_ARMED (1<<0)
    xm_u32_t flags;
    kTimer_t kTimer;
} vTimer_t;

typedef struct {
    xmTime_t acc;
    xmTime_t delta;
    xm_u32_t flags;
#define VCLOCK_ENABLED (1<<0)
} vClock_t;

extern xm_s32_t SetupKTimers(void);
extern void SetupSysClock(void);
extern void SetupHwTimer(void);

static inline xmTime_t GetSysClockUsec(void) {
    return sysHwClock->GetTimeUsec();
}

static inline xmTime_t GetTimeUsecVClock(vClock_t *vClock) {
    xmTime_t t=vClock->acc;
    if (vClock->flags&VCLOCK_ENABLED)
	t+=(GetSysClockUsec()-vClock->delta);
	    
    return t;
}

extern void InitKTimer(xm_s32_t vCpuId, kTimer_t *kTimer, void (*Act)(kTimer_t *, void *), void *args, void *kThread);
extern xm_s32_t ArmKTimer(kTimer_t *kTimer, xmTime_t value, xmTime_t interval);
extern xm_s32_t DisarmKTimer(kTimer_t *kTimer);
extern xm_s32_t InitVTimer(xm_s32_t vCpuId, vTimer_t *vTimer, void *k);
extern xm_s32_t ArmVTimer(vTimer_t *vTimer, vClock_t *vClock, xmTime_t value, xmTime_t interval);
extern xm_s32_t DisarmVTimer(vTimer_t *vTimer, vClock_t *vClock);
extern inline void SetHwTimer(xmTime_t nextAct);
extern xmTime_t TraverseKTimerQueue(struct dynList *l, xmTime_t cTime);

static inline void InitVClock(vClock_t *vClock) {
    vClock->acc=0;
    vClock->delta=0;
    vClock->flags=0;
}

static inline void StopVClock(vClock_t *vClock, vTimer_t *vTimer) {
    if (vTimer->flags&VTIMER_ARMED)
	DisarmKTimer(&vTimer->kTimer);

    vClock->flags&=(~VCLOCK_ENABLED);
    vClock->acc+=GetSysClockUsec()-vClock->delta;
}

static inline void ResumeVClock(vClock_t *vClock, vTimer_t *vTimer) {
    vClock->delta=GetSysClockUsec();
    vClock->flags|=VCLOCK_ENABLED;
    
    if (vTimer->flags&VTIMER_ARMED)
	ArmKTimer(&vTimer->kTimer, vTimer->value-vClock->acc+vClock->delta, vTimer->interval);
}

static inline xmTime_t HwTime2Duration(hwTime_t t, hwTime_t hz) {
    hwTime_t s;
    s=t/hz;
    return (s*USECS_PER_SEC+((t-s*hz)*USECS_PER_SEC)/hz);
}

static inline hwTime_t Duration2HwTime(xmTime_t d, hwTime_t hz) {
    hwTime_t s;
    s=d/USECS_PER_SEC;
    return s*hz+((d-s*USECS_PER_SEC)*hz)/USECS_PER_SEC;
}

extern hwTimer_t *GetSysHwTimer(void);

#endif
#endif
