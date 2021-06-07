/*
 * $FILE: hypervisor.h
 *
 * hypervisor management functions
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _LIB_XM_HYPERVISOR_H_
#define _LIB_XM_HYPERVISOR_H_

#include <xm_inc/config.h>
#include <xm_inc/xmconf.h>
#include <arch/hypervisor.h>

#include <xm_inc/linkage.h>

#define XM_PARTITION_SELF (XM_params_get_PCT()->id)

/* Partition management */
extern __stdcall xm_s32_t XM_write_console(char *buffer, xm_s32_t length);
extern __stdcall xm_s32_t XM_memory_copy(xmId_t destId, xm_u32_t destAddr, xmId_t srcId,  xm_u32_t srcAddr, xm_u32_t size);

/* Interrupt management */
//extern __stdcall xm_s32_t XM_are_irqs_enabled(void);
extern __stdcall xm_s32_t XM_mask_irq(xm_u32_t noIrq);
extern __stdcall xm_s32_t XM_unmask_irq(xm_u32_t noIrq);
//extern __stdcall xm_s32_t XM_clear_irq(xm_u32_t noIrq);
//extern __stdcall xm_s32_t XM_set_irqpend(xm_u32_t noIrq);

#endif
