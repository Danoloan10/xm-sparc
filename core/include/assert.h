/*
 * $FILE: assert.h
 *
 * Assert definition
 *
 * $VERSION$
 * 
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_ASSERT_H_
#define _XM_ASSERT_H_

#ifndef _XM_KERNEL_
#error Kernel file, do not include.
#endif

#include <processor.h>

#ifdef CONFIG_ASSERT

#define ASSERT(exp) do { \
    cpuCtxt_t _ctxt; \
    if (!(exp)) { \
        GetCpuCtxt(&_ctxt); \
        SystemPanic(&_ctxt, __XM_FILE__":%u: failed assertion `"#exp"'\n", __LINE__); \
    } \
} while(0)

#define ASSERT_LOCK(exp, lock) do { \
    if (!(exp)) { \
        cpuCtxt_t _ctxt; \
        SpinUnlock((lock)); \
        GetCpuCtxt(&_ctxt); \
        SystemPanic(&_ctxt, __XM_FILE__":%u: failed assertion `"#exp"'\n", __LINE__); \
    } \
    ((void)0); \
} while(0)

#else

#define ASSERT(exp) ((void)0)
#define ASSERT_LOCK(exp, sLock) ((void)0)

#endif

#endif
