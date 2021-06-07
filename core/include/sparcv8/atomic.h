/*
 * $FILE: atomic.h
 *
 * atomic operations
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_ARCH_ATOMIC_H_
#define _XM_ARCH_ATOMIC_H_

typedef struct { 
    volatile xm_u32_t val; 
} xmAtomic_t;

#ifdef _XM_KERNEL_
#define XMAtomicSet(v,i) (((v)->val) = (i))
#define XMAtomicGet(v) ((v)->val)
#define XMAtomicClearMask(mask, v) ((v)->val&=~(mask))
#define XMAtomicSetMask(mask, v) ((v)->val|=(mask))

static inline void XMAtomicInc(xmAtomic_t *v) {
    v->val++;
}

static inline void XMAtomicDec(xmAtomic_t *v) {
    v->val--;
}

static inline xm_s32_t XMAtomicDecAndTest(xmAtomic_t *v) {
    return ((--(v->val))!=0);
}
#endif
#endif
