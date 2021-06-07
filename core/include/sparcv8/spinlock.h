/*
 * $FILE: spinlock.h
 *
 * Spin locks related stuffs
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

#ifndef _XM_ARCH_SPINLOCK_H_
#define _XM_ARCH_SPINLOCK_H_

#ifndef _XM_KERNEL_
#error Kernel file, do not include.
#endif

typedef struct {
    volatile xm_u8_t lock;
} archSpinLock_t;

#define __ARCH_SPINLOCK_UNLOCKED {0}

static inline void __ArchSpinLock(archSpinLock_t *lock) {
#ifdef CONFIG_MULTICORE_SUPPORT
    __asm__ __volatile__("\n1:\n\t"                                     \
                         "ldstuba [%0] 1, %%g2\n\t" /* ASI_LEON23_DCACHE_MISS */ \
                         "orcc  %%g2, 0x0, %%g0\n\t"                    \
                         "bne,a 2f\n\t"                                 \
                         "ldub [%0], %%g2\n\t"                          \
                         ".subsection 2\n"                              \
                         "2:\n\t"                                       \
                         "orcc %%g2, 0x0, %%g0\n\t"                     \
                         "bne,a 2b\n\t"                                 \
                         "ldub [%0], %%g2\n\t"                          \
                         "b,a 1b\n\t"                                   \
                         "nop\n\t"                                      \
                         ".previous\n"                                  \
                         : /* no outputs */                             \
                         : "r" (&lock->lock)                            \
                         : "g2", "memory", "cc");
#endif
}

static inline void __ArchSpinUnlock(archSpinLock_t *lock) {
#ifdef CONFIG_MULTICORE_SUPPORT
    __asm__ __volatile__("stb %%g0, [%0]" : : "r" (&lock->lock) : "memory");    
#endif
}

static inline xm_s32_t __ArchSpinTryLock(archSpinLock_t *lock) {
    xm_u32_t result=0;
#ifdef CONFIG_MULTICORE_SUPPORT
    __asm__ __volatile__("ldstuba [%1] 1, %0" /* ASI_LEON23_DCACHE_MISS */ \
                         : "=r" (result) \
                         : "r" (lock) \
                         : "memory");
#endif
    return (result==0);
}

#define __ArchSpinIsLocked(x) (*(volatile xm_s8_t *)(&(x)->lock)>0)

static inline xm_u32_t GetIpl(void) {
    xm_u32_t retVal = 0U;
    __asm__ __volatile__("rd %%psr, %0" : "=r" (retVal));
    return retVal;
}

#ifndef PSR_PIL_MASK
#define PSR_PIL_MASK 0x00000f00 // Proc interrupt level
#endif

static inline void HwCli(void) {
    xm_u32_t tmp1, tmp2;    
    __asm__ __volatile__("rd %%psr, %0\n\t"  \
			 "or %0, %2, %1\n\t" \
			 "wr %1, 0, %%psr\n\t"                         \
			 "nop; nop; nop\n" : "=&r" (tmp1), "=r" (tmp2) \
			 : "i" (PSR_PIL_MASK) : "memory");

}

static inline void HwSti(void) {
    xm_u32_t tmp;  
    __asm__ __volatile__("rd %%psr, %0\n\t"    \
			 "andn %0, %1, %0\n\t" \
			 "wr %0, 0, %%psr\n\t" \
			 "nop; nop; nop\n" : "=&r" (tmp) : "i" (PSR_PIL_MASK)	\
			 : "memory");
}

static inline xm_u32_t __SaveFlagsCli(void) {
    xm_u32_t retval = 0U;
    xm_u32_t tmp = 0U;
  
    __asm__ __volatile__("rd %%psr, %0\n\t" \
			 "or %0, %2, %1\n\t" \
			 "wr %1, 0, %%psr\n\t" \
			 "nop; nop; nop\n" : "=&r" (retval), "=r" (tmp) \
			 : "i" (PSR_PIL_MASK) : "memory");

    return retval;
}

#define HwSaveFlagsCli(flags) ((flags)=__SaveFlagsCli())

static inline void HwRestoreFlags(xm_u32_t flags) {
    xm_u32_t tmp;
  
    __asm__ __volatile__("rd %%psr, %0\n\t" \
			 "and %2, %1, %2\n\t" \
			 "andn %0, %1, %0\n\t" \
			 "wr %0, %2, %%psr\n\t" \
			 "nop; nop; nop\n" : "=&r" (tmp) \
			 : "i" (PSR_PIL_MASK), "r" (flags) \
			 : "memory");
}

#define HwSaveFlags(flags) ((flags)=GetIpl())

static inline xm_s32_t HwIsSti(void) {
    return !((GetIpl()&PSR_PIL_MASK)==PSR_PIL_MASK);
}

#endif
