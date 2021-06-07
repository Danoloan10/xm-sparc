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
/*
  Changelog:
  - [02/09/2015:SPR-020915-01] MISRA rules.9.1 violation fixed.
*/
#include <kthread.h>
#include <processor.h>
#include <sched.h>
#include <smp.h>
#include <spinlock.h>
#include <stdc.h>
#include <objects/hm.h>

void SystemPanic(cpuCtxt_t *ctxt, xm_s8_t *fmt, ...) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xmHmLog_t hmLog;
    va_list vl = 0;
    ASSERT(fmt);

    HwCli();

    kprintf("System FATAL ERROR:\n");
    va_start(vl, fmt);
    vprintf(fmt, vl);
    va_end(vl);
    kprintf("\n");
    if (info&&info->sched.cKThread&&ctxt)
	DumpState(ctxt);
    hmLog.opCode=0;
    if (info&&info->sched.cKThread&&info->sched.cKThread->ctrl.g)
	hmLog.opCode=GetPartitionCfg(info->sched.cKThread)->rId<<HMLOG_OPCODE_PARTID_BIT;
    
    hmLog.opCode|=HMLOG_OPCODE_SYS_MASK|
        (XM_HM_EV_INTERNAL_ERROR<<HMLOG_OPCODE_EVENT_BIT);
    
    if (info&&info->sched.cKThread&&ctxt) {
        hmLog.opCode|=HMLOG_OPCODE_VALID_CPUCTXT_MASK;
	CpuCtxt2HmCpuCtxt(ctxt, &hmLog.cpuCtxt);        
    } else
        memset(hmLog.payload, 0, XM_HMLOG_PAYLOAD_LENGTH*sizeof(xmWord_t));

    HmRaiseEvent(&hmLog);

    SmpHaltAll();
    HaltSystem();
}

void PartitionPanic(cpuCtxt_t *ctxt, xm_s8_t *fmt, ...) {
    localProcessor_t *info=GET_LOCAL_PROCESSOR();
    xmHmLog_t hmLog;
    va_list vl = 0;
    ASSERT(fmt);

    kprintf("System PANIC [");

    if (info->sched.cKThread->ctrl.g)
	kprintf("0x%x:id(%d)]:\n", info->sched.cKThread, GetPartitionCfg(info->sched.cKThread)->rId);
    else
	kprintf("0x%x:IDLE]:\n", info->sched.cKThread);

    va_start(vl, fmt);
    vprintf(fmt, vl);
    va_end(vl);
    kprintf("\n");
    if (ctxt)
	DumpState(ctxt);

    if (info->sched.cKThread==info->sched.idleKThread)
	HaltSystem();

    hmLog.opCode=0;
    if (info->sched.cKThread&&info->sched.cKThread->ctrl.g)
	hmLog.opCode=GetPartitionCfg(info->sched.cKThread)->rId<<HMLOG_OPCODE_PARTID_BIT;

    hmLog.opCode|=(XM_HM_EV_PARTITION_UNRECOVERABLE<<HMLOG_OPCODE_EVENT_BIT);

    if (ctxt) {
        hmLog.opCode|=HMLOG_OPCODE_VALID_CPUCTXT_MASK;
	CpuCtxt2HmCpuCtxt(ctxt, &hmLog.cpuCtxt);        
    } else
        memset(hmLog.payload, 0, XM_HMLOG_PAYLOAD_LENGTH*sizeof(xmWord_t));
    HmRaiseEvent(&hmLog);
    
    // Finish current kthread
    SetKThreadFlags(info->sched.cKThread, KTHREAD_HALTED_F);
    if (info->sched.cKThread==info->sched.idleKThread)
	SystemPanic(ctxt, "Idle thread triggered a PANIC???");
    Schedule();
    SystemPanic(ctxt, "[PANIC] This line should not be reached!!!");
}
