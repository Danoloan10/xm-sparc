/*
 * $FILE: vmmap_mpu.c
 *
 * Virtual memory map management
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
#include <rsvmem.h>
#include <kthread.h>
#include <stdc.h>
#include <vmmap.h>
#include <virtmm.h>
#include <physmm.h>
#include <arch/xm_def.h>
#include <arch/processor.h>
#include <arch/paging.h>
#include <arch/physmm.h>

xmAddress_t VmMapXm(xmAddress_t pAddr, xmAddress_t vAddr, xmSize_t size, xm_u32_t flags)  {
    return vAddr+size;
}
