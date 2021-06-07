/*
 * $FILE: xmhypercalls.h
 *
 * Generic hypercall definition
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 *
 * [30/03/2016]     Ángel Esquinas (aesquinas@fentiss.com)
 * CP-250216-01:
 *     Add "XM_reload_watchdog" hypercall.
 */

#ifndef _LIB_XM_HYPERCALLS_H_
#define _LIB_XM_HYPERCALLS_H_

#ifdef _XM_KERNEL_
#error Guest file, do not include.
#endif

#include <xm_inc/config.h>
#include <xm_inc/linkage.h>
#include <xm_inc/hypercalls.h>
#include <xm_inc/arch/irqs.h>
#include <xm_inc/arch/linkage.h>
#include <arch/xmhypercalls.h>

#ifndef __ASSEMBLY__
#include <xm_inc/arch/arch_types.h>
#include <xm_inc/objdir.h>

/* <track id="hypercall-list"> */

extern __stdcall xm_s32_t XM_get_gid_by_name(xm_u8_t *name, xm_u32_t entity);
extern __stdcall xmId_t XM_get_vcpuid(void);

// Time management hypercalls
extern __stdcall xm_s32_t XM_get_time(xm_u32_t clock_id, xmTime_t *time);
extern __stdcall xm_s32_t XM_set_timer(xm_u32_t clock_id, xmTime_t abstime, xmTime_t interval);
extern __stdcall xm_s32_t XM_reload_watchdog(xmTime_t budget);

// Partition status hypercalls
extern __stdcall xm_s32_t XM_suspend_partition(xm_u32_t partition_id);
extern __stdcall xm_s32_t XM_resume_partition(xm_u32_t partition_id);
extern __stdcall xm_s32_t XM_shutdown_partition(xm_u32_t partition_id);
extern __stdcall xm_s32_t XM_reset_partition(xm_u32_t partition_id, xmAddress_t ePoint, xm_u32_t resetMode, xm_u32_t status);
extern __stdcall xm_s32_t XM_halt_partition(xm_u32_t partition_id);
extern __stdcall xm_s32_t XM_idle_self(void);

extern __stdcall xm_s32_t XM_reset_vcpu(xm_u32_t vcpu_id, xmAddress_t entry);
extern __stdcall xm_s32_t XM_halt_vcpu(xm_u32_t vcpu_id);

// system status hypercalls
extern __stdcall xm_s32_t XM_halt_system(void);
extern __stdcall xm_s32_t XM_reset_system(xm_u32_t resetMode);

// Object related hypercalls
//@ \void{<track id="object-hc">}
extern __stdcall xm_s32_t XM_read_object(xmObjDesc_t objDesc, void *buffer, xm_u32_t size, xm_u32_t *flags);
extern __stdcall xm_s32_t XM_write_object(xmObjDesc_t objDesc, void *buffer, xm_u32_t size, xm_u32_t *flags);
extern __stdcall xm_s32_t XM_seek_object(xmObjDesc_t objDesc, xm_u32_t offset, xm_u32_t whence);
extern __stdcall xm_s32_t XM_ctrl_object(xmObjDesc_t objDesc, xm_u32_t cmd, void *arg);
//@ \void{</track id="object-hc">}

// Hw interrupt management

extern __stdcall void XM_disable_irqs(void);
extern __stdcall void XM_enable_irqs(void);
extern __stdcall xm_s32_t XM_override_trap_hndl(xm_s32_t entry, struct trapHandler *);
extern __stdcall xm_s32_t XM_clear_irqmask(xm_u32_t hwIrqsMask, xm_u32_t extIrqsMask);
extern __stdcall xm_s32_t XM_set_irqmask(xm_u32_t hwIrqsMask, xm_u32_t extIrqsMask);
extern __stdcall xm_s32_t XM_set_irqpend(xm_u32_t hwIrqPend, xm_u32_t extIrqPend);
extern __stdcall xm_s32_t XM_clear_irqpend(xm_u32_t hwIrqPend, xm_u32_t extIrqPend);
extern __stdcall xm_s32_t XM_route_irq(xm_u32_t type, xm_u32_t irq, xm_u16_t vector);

extern __stdcall xm_s32_t XM_raise_ipvi(xm_u8_t no_ipvi);

extern __stdcall xm_s32_t XM_set_cache_state(xm_u32_t cache, xm_u32_t action);

extern __stdcall xm_s32_t XM_switch_sched_plan(xm_u32_t newPlanId, xm_u32_t *currentPlanId);

/* </track id="hypercall-list"> */
#endif

#endif
