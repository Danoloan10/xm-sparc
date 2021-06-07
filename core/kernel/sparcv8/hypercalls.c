/*
 * $FILE: hypercalls.c
 *
 * XM's hypercalls
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
 * Changelog:
 *
 * [dd/mm/yyyy]     [author]
 * [brief description]
 *
 * [20/11/2015]
 * SPR-110915-01: ipvi stuff depends on IPVI_SUPPORT
 *
 * [30/03/2016]     Ángel Esquinas (aesquinas@fentiss.com)
 * CP-250216-01:
 *     Add "ReloadWatchdogSys" to the hypercall tab, position 34.
 */

#include <assert.h>
#include <gaccess.h>
#include <hypercalls.h>
#include <sched.h>
#include <local.h>

xm_u32_t IoPortLogSearch(xm_u32_t port) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    struct xmcPartition *cfg=GetPartitionCfg(info->sched.cKThread);
    struct xmcIoPort *ioPort;
    xm_u32_t a0, a1;
    xm_s32_t i;

    for (i=0; i<cfg->noIoPorts;i++){
       ioPort=&xmcIoPortTab[i+cfg->ioPortsOffset];
	if (ioPort->type==XM_RESTRICTED_IOPORT) {
	    a0=ioPort->restricted.address;
	    a1=a0+sizeof(xm_u32_t);
	    if (port>=a0&&port<a1)
		return ioPort->restricted.mask;
	} else {
	    a0=ioPort->range.base;
	    a1=a0+sizeof(xm_u32_t)*ioPort->range.noPorts;
	    if (port>=a0&&port<a1)
		return ~0UL;
	}
    }
    
    return 0;
}

#ifdef CONFIG_SPW_RTC
xm_s32_t SparcIoOutportMsgSys(xm_u32_t port, struct ioOutPortMsg *ioMsg, xm_u32_t timeout) {
    xm_u32_t mask=0, oldValue;
    xm_s32_t i;

    ASSERT(!HwIsSti());
    if (port&0x3) return XM_INVALID_PARAM;
    if (!(mask=IoPortLogSearch(port)))
	return XM_PERM_ERROR;

    if (CheckGParam(ioMsg, 4, sizeof(xm_u32_t), PFLAG_RW|PFLAG_NOT_NULL)<0)
        return XM_INVALID_PARAM;    
    
    if (CheckGParam(ioMsg->msg, 4, sizeof(xm_u32_t), PFLAG_RW|PFLAG_NOT_NULL)<0)
        return XM_INVALID_PARAM;

    if (!(IoPortLogSearch(ioMsg->checkingAddr)))
	return XM_PERM_ERROR;

    xmTime_t initTime;
    initTime=GetSysClockUsec();
    for (i=0;i<ioMsg->size;i++){
       if (mask==~0UL) {  /* If no mask then write directly into the port */
    	  StoreIoReg(port, ioMsg->msg[i]);
       } else {           /* Otherwise read first the value of the port */
          /* <track id="restricted-port-mask"> */
	  oldValue=LoadIoReg(port);
	  StoreIoReg(port, ((oldValue&~(mask))|(ioMsg->msg[i]&mask)));
	  /* </track id="restricted-port-mask"> */
       }
       initTime=GetSysClockUsec();
       while (((LoadIoReg(ioMsg->checkingAddr)&ioMsg->validationMask)!=(ioMsg->validationValue&ioMsg->validationMask))){
          if ((GetSysClockUsec()-initTime)>timeout){
             return XM_OP_NOT_ALLOWED;
          }
       }
       
    }
    return XM_OK;
}
#endif

__hypercall xm_s32_t SparcIoOutportSys(xm_u32_t port, xm_u32_t value) {
    xm_u32_t mask=0, oldValue;

    ASSERT(!HwIsSti());
    if (port&0x3) return XM_INVALID_PARAM;
    if (!(mask=IoPortLogSearch(port)))
        return XM_PERM_ERROR;

    if (mask==~0UL) {  /* If no mask then write directly into the port */
        StoreIoReg(port, value);
    } else {           /* Otherwise read first the value of the port */
        /* <track id="restricted-port-mask"> */
        oldValue=LoadIoReg(port);
        StoreIoReg(port, ((oldValue&~(mask))|(value&mask)));
        /* </track id="restricted-port-mask"> */
    }
    return XM_OK;
}

