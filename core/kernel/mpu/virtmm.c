/*
 * $FILE: virtmm.c
 *
 * Virtual memory manager
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
  - [02/09/2015:SPR-020915-04] Remove VBOOT macro and boot.h
*/
#include <assert.h>

void SetupVirtMM(xmAddress_t sAddr, xmAddress_t eAddr) {
}

xm_s32_t SetupPageTable(kThread_t *k) {
    return 0;
}
