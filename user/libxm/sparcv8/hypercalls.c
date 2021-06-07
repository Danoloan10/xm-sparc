/*
 * $FILE: hypercalls.c
 *
 * XM system calls definitions
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
  - [30/03/2016: CP-250216-01] Add XM_reload_watchdog hypercall
*/

#include <xm.h>

__stdcall xmId_t XM_get_vcpuid(void) {
    xm_s32_t _r = -1;
    _XM_HCALL0(get_vcpuid_nr, _r);
    return _r;
}

xm_hcall1r(halt_vcpu, xm_u32_t, vcpuId);
xm_hcall2r(reset_vcpu, xm_u32_t, vcpuId, xmAddress_t, entry);

xm_hcall0r(halt_system);
xm_hcall1r(reset_system, xm_u32_t, resetMode);
xm_hcall1r(halt_partition, xm_u32_t, partitionId);
xm_hcall1r(suspend_partition, xm_u32_t, partitionId);
xm_hcall1r(resume_partition, xm_u32_t, partitionId);
xm_hcall4r(reset_partition, xm_u32_t, partitionId, xmAddress_t, ePoint, xm_u32_t, resetMode, xm_u32_t, status);
xm_hcall1r(shutdown_partition, xm_u32_t, partitionId);
xm_hcall0r(idle_self);
xm_hcall4r(read_object, xmObjDesc_t, objDesc, void *, buffer, xm_u32_t, size, xm_u32_t *, flags);
xm_hcall4r(write_object, xmObjDesc_t, objDesc, void *, buffer, xm_u32_t, size, xm_u32_t *, flags);
xm_hcall3r(ctrl_object, xmObjDesc_t, objDesc, xm_u32_t, cmd, void *, arg);
xm_hcall3r(seek_object, xmObjDesc_t, traceStream, xm_u32_t, offset, xm_u32_t, whence);
xm_hcall2r(get_time, xm_u32_t, clock_id, xmTime_t *, time);

xm_hcall2r(clear_irqmask, xm_u32_t, hwIrqsMask, xm_u32_t, extIrqsPend);
xm_hcall2r(set_irqmask, xm_u32_t, hwIrqsMask, xm_u32_t, extIrqsPend);
xm_hcall2r(set_irqpend, xm_u32_t, hwIrqMask, xm_u32_t, extIrqMask);
xm_hcall2r(clear_irqpend, xm_u32_t, hwIrqMask, xm_u32_t, extIrqMask);
xm_hcall3r(route_irq, xm_u32_t, type, xm_u32_t, irq, xm_u16_t, vector);
xm_hcall1r(raise_ipvi, xm_u8_t, no_ipvi);

xm_hcall2r(set_cache_state, xm_u32_t, cache, xm_u32_t, action);
xm_hcall2r(switch_sched_plan, xm_u32_t, newPlanId, xm_u32_t *, currentPlanId);

xm_hcall2r(get_gid_by_name, xm_u8_t *, name, xm_u32_t, entity);

ASMLINK xm_s32_t XM_set_timer(xm_u32_t clock_id, xmTime_t abstime, xmTime_t interval) {
    xm_s32_t _r = -1;
    _XM_HCALL5(clock_id, abstime, abstime<<32, interval, interval<<32, set_timer_nr, _r);
    return _r;
}

ASMLINK xm_s32_t XM_reload_watchdog(xmTime_t budget)
{
    xm_s32_t _r = -1;
    _XM_HCALL2(budget, budget << 32, reload_watchdog_nr, _r);
    return _r;
}

xm_hcall2r(sparc_inport, xm_u32_t, port, xm_u32_t *, value);
xm_hcall2r(sparc_outport, xm_u32_t, port, xm_u32_t, value);
xm_hcall3r(sparc_outport_sdp, xm_u8_t *, dst, xm_u8_t *, src, xm_u32_t, size);
xm_hcall1r(sparc_disable_sdp, xm_u32_t, chipSelect);
#ifdef CONFIG_SPW_RTC
xm_hcall3r(sparc_outport_msg, xm_u32_t, port, struct ioOutPortMsg *, ioMsg, xm_u32_t, timeout);
#endif
xm_hcall1(sparc_write_tbr, xmWord_t, val);
