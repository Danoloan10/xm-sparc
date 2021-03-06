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

#ifndef _XAL_ASSERT_H_
#define _XAL_ASSERT_H_

#ifdef _DEBUG_

#include <xal.h>

#define assert(exp)  \
    ((exp)?0:LHalt(__FILE__":%u: failed assertion `"#exp"'\n", __LINE__))

#define ASSERT(exp) assert(exp)

#else

#define assert(exp) ((void)0)

#endif

#endif
