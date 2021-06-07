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

#ifndef _XM_LIST_H_
#define _XM_LIST_H_

#ifndef __ASSEMBLY__
#include <assert.h>

struct dynList;

struct dynListNode {
    struct dynList *list;
    struct dynListNode *prev, *next;
};

struct dynList {
    struct dynListNode *head;
    xm_s32_t noElem;
    spinLock_t lock;
};

static inline void DynListInit(struct dynList *l) {
    l->lock=SPINLOCK_INIT;
#ifdef CONFIG_SMP_SUPPORT
    SpinLock(&l->lock);
#endif
    l->noElem=0;
    l->head=0;
#ifdef CONFIG_SMP_SUPPORT
    SpinUnlock(&l->lock);
#endif
}

static inline xm_s32_t DynListInsertHead(struct dynList *l, struct dynListNode *e) {
    if (e->list) {
	ASSERT(e->list==l);
	return 0;
    }
    ASSERT(!e->next&&!e->prev);
#ifdef CONFIG_SMP_SUPPORT
    SpinLock(&l->lock);
#endif
    if (l->head) {
        ASSERT_LOCK(l->noElem>0, &l->lock);
//	ASSERT(l->noElem>0);
	e->next=l->head;
	e->prev=l->head->prev;
	l->head->prev->next=e;
	l->head->prev=e;	
    } else {	
        ASSERT_LOCK(!l->noElem, &l->lock);
	//ASSERT(!l->noElem);
	e->prev=e->next=e;
    }
    l->head=e;
    l->noElem++;
    e->list=l;
#ifdef CONFIG_SMP_SUPPORT
    SpinUnlock(&l->lock);
#endif
    ASSERT(l->noElem>=0);
    return 0;
}

static inline xm_s32_t DynListInsertTail(struct dynList *l, struct dynListNode *e) {
    if (e->list) {
	ASSERT(e->list==l);
	return 0;
    }
    ASSERT(!e->next&&!e->prev);
#ifdef CONFIG_SMP_SUPPORT
    SpinLock(&l->lock);
#endif
    if (l->head) {
	e->next=l->head;
	e->prev=l->head->prev;
	l->head->prev->next=e;
	l->head->prev=e;	
    } else {
	e->prev=e->next=e;
	l->head=e;
    }
    l->noElem++;
    e->list=l;
#ifdef CONFIG_SMP_SUPPORT
    SpinUnlock(&l->lock);
#endif
    ASSERT(l->noElem>=0);
    return 0;
}

static inline void *DynListRemoveHead(struct dynList *l) {
    struct dynListNode *e=0;
#ifdef CONFIG_SMP_SUPPORT
    SpinLock(&l->lock);
#endif
    if (l->head) {
	e=l->head;
	l->head=e->next;
	e->prev->next=e->next;
	e->next->prev=e->prev;
	e->prev=e->next=0;
	e->list=0;
	l->noElem--;
	if (!l->noElem)
	    l->head=0;
    }
#ifdef CONFIG_SMP_SUPPORT
    SpinUnlock(&l->lock);
#endif
    ASSERT(l->noElem>=0);

    return e;
}

static inline void *DynListRemoveTail(struct dynList *l) {
    struct dynListNode *e=0;
#ifdef CONFIG_SMP_SUPPORT
    SpinLock(&l->lock);
#endif
    if (l->head) {
	e=l->head->prev;
	e->prev->next=e->next;
	e->next->prev=e->prev;
	e->prev=e->next=0;
	e->list=0;
	l->noElem--;
	if (!l->noElem)
	    l->head=0;
    }
#ifdef CONFIG_SMP_SUPPORT
    SpinUnlock(&l->lock);
#endif
    ASSERT(l->noElem>=0);

    return e;
}

static inline xm_s32_t DynListRemoveElement(struct dynList *l, struct dynListNode *e) {
    ASSERT(e->list==l);
    ASSERT(e->prev&&e->next);
#ifdef CONFIG_SMP_SUPPORT
    SpinLock(&l->lock);
#endif
    e->prev->next=e->next;
    e->next->prev=e->prev;
    if (l->head==e)
	l->head=e->next;
    e->prev=e->next=0;
    e->list=0;
    l->noElem--;
    if (!l->noElem)
	l->head=0;
#ifdef CONFIG_SMP_SUPPORT
    SpinUnlock(&l->lock);
#endif
    ASSERT(l->noElem>=0);

    return 0;
}

#define DYNLIST_FOR_EACH_ELEMENT_BEGIN(_l, _element, _cond) do { \
    xm_s32_t __e; \
    struct dynListNode *__n; \
    for (__e=(_l)->noElem, __n=(_l)->head, _element=(void *)__n; __e && (_cond); __e--, __n=__n->next, _element=(void *)__n) {


#define DYNLIST_FOR_EACH_ELEMENT_END(_l) \
    } \
} while(0)

#define DYNLIST_FOR_EACH_ELEMENT_EXIT(_l)

#endif
#endif
