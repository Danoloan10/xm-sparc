/*
 * $FILE: list.h
 *
 * List
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_LIST2_H_
#define _XM_LIST2_H_

#ifndef __ASSEMBLY__
#include <assert.h>

struct dynListR;

struct dynListNodeR {
    xmAddress_t listR, prevR, nextR;
};

struct dynListR {
    xmAddress_t headR;
    xm_s32_t noElem;
};

#define REL2ABS(p, base) ((struct dynListNodeR *)((p)+(base)))

static inline void DynListInitR(struct dynListR *l) {
    l->noElem=0;
    l->headR=0;
}

static inline xm_s32_t DynListInsertHeadR(xmAddress_t base, struct dynListR *l, struct dynListNodeR *e) {
    if (e->listR) {
	ASSERT((struct dynListR *)(e->listR+base)==l);
	return 0;
    }
    ASSERT(!e->nextR&&!e->prevR);
    if (l->headR) {
	ASSERT(l->noElem>0);
	e->nextR=l->headR;
	e->prevR=REL2ABS(l->headR, base)->prevR;
        REL2ABS(REL2ABS(l->headR, base)->prevR, base)->nextR=(xmAddress_t)e-base;
        REL2ABS(l->headR, base)->prevR=(xmAddress_t)e-base;
    } else {	
	ASSERT(!l->noElem);
	e->prevR=e->nextR=(xmAddress_t)e-base;
    }
    l->headR=(xmAddress_t)e-base;
    ASSERT(l->noElem>=0);
    l->noElem++;
    e->listR=(xmAddress_t)l-base;
    return 0;
}

static inline void *DynListRemoveTailR(xmAddress_t base, struct dynListR *l) {
    struct dynListNodeR *e=0;
    if (l->headR) {
        e=REL2ABS(REL2ABS(l->headR, base)->prevR, base);
        REL2ABS(e->prevR, base)->nextR=e->nextR;
        REL2ABS(e->nextR, base)->prevR=e->prevR;
        e->prevR=e->nextR=0;
	e->listR=0;
	l->noElem--;
	if (!l->noElem)
	    l->headR=0;
    }
    ASSERT(l->noElem>=0);

    return e;
}

#endif
#endif
