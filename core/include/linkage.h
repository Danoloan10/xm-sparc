/*
 * $FILE: linkage.h
 *
 * Definition of some macros to ease the interoperatibility between
 * assembly and C
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
  Changelog:
  - [02/09/2015:SPR-020915-05] Remove WARN_UNUSED_RESULT macro
*/
#ifndef _XM_LINKAGE_H_
#define _XM_LINKAGE_H_

#ifdef _XM_KERNEL_
#include <arch/linkage.h>
#include <arch/paging.h>
#else
#include <xm_inc/arch/linkage.h>
#include <xm_inc/arch/paging.h>
#endif

#define PAGE_ALIGN .align PAGE_SIZE

#ifndef _ASSEMBLY_
#define __NOINLINE __attribute__((noinline))
#define __PACKED __attribute__((__packed__))
#endif

#define SYMBOL_NAME(X) X
#define SYMBOL_NAME_LABEL(X) X##:

#define ENTRY(name) \
    .globl SYMBOL_NAME(name); \
    ASM_ALIGN; \
    SYMBOL_NAME_LABEL(name)

#define __STR(x) #x
#define TO_STR(x) __STR(x)

#endif
