/*
 * $FILE: leon_timers.c
 *
 * LEON's timers
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <assert.h>
#include <kdevice.h>
#include <ktimer.h>
#include <stdc.h>
#include <processor.h>
#include <arch/irqs.h>
#include <arch/xm_def.h>

#include <local.h>
#include <spinlock.h>

#if defined(CONFIG_LEON2) || defined(CONFIG_LEON3) || defined(CONFIG_LEON3FT) || defined(CONFIG_LEON4)

RESERVE_HWIRQ(TIMER1_TRAP_NR);
RESERVE_HWIRQ(TIMER2_TRAP_NR);
#if defined(CONFIG_LEON3FT)
RESERVE_HWIRQ(TIMER3_TRAP_NR);
RESERVE_HWIRQ(TIMER_LATCH_TRAP_NR);
RESERVE_IOPORTS(LEON_TIMER3_BASE, 3);
RESERVE_IOPORTS(LEON_TIMER_LATCH_BASE, 3);
#endif 
RESERVE_IOPORTS(LEON_TIMER1_BASE, 3);
RESERVE_IOPORTS(LEON_TIMER2_BASE, 3);
RESERVE_IOPORTS(LEON_TIMER_CFG_BASE, 2);
#define TIMC_REG 0x0
  #define TIM_VAL_MASK 0xFFFFFF

#define TIMR_REG 0x4
#if defined(CONFIG_LEON2)||defined(CONFIG_SIMLEON)
  #ifdef CONFIG_AT697F
    #define TIM_RLD_MASK 0xFFFFFFFF
    #define TIM_RLD_MASK_BITS 32
  #else
    #define TIM_RLD_MASK 0xFFFFFF
    #define TIM_RLD_MASK_BITS 24
  #endif
#else
  #define TIM_RLD_MASK 0xFFFFFFFF
  #define TIM_RLD_MASK_BITS 32
#endif

#define TIMCTR_REG 0x8
  #define LD_BIT 0x4
  #define RL_BIT 0x2
  #define EN_BIT 0x1
////////////
  #define IE_BIT 0x8
////////////


#define SCAC_REG 0x0
  #define SCAC_CTR_VAL_MASK 0x3FF

#define SCAR_REG 0x4
  #define SCAC_RLD_VAL_MASK 0x3FF

#define WDG_REG 0x0
  #define WDC_MASK 0x00FFFFFF

static hwTimer_t pitTimer;
static timerHandler_t pitHandler;
static spinLock_t spinlock=SPINLOCK_INIT;

static inline xm_u32_t GetTimerBaseAddr(xm_s32_t timer) {
#ifdef CONFIG_LEON2
    return LEON_TIMER1_BASE+(timer-1)*0x10;
#else
#ifdef CONFIG_LEON3FT
    if((timer==2)&&(__GetCpuHwId()==1))
	    return LEON_TIMER_LATCH_BASE;
#endif
    return LEON_TIMER1_BASE+(timer-1)*0x10+__GetCpuHwId()*0x20;
#endif
}

static inline xm_s32_t GetTimerIrq(xm_s32_t timer) {
#ifdef CONFIG_LEON2
    return TIMER1_TRAP_NR+(timer-1);
#else
#ifdef CONFIG_LEON3FT
    if((timer==2)&&(__GetCpuHwId()==1))
	        return TIMER_LATCH_TRAP_NR;
#endif
    return TIMER1_TRAP_NR+(timer-1)+__GetCpuHwId()*2;
#endif
}

static void TimerIrqHandler(cpuCtxt_t *ctxt, void *irqData) {
    if (pitHandler)
	(*pitHandler)();
    HwEnableIrq(GetTimerIrq(2));
}

static xm_s32_t InitPitTimer(void) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    pitTimer.freqKhz=LEON_PRESCALER_KHZ;
    info->cpu.globalIrqMask&=~(1<<GetTimerIrq(2));    
    SetIrqHandler(GetTimerIrq(2), TimerIrqHandler, 0);
    StoreIoReg(GetTimerBaseAddr(2)+TIMR_REG, 0);
    StoreIoReg(GetTimerBaseAddr(2)+TIMCTR_REG,  0);
    HwEnableIrq(GetTimerIrq(2));
    pitTimer.flags|=HWTIMER_ENABLED|PER_CPU;
    return 1;
}

static void SetPitTimer(xmTime_t interval) {
    StoreIoReg(GetTimerBaseAddr(2)+TIMR_REG, interval&TIM_RLD_MASK);
#ifdef CONFIG_LEON2
    StoreIoReg(GetTimerBaseAddr(2)+TIMCTR_REG, LD_BIT|EN_BIT);
#else
    StoreIoReg(GetTimerBaseAddr(2)+TIMCTR_REG, LD_BIT|EN_BIT|IE_BIT);
#endif
}

static xmTime_t GetMaxIntervalPit(void) {
    return 10000000LL; // 10s
}

static xmTime_t GetMinIntervalPit(void) {
    return 50LL; // 50usec
}

static timerHandler_t SetTimerHandlerPit(timerHandler_t TimerHandler) {
    timerHandler_t OldPitUserHandler=pitHandler;
  
    pitHandler=TimerHandler;
    return OldPitUserHandler;
}

static hwTimer_t pitTimer={
    .name="LEON timer",
    .flags=0,
    .InitHwTimer=InitPitTimer,
    .SetHwTimer=SetPitTimer,
    .GetMaxInterval=GetMaxIntervalPit,
    .GetMinInterval=GetMinIntervalPit,
    .SetTimerHandler=SetTimerHandlerPit,
};

static hwClock_t pitClock;

static struct pitClockData {
    volatile xm_u32_t ticks;
} pitClockData={
    .ticks=0,
};

static void ClockIrqHandler(cpuCtxt_t *ctxt, void *irqData) {
    pitClockData.ticks++;    
}

static xm_s32_t InitPitClock(void) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();

#ifdef CONFIG_SMP_SUPPORT
    if (__GetCpuHwId()!=0)
        return 1;
#endif
    if (__GetCpuHwId()==0) {
    // Programming LEON's preescaler to LEON_PRESCALER_KHZ
	    StoreIoReg(LEON_TIMER_CFG_BASE+SCAR_REG,((cpuKhz/LEON_PRESCALER_KHZ)-1)&SCAC_RLD_VAL_MASK);
	}

    pitClock.freqKhz=LEON_PRESCALER_KHZ;
    info->cpu.globalIrqMask&=~(1<<GetTimerIrq(1));
    SetIrqHandler(GetTimerIrq(1), ClockIrqHandler, 0);
    StoreIoReg(GetTimerBaseAddr(1)+TIMR_REG, TIM_RLD_MASK);
#ifdef CONFIG_LEON2
    StoreIoReg(LEON_TIMER1_BASE+TIMCTR_REG, LD_BIT|RL_BIT|EN_BIT);
#else
    StoreIoReg(GetTimerBaseAddr(1)+TIMCTR_REG, LD_BIT|RL_BIT|EN_BIT|IE_BIT);
#endif
    HwEnableIrq(GetTimerIrq(1));
    pitClock.flags|=HWCLOCK_ENABLED;

    return 1;
}

#define ITP_REG 0x4
#define ITC_REG 0xC

#ifdef CONFIG_SMP_SUPPORT

static inline xm_u32_t _GetTimerBaseAddr(xm_s32_t timer) {
    return LEON_TIMER1_BASE;
}

static inline xm_s32_t _GetTimerIrq(xm_s32_t timer) {
    return TIMER1_TRAP_NR;
}

#else

static inline xm_u32_t _GetTimerBaseAddr(xm_s32_t timer) {
    return GetTimerBaseAddr(timer);
}

static inline xm_s32_t _GetTimerIrq(xm_s32_t timer) {
    return GetTimerIrq(timer);
}

#endif

static hwTime_t ReadPitClock(void) {
#ifdef CONFIG_DEBUG
    static hwTime_t lastVal[]={[0 ... CONFIG_NO_CPUS-1]=0};
#endif
    hwTime_t cTime, t;
    if (!(pitClock.flags&HWCLOCK_ENABLED)) 
        return 0;
    SpinLock(&spinlock);
    // I. Ripoll's reading clock algorithm, don't modify
    cTime=TIM_RLD_MASK-LoadIoReg(_GetTimerBaseAddr(1)+TIMC_REG);
    // Checking timer1/clock overflow
    if (LoadIoReg(LEON_PIC_BASE+ITP_REG)&(1<<_GetTimerIrq(1))) {
	StoreIoReg(LEON_PIC_BASE+ITC_REG, 1<<_GetTimerIrq(1));
	pitClockData.ticks++;
	cTime=TIM_RLD_MASK-LoadIoReg(_GetTimerBaseAddr(1)+TIMC_REG);
    }
    t=pitClockData.ticks;
    cTime+=(t<<TIM_RLD_MASK_BITS);
#ifdef CONFIG_DEBUG
    ASSERT(cTime>=lastVal[GET_CPU_ID()]);
    lastVal[GET_CPU_ID()]=cTime;
#endif
    SpinUnlock(&spinlock);
    return cTime;
}

static hwClock_t pitClock={
    .name="LEON clock",
    .flags=0,
    .InitClock=InitPitClock,
    .GetTimeUsec=ReadPitClock,
};

hwClock_t *sysHwClock=&pitClock;
hwTimer_t *GetSysHwTimer(void) {
    return &pitTimer;
}

#endif

