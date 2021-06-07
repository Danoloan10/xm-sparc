/*
 * $FILE: config.h
 *
 * Config file
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 *
 * Changelog:
 *   - [09/12/2016:CP-161209-01] Remove CONFIG_MAX_CUSTOM_FILES. It is defined in the menuconfig.
 */

#ifndef _XM_CONFIG_H_
#define _XM_CONFIG_H_

#ifdef _XM_KERNEL_
#include <autoconf.h>
#else
#include <xm_inc/autoconf.h>
#endif

#ifdef ASM
#define __ASSEMBLY__
#endif

// bits: (31..24)(23..16)(15..8)(7..0)
// Reserved.VERSION.SUBVERSION.REVISION
#define XM_VERSION (((CONFIG_XM_VERSION&0xFF)<<16)|((CONFIG_XM_SUBVERSION&0xFF)<<8)|(CONFIG_XM_REVISION&0xFF))

#define CONFIG_KSTACK_SIZE (CONFIG_KSTACK_KB*1024)

#ifndef CONFIG_NO_CPUS
#define CONFIG_NO_CPUS 1
#endif

#if (CONFIG_ID_STRING_LENGTH&3)
#error CONFIG_ID_STRING_LENGTH must be a power of 4 (log2(32))
#endif

#if !defined(CONFIG_HWIRQ_PRIO_FBS)&&!defined(CONFIG_HWIRQ_PRIO_LBS)
#error "Interrupt priority order must be defined"
#endif

//#if ((CONFIG_XM_OFFSET^CONFIG_XM_LOAD_ADDR)&((16*1024*1024)-1))
//#error "XtratuM physical and virtual offsets shall match"
//#endif

#ifdef __BASE_FILE__
#define __XM_FILE__ __BASE_FILE__
#else
#define __XM_FILE__ __FILE__
#endif

#endif
