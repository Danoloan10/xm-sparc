/*
 * $FILE: xm_def.h
 *
 * XM's SPARCv8 hardware configuration
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_ARCH_XMDEF_H_
#define _XM_ARCH_XMDEF_H_

/* <track id="DOC_SYSCALL_TRAP_NR"> */
#define XM_HYPERCALL_TRAP 0xF0
#define XM_ASMHYPERCALL_TRAP 0xF1
/* </track id="DOC_SYSCALL_TRAP_NR"> */

#define XM_FPU_REG_NR (32+1)
#define NR_PPAG 4

#if defined(CONFIG_LEON2) || defined(CONFIG_LEON3) || defined(CONFIG_LEON3FT) || defined(CONFIG_LEON4)
#ifdef _XM_KERNEL_
#include <arch/leon.h>
#else
#include <xm_inc/arch/leon.h>
#endif
#else
#error Processor not valid
#endif

#define __ARCH_MEMCPY


#endif
