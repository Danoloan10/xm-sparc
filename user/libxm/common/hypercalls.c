/*
 * $FILE: hypercalls.c
 *
 * XM system calls definitions
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <xmhypercalls.h>
#include <xm_inc/hypercalls.h>
#include <hypervisor.h>

void XM_disable_irqs(void) {
    XM_sparc_set_pil();
}

void XM_enable_irqs(void) {
    /* SPR-150115-01 fix */
    XM_sparc_set_psr((XM_sparc_get_psr()|PSR_ET_BIT)&~PSR_PIL_MASK);
}
