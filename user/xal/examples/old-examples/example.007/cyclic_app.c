/* $Example 007
 * $FILE: partition1.c
 *
 * Cyclic scheduling test at partition level
 *
 * $VERSION$
 *
 * Author: Alfons Crespo
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

#include <stdio.h>
#include <xm.h>
#include <irqs.h>

#define FIRSTSLOTTAG 1
int nSlot = 0;
volatile int nMAF = 0;
static long long current_clock, told1, told2,told3 ;
int cnt1, cnt2, cnt3 = 0;

void TrapHandler(trapCtxt_t *ctxt) {
    partitionControlTable_t *controlTable;
    controlTable = XM_params_get_PCT();

    XM_get_time(XM_HW_CLOCK, &current_clock);
    if (controlTable->schedInfo.noSlot == FIRSTSLOTTAG)
        nMAF++;
    nSlot = controlTable->schedInfo.id;
}


void task1(void) {
   int i,j;
   long long t0;
    XM_get_time(XM_HW_CLOCK, &t0);
    printf("[P%d][Task1]: interval=%d\n", XM_PARTITION_SELF, (long) (t0-told1));
    told1 = t0;
    for (i=0; i<10; i++) {
        for (j=0; j<10; j++) {
            cnt1 = cnt1 + 1;
        }
    }
}

void task2(void) {
   int i,j;
   long long t0;
    XM_get_time(XM_HW_CLOCK, &t0);
    printf("[P%d][Task2]: interval=%d\n", XM_PARTITION_SELF, (long) (t0-told2));

    told2 = t0;
    for (i=0; i<10; i++) {
        for (j=0; j<10; j++) {
            cnt2 = cnt2 + 1;
        }
    }
}

void task3(void) {
   int i,j;
   long long t0;
    XM_get_time(XM_HW_CLOCK, &t0);
    printf("[P%d][Task3]: interval=%d\n", XM_PARTITION_SELF, (long) (t0-told3));
    told3 = t0;
    for (i=0; i<10; i++) {
        for (j=0; j<10; j++) {
            cnt3 = cnt3 + 1;
        }
    }
}


void PartitionMain(void) {
    long long t0, t1;
    
    printf("[P%d] Example 007\n",XM_PARTITION_SELF);

    SetupIrqs();
    InstallTrapHandler(XM_VT_EXT_CYCLIC_SLOT_START_TTAB, TrapHandler);
    XM_clear_irqmask(0, (1<<XM_VT_EXT_CYCLIC_SLOT_START));
    XM_sparc_clear_pil();

    while (1) {
        switch (nSlot) {
            case 0: task1(); task2(); break;
            case 1: task1(); task3(); break;
            case 2: task1(); break;
            case 3: task1(); break;
            default : break;
        }

        printf("[P%d] MAF: %d MIF: %d cnt1: %d; cnt2: %d; cnt3: %d\n",XM_PARTITION_SELF, nMAF, nSlot, cnt1,cnt2,cnt3);
        XM_idle_self();
//new slot

    }
}