__hypercall xm_u32_t SparcIoInportSys(xm_u32_t port, xm_u32_t *__gParam value) {
    xm_u32_t mask=0;
    
    ASSERT(!HwIsSti());
    if (CheckGParam(value, 4, sizeof(xm_u32_t), 1)<0)
        return XM_INVALID_PARAM;
    
    if (!value) return XM_INVALID_PARAM;
    if (port&0x3) return XM_INVALID_PARAM;
    if (!(mask=IoPortLogSearch(port)))
	return XM_PERM_ERROR;
    
    *value=(LoadIoReg(port)&mask); 

    return XM_OK;
}

__hypercall void SparcWriteTbrSys(xmWord_t val) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    info->sched.cKThread->ctrl.g->kArch.tbr=val;
    info->sched.cKThread->ctrl.g->partCtrlTab->arch.tbr=val;
}

// Hypercall table
HYPERCALL_TAB(HaltPartitionSys); // 0
HYPERCALL_TAB(SuspendPartitionSys); // 1
HYPERCALL_TAB(ResumePartitionSys); // 2
HYPERCALL_TAB(ResetPartitionSys); // 3
HYPERCALL_TAB(ShutdownPartitionSys); // 4
HYPERCALL_TAB(HaltSystemSys); // 5
HYPERCALL_TAB(ResetSystemSys); // 6
HYPERCALL_TAB(IdleSelfSys); // 7

HYPERCALL_TAB(GetTimeSys); // 8
HYPERCALL_TAB(SetTimerSys); // 9
HYPERCALL_TAB(ReadObjectSys); // 10
HYPERCALL_TAB(WriteObjectSys); // 11
HYPERCALL_TAB(SeekObjectSys); // 12
HYPERCALL_TAB(CtrlObjectSys); // 13

HYPERCALL_TAB(ClearIrqMaskSys); // 14
HYPERCALL_TAB(SetIrqMaskSys); // 15
HYPERCALL_TAB(ForceIrqsSys); // 16
HYPERCALL_TAB(ClearIrqsSys); // 17
HYPERCALL_TAB(RouteIrqSys); // 18

HYPERCALLR_TAB(SetCacheStateSys); // 19

HYPERCALLR_TAB(SwitchSchedPlanSys); // 20
HYPERCALLR_TAB(GetGidByName); // 21

HYPERCALL_TAB(SparcIoInportSys); // 22
HYPERCALL_TAB(SparcIoOutportSys); // 23
HYPERCALL_TAB(SparcWriteTbrSys); // 24

#ifdef CONFIG_SPW_RTC
HYPERCALL_TAB(DrvIoDisableSDPSys); // 25
HYPERCALL_TAB(DrvIoOutportSDPSys); // 26
HYPERCALL_TAB(SparcIoOutportMsgSys); // 27
#else
HYPERCALL_TAB(0); // 25
HYPERCALLR_TAB(0); // 26
HYPERCALLR_TAB(0); // 27
#endif
HYPERCALLR_TAB(0); // 28
HYPERCALLR_TAB(0); // 39

HYPERCALL_TAB(ResetVCpuSys); // 30
HYPERCALL_TAB(HaltVCpuSys); // 31
HYPERCALL_TAB(GetVCpuIdSys); // 32
#ifdef CONFIG_IPVI_SUPPORT
HYPERCALL_TAB(RaiseIpviSys); // 33
#else
HYPERCALL_TAB(0); // 33
#endif
HYPERCALL_TAB(ReloadWatchdogSys); /* 34 */

// ASM hypercall table
ASM_HYPERCALL_TAB(SparcIRetSys);
ASM_HYPERCALL_TAB(SparcFlushRegWinSys);
ASM_HYPERCALL_TAB(SparcGetPsrSys);
ASM_HYPERCALL_TAB(SparcSetPsrSys);
ASM_HYPERCALL_TAB(SparcSetPilSys);
ASM_HYPERCALL_TAB(SparcClearPilSys);
ASM_HYPERCALL_TAB(SparcCtrlWinFlowSys);
