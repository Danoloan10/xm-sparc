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
 *
 */

#ifndef _XM_SPINLOCK_H_
#define _XM_SPINLOCK_H_

#ifndef _XM_KERNEL_
#error Kernel file, do not include.
#endif

#include <arch/spinlock.h>

typedef struct {
    archSpinLock_t archLock;
} spinLock_t;

#define SPINLOCK_INIT (spinLock_t){.archLock=__ARCH_SPINLOCK_UNLOCKED,}

static inline void SpinLock(spinLock_t *s) {
    __ArchSpinLock(&s->archLock);
}

static inline void SpinUnlock(spinLock_t *s) {
    __ArchSpinUnlock(&s->archLock);
}

static inline xm_s32_t SpinIsLocked(spinLock_t *s) {
    return(xm_s32_t)__ArchSpinIsLocked(&s->archLock);
}

#define SpinLockIrqSave(s, flags) do {		\
    HwSaveFlagsCli(flags);			\
    SpinLock(s);				\
} while(0)

#define SpinUnlockIrqRestore(s, flags) do {	\
    SpinUnlock(s);				\
    HwRestoreFlags(flags);			\
} while(0)

static inline xm_s32_t SpinTryLock(spinLock_t *s) {
    return __ArchSpinTryLock(&s->archLock);
}

#endif
