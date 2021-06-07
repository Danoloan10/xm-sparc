/*
 * $FILE: smp.c
 *
 * Symmetric multiprocessor support
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
  - [02/09/2015:SPR-020915-04] Remove VBOOT macro and boot.h
*/

#include <assert.h>
#include <local.h>
#include <sched.h>
#include <smp.h>
#include <spinlock.h>

static struct ipiMsg {
    spinLock_t lock;
    xm_u32_t msg;
    xm_u32_t bitmap;
} ipiMsg[CONFIG_NO_CPUS];

extern void PicBroadcastIrq(xm_u32_t irq);
#define SYSTEM_IPI 13
extern void PicForceIrqCpu(xm_u8_t dst, xm_u32_t irq);

static void FillIpiMsg(xm_u32_t msg) {
    xm_u32_t cpuId=GET_CPU_ID();
    SpinLock(&ipiMsg[cpuId].lock);
    ipiMsg[cpuId].msg=msg;
    ipiMsg[cpuId].bitmap=0;
    SpinUnlock(&ipiMsg[cpuId].lock);
}

void SendAllIpi(xm_u32_t msg) {
    FillIpiMsg(msg);
    PicBroadcastIrq(SYSTEM_IPI);
}

void SendIpi(xm_u32_t cpuId, xm_u32_t msg) {
    FillIpiMsg(msg);
    PicForceIrqCpu(cpuId, SYSTEM_IPI);
}

#ifdef CONFIG_SMP_SUPPORT
extern xm_u32_t resetMask[];
static spinLock_t resetMaskLock = SPINLOCK_INIT;
void ClearResetMask(void) {
    SpinLock(&resetMaskLock);
    resetMask[0] &= ~(1<<GET_CPU_ID());
    SpinUnlock(&resetMaskLock);
}

void SetResetMask(xm_u32_t cpuId) {
    resetMask[0] |= (1<<cpuId);
}
#endif

static void IpiHandler(cpuCtxt_t *ctxt, void *data) {
    extern void start(void);
    extern void _Reset(xmAddress_t);
    xm_u32_t cpuId=GET_CPU_ID(), e, msg;
    for (e=0; e<CONFIG_NO_CPUS; e++) {
        if ((e!=cpuId) && !(ipiMsg[e].bitmap&(1<<cpuId)))
        {
            SpinLock(&ipiMsg[e].lock);
            ipiMsg[e].bitmap|=(1<<cpuId);
            msg=ipiMsg[e].msg;
            SpinUnlock(&ipiMsg[e].lock);
            switch(msg) {
            case HALT_SYSTEM_IPI_MSG:
                HaltSystem();
                break;
            case WARM_RESET_SYSTEM_IPI_MSG:
                _Reset((xmAddress_t)start);
                break;
            case COLD_RESET_SYSTEM_IPI_MSG:
                _Reset(xmcTab.rsw.entryPoint);
                break;
            case RESCHED_SYSTEM_IPI_MSG:
                Schedule();
                break;
            }
        }
    }
}

void SetupLocalSmp(void) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    HwEnableIrq(SYSTEM_IPI);
    info->cpu.globalIrqMask&=~(1<<SYSTEM_IPI);
    SetIrqHandler(SYSTEM_IPI, IpiHandler, 0);
}

void SetupSmp(void) {
    extern xm_u8_t SparcGetNoCpus(void);
	extern xm_u32_t sysResetCounter[];
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xm_s32_t cpu;
    memset(ipiMsg, 0, sizeof(struct ipiMsg)*CONFIG_NO_CPUS);
    for (cpu=0; cpu<CONFIG_NO_CPUS; cpu++)
        ipiMsg[cpu].lock=SPINLOCK_INIT;
    info->cpu.globalIrqMask&=~(1<<SYSTEM_IPI);
    SET_NRCPUS((SparcGetNoCpus()<xmcTab.hpv.noCpus)?SparcGetNoCpus():xmcTab.hpv.noCpus);
    for (cpu=GET_CPU_ID()+1; cpu<GET_NRCPUS(); cpu++) {
	    if (sysResetCounter[0]==0) {
        WakeUpCpu(cpu);
		}
        SetResetMask(cpu);
	}
}
