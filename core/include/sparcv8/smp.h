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

#ifndef _XM_ARCH_SMP_H_
#define _XM_ARCH_SMP_H_

#ifndef _XM_KERNEL_
#error Kernel file, do not include.
#endif

#include <linkage.h>

extern void SendIpi(xm_u32_t cpuId, xm_u32_t msg);
extern void SendAllIpi(xm_u32_t msg);
#define HALT_SYSTEM_IPI_MSG 0
#define RESCHED_SYSTEM_IPI_MSG 1
#define WARM_RESET_SYSTEM_IPI_MSG 2
#define COLD_RESET_SYSTEM_IPI_MSG 3

extern void WakeUpCpu(xm_s32_t cpu);

extern void SetupLocalSmp(void);

extern xm_u32_t __GetCpuId(void);
extern xm_u32_t __GetCpuHwId(void);
extern void __SetCpuId(xm_u32_t);
extern void __SetCpuHwId(xm_u32_t);

#define GET_CPU_ID() __GetCpuId()
#define GET_CPU_HWID() __GetCpuHwId()

#endif
