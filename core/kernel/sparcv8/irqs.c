/*
 * $FILE: irqs.c
 *
 * IRQS' code
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
  - [17/11/2015:SPR-221015-01] MMU trap handling of 4 & 16 removed
*/

#include <assert.h>
#include <bitwise.h>
#include <irqs.h>
#include <kthread.h>
#include <physmm.h>
#include <processor.h>
#include <sched.h>
#include <stdc.h>

xm_s32_t ArchTrapIsSysCtxt(cpuCtxt_t *ctxt) {
    extern xm_u8_t WindowOverflowTrap[], EWindowOverflowTrap[];
    extern xm_u8_t WindowUnderflowTrap[], EWindowUnderflowTrap[];
    extern xm_u8_t SIRetCheckRetAddr[], EIRetCheckRetAddr[];

    if ((ctxt->pc>=(xmAddress_t)WindowOverflowTrap)&&(ctxt->pc<(xmAddress_t)EWindowOverflowTrap))
        return 0;
    
    if ((ctxt->pc>=(xmAddress_t)WindowUnderflowTrap)&&(ctxt->pc<(xmAddress_t)EWindowUnderflowTrap))
        return 0;

    if ((ctxt->pc>=(xmAddress_t)SIRetCheckRetAddr)&&(ctxt->pc<(xmAddress_t)EIRetCheckRetAddr))
        return 0;

    return 1;
}

#if defined(CONFIG_MMU)
static xm_s32_t Sparcv8TrapPageFault(cpuCtxt_t *ctxt) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xm_u32_t faultAddress=GetMmuFaultAddressReg();

    info->sched.cKThread->ctrl.g->partCtrlTab->arch.faultStatusReg=GetMmuFaultStatusReg();
    info->sched.cKThread->ctrl.g->partCtrlTab->arch.faultAddressReg=faultAddress;
    return 0;
}
#endif

xmAddress_t IrqVector2Address(xm_s32_t vector) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xmAddress_t *tbr=(xmAddress_t *)info->sched.cKThread->ctrl.g->partCtrlTab->arch.tbr;
    cpuCtxt_t *ctxt=info->sched.cKThread->ctrl.irqCpuCtxt;
    if (CheckGParam(tbr, 256*16, 4, 0)<0)
        PartitionPanic(ctxt,"Partition unrecoverable error: incorrect TBR address (0x%x)\n", tbr);
    return (xmAddress_t)&(tbr[vector*4]);
}

void ArchSetupIrqs(void) {
    extern void InitPic(void);
    InitPic();

#if defined(CONFIG_MMU)
    SetTrapHandler(1, Sparcv8TrapPageFault);
    SetTrapHandler(15, Sparcv8TrapPageFault);
#endif
}

