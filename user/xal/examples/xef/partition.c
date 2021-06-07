/*
 * $FILE: partition.c
 *
 * Fent Innovative Software Solutions
 *
 * $LICENSE:
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <string.h>
#include <stdio.h>
#include <xm.h>
#include <irqs.h>

#include <xef.h>

#define NULL ((void *)0)

#define PRINT(...) do { \
        printf("[P%d] ", XM_PARTITION_SELF); \
        printf(__VA_ARGS__); \
    } while (0)

static void *_Memcpy(void *dest, const void *src, unsigned long n) {
    //printf("dest %x src: %x %d\n", dest, src, n);
    //printf("%d\n", 
    XM_memory_copy(UNCHECKED_ID, (xm_u32_t)dest, UNCHECKED_ID, (xm_u32_t)src, n);
}

void PartitionMain(void)
{
    struct xmImageHdr *partHdr, *partHdrCf;
    struct xefFile xefFile, xefCfFile;
    /* 
       xefCfInfo point out to the cfBuffer within partition1
       The address _MUST_ updated every time a change is performed in partition1
     */
    struct xefCustomFile xefCfInfo = {.sAddr = (xmAddress_t) 0x40085760}; 
    struct xefOps xefOps;
    extern xm_u8_t sXef[], sXefCf[];    
    int ret;
    xefOps.Write=_Memcpy;
    xefOps.Read=memcpy;
    xefOps.VAddr2PAddr=0;
    if (XEF_parse_file(sXef, &xefFile, &xefOps)==XEF_OK) {
        if ((partHdr=XEF_load_file(&xefFile))!=NULL)
        {
            if ((XEF_parse_file(sXefCf, &xefCfFile, &xefOps) == XEF_OK) &&
                (XEF_load_custom_file(&xefCfFile, &xefCfInfo) != NULL))
            {
                printf("OK - Reset: %d\n", XM_reset_partition(1, -1, 0, 0));
                
            }
            else
            {
                printf("FAIL\n");
            }
        }
    }

    XM_halt_partition(XM_PARTITION_SELF);
}
