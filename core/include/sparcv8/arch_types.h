/*
 * $FILE: arch_types.h
 *
 * Types defined by the architecture
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_ARCH_TYPES_H_
#define _XM_ARCH_TYPES_H_

#ifndef __ASSEMBLY__
/*  <track id="sparc-basic-types"> */
// Basic types
typedef unsigned char xm_u8_t;
typedef char xm_s8_t;
typedef unsigned short xm_u16_t;
typedef short xm_s16_t;
typedef unsigned int xm_u32_t;
typedef int xm_s32_t;
typedef unsigned long long xm_u64_t;
typedef long long xm_s64_t;
/*  </track id="sparc-basic-types"> */

/*  <track id="sparc-extended-types"> */
// Extended types
typedef long xmLong_t;
typedef xm_u32_t xmWord_t;
#define XM_LOG2_WORD_SZ 5
typedef xm_s64_t xmTime_t;
#define MAX_XMTIME 0x7fffffffffffffffLL
typedef xm_u32_t xmAddress_t;
typedef xmAddress_t xmIoAddress_t;
typedef xm_u32_t xmSize_t;
typedef xm_s32_t xmSSize_t;
typedef xm_u32_t xmId_t;
/*  </track id="sparc-extended-types"> */

#define PRNT_ADDR_FMT "l"
#define PTR2ADDR(x) ((xmWord_t)x)
#define ADDR2PTR(x) ((void *)((xmWord_t)x))

#ifdef _XM_KERNEL_

// Extended internal types
typedef xm_s64_t hwTime_t;

#endif

#endif
#endif
