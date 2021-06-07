/*
 * $FILE: physmm.h
 *
 * Physical memory manager
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_ARCH_PHYSMM_H_
#define _XM_ARCH_PHYSMM_H_

// region type 
#define PMM_STRAM_TYPE XM_MEM_REG_STRAM
#define PMM_SDRAM_TYPE XM_MEM_REG_SDRAM

#ifdef _XM_KERNEL_
#include <arch/leon.h>
#endif

#endif
