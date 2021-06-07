/*
 * $FILE: smp.h
 *
 * SMP related stuff
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_SMP_H_
#define _XM_SMP_H_

#ifndef _XM_KERNEL_
#error Kernel file, do not include.
#endif

#include <linkage.h>
#include <arch/smp.h>
#include <spinlock.h>

extern xm_u16_t __nrCpus;

#define GET_NRCPUS() __nrCpus
#define SET_NRCPUS(nrCpu) __nrCpus=(nrCpu)

extern xm_s32_t InitSmp(void);
extern void SetupSmp(void);

#ifndef CONFIG_SMP
#define SmpFlushTlb()
//#define SmpRemoveKThread(cpunr)
#define SmpHaltAll()
#endif

#endif
