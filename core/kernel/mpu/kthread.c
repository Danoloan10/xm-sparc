/*
 * $FILE: kthread.c
 *
 * Kernel and Guest context
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
  Changelog
  - [02/09/2015:SPR-020915-04] Remove boot.h
*/
#include <assert.h>
#include <kthread.h>
#include <sched.h>
#include <stdc.h>
#include <vmmap.h>
#include <xmef.h>

void FillMmPartCtrlTabWarm(partitionControlTable_t *partCtrlTab, kThread_t *k) {
}

void FillMmPartCtrlTabCold(partitionControlTable_t *partCtrlTab, kThread_t *k) {
}
