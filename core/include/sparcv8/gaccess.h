/*
 * $FILE: gaccess.h
 *
 * Guest shared info
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_ARCH_GACCESS_H_
#define _XM_ARCH_GACCESS_H_

#ifndef _XM_KERNEL_
#error Kernel file, do not include.
#endif

#include <arch/xm_def.h>
#include <irqs.h>
#include <arch/asm.h>

#define _CHECK_BOUNDS(a, b, c, d) (((a)<=(c))&&((b)>=(d)))

#define __archGParam
/*
#define __ArchCheckGPhysAddr(__cfg, __addr, __size) ({ \
    xm_s32_t __e, __r=0; \
    xmAddress_t __a, __b, __c, __d;   \
    __c=(xmAddress_t)(__addr); \
    __d=__c+(xmAddress_t)(__size); \
    for (__e=0; __e<__cfg->noPhysicalMemoryAreas; __e++) { \
        __a=xmcPhysMemAreaTab[__cfg->physicalMemoryAreasOffset+__e].startAddr; \
        __b=__a+xmcPhysMemAreaTab[__cfg->physicalMemoryAreasOffset+__e].size; \
       if ((__c>=__a) &&(__b>__d)) \
            break; \
    } \
    if (__e>=__cfg->noPhysicalMemoryAreas) \
        __r=-1; \
    __r; \
})

#define __ArchCheckGMappedAtAddr(__cfg, __addr, __size) ({ \
    xm_s32_t __e, __r=0; \
    xmAddress_t __a, __b, __c, __d;   \
    __c=(xmAddress_t)(__addr); \
    __d=__c+(xmAddress_t)(__size); \
    for (__e=0; __e<__cfg->noPhysicalMemoryAreas; __e++) { \
        __a=xmcPhysMemAreaTab[__cfg->physicalMemoryAreasOffset+__e].mappedAt; \
        __b=__a+xmcPhysMemAreaTab[__cfg->physicalMemoryAreasOffset+__e].size; \
       if ((__c>=__a) &&(__b>__d)) \
            break; \
    } \
    if (__e>=__cfg->noPhysicalMemoryAreas) \
        __r=-1; \
    __r; \
})
*/
#ifdef CONFIG_MMU
/*
static inline xm_s32_t __MMUCheckMem(xmAddress_t param, xmSize_t size) {
    xmAddress_t addr;
    xm_u8_t tmp;
    for (addr=param; addr<param+size; addr=(addr&PAGE_MASK)+PAGE_SIZE)
        __asm__ __volatile__ ("1:ldub [%1], %0\n\t" \
                              "2:stub %0, [%1]\n\t" \
                              ASM_EXPTABLE(1b) \
                              ASM_EXPTABLE(2b) \
                              : "=r" (tmp) : "r" (addr));    
    return 0;
}

static inline xm_s32_t __ArchCheckGParam(void *param, xmSize_t size) {
    if (!param) return 0;
    if ((((xmAddress_t)param+size)<CONFIG_XM_OFFSET))
        return __MMUCheckMem((xmAddress_t)param, size);
    return -1;
}
*/
/*
#define __ArchCheckGParam(__ctxt, __param, __size) ({\
    xm_s32_t __r=0; \
    if (((xmAddress_t)(__param)>=CONFIG_XM_OFFSET)||(((xmAddress_t)(__param)+(xm_u32_t)(__size))>=CONFIG_XM_OFFSET)) \
        __r=-1; \
    else \
        __r=__MMUCheckMem((xm_u8_t *)__param, __size); \
    __r; \
})
*/
#else

/*
#define __ArchCheckGParam(__ctxt, __param, __size) ({ \
    localSched_t *__sched=GET_LOCAL_SCHED(); \
    struct xmcPartition *__cfg=__sched->cKThread->ctrl.g->cfg; \
    xm_s32_t __e, __r=0, __a, __b; \
    if (__param) { \
	for (__e=0; __e<__cfg->noPhysicalMemoryAreas; __e++) { \
	    __a=xmcPhysMemAreaTab[__cfg->physicalMemoryAreasOffset+__e].startAddr; \
	    __b=__a+xmcPhysMemAreaTab[__cfg->physicalMemoryAreasOffset+__e].size; \
	    if ((((xm_u32_t)(__param))>=__a)&&(((xm_u32_t)(__param)+(xm_u32_t)(__size))<__b)) \
	        break; \
	} \
	if (__e>=__cfg->noPhysicalMemoryAreas) \
	    __r=-1; \
    } \
    __r; \
})
*/
//#define __ArchCheckGParam(__ctxt, __param, __size) ({ 0; })
#endif

#endif
