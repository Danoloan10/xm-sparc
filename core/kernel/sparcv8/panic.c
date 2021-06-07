/*
 * $FILE: panic.c
 *
 * Code executed in a panic situation
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <kthread.h>
#include <processor.h>
#include <sched.h>
#include <stdc.h>
#include <objects/hm.h>

void DumpState(cpuCtxt_t *regs) {
    kprintf("CPU state:\n");
    kprintf("G0:0x0 G1:0x%x G2:0x%x G3:0x%x\n", regs->g1, regs->g2, regs->g3);
    kprintf("G4:0x%x G5:0x%x G6:0x%x G7:0x%x\n", regs->g4, regs->g5, regs->g6, regs->g7);
    kprintf("PC:0x%x NPC:0x%x PSR:0x%x Y:0x%x\n", regs->pc, regs->nPc, regs->psr, regs->y);
#if defined(CONFIG_MMU)||defined(CONFIG_MPU)
    kprintf("FSR:0x%x FAR:0x%x\n", GetMmuFaultStatusReg(), GetMmuFaultAddressReg());
#endif
    kprintf("\n");
}
