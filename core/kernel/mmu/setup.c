/*
 * $FILE: setup.c
 *
 * Setting up and starting up the kernel
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <assert.h>
#include <stdc.h>
#include <processor.h>
#include <sched.h>
#include <kthread.h>
#include <vmmap.h>
#include <virtmm.h>

#include <local.h>

__NOINLINE void FreeBootMem(void) {
    extern void IdleTask(void);
    ASSERT(!HwIsSti());
    GET_LOCAL_PROCESSOR()->sched.flags|=LOCAL_SCHED_ENABLED;
    Schedule();
    IdleTask();
}
