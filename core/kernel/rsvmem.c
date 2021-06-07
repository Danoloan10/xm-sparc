/*
 * $FILE: rsvmem.c
 *
 * Memory for structures
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <xmconf.h>
#include <assert.h>
#include <processor.h>

void InitRsvMem(void) {
    xm_s32_t e;
    for (e=0; xmcRsvMemTab[e].obj; e++)
	xmcRsvMemTab[e].usedAlign&=~RSV_MEM_USED;
}

void *AllocRsvMem(xm_u32_t size, xm_u32_t align) {
    xm_s32_t e;
    for (e=0; xmcRsvMemTab[e].obj; e++) {        
	if (!(xmcRsvMemTab[e].usedAlign&RSV_MEM_USED)&&((xmcRsvMemTab[e].usedAlign&~RSV_MEM_USED)==align)&&(xmcRsvMemTab[e].size==size)) {
            xmcRsvMemTab[e].usedAlign|=RSV_MEM_USED;
	    return (void *)((xmAddress_t)xmcRsvMemTab[e].obj+(xmAddress_t)&xmcTab);
	}
    }
    return 0;
}



