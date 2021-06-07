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

#define DIV(a, b, c) __asm__ __volatile__ ("udiv %1, %2, %0\n\t" : "=r" (c) : "r" (a), "r" (b))

#define PRINT(...) do { \
        printf("[P%d] ", XM_PARTITION_SELF); \
        printf(__VA_ARGS__); \
} while (0)

#define No_WARM_RESETS 2

xm_s32_t control = 1;
xm_u32_t excRet;

void DivideExceptionHandler(trapCtxt_t *ctxt)                                       //XAL trap API
{
    xm_u32_t partStatus = XM_WARM_RESET;
    partitionControlTable_t * partitionControlTable;
    
    partitionControlTable=XM_params_get_PCT();
    
    if (partitionControlTable->resetCounter < No_WARM_RESETS)
    {
        PRINT("Divide by zero exception propagated and handled. A warm reset of the partition action will be performed.\n");
        XM_reset_partition(XM_PARTITION_SELF, -1, XM_WARM_RESET, partStatus);
    }
    else
    {
        PRINT("Divide Exception propagated and handled. A halt partition action will be performed.\n");
        XM_halt_partition(XM_PARTITION_SELF);
    }
}

void PartitionMain(void)
{
    volatile xm_s32_t val = 0;
    int D, d, r;
    
    PRINT("Starting example 002...\n");
    XM_idle_self();

#ifdef CONFIG_SPARCv8
    InstallTrapHandler(DIVISION_BY_ZERO, DivideExceptionHandler); //Install divide by zero exception handler
#endif

    PRINT("Dividing by zero...\n");
    D = 1;
    d = 0;
    //Forces
    DIV(D,d,r);

    PRINT("This should not be executed!\n");
    XM_halt_partition(XM_PARTITION_SELF);
}
