/* $Test 124
 * $FILE: partitionA.c
 *
 * 
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

#define P2 2
#define P3 3


static xmPartitionStatus_t partStatus;

static inline int GetStatus(xmPartitionStatus_t *partStatus) {
    return(partStatus->state);
}

void PrintStatus (void) {
static xmPartitionStatus_t partStatus;
int st2, st3;   

    XM_get_partition_status(P2, &partStatus);
    st2 = GetStatus(&partStatus); //partStatus->state;
    XM_get_partition_status(P3, &partStatus);
    st3 = GetStatus(&partStatus); //= partStatus->state;
    printf("[P%d] Status P2 => 0x%x ;  P3 => 0x%x\n", XM_PARTITION_SELF, st2, st3);
}

void PartitionMain(void) {

    int retValue;

  printf("[P%d]Example 004. Partition %d\n", XM_PARTITION_SELF,  XM_PARTITION_SELF);

    PrintStatus();
    XM_idle_self();
    retValue= XM_suspend_partition(P2);
    printf("[P%d] Partition %d  %d is suspended\n", XM_PARTITION_SELF, P2, retValue);
    if (retValue >=0) PrintStatus();
    XM_idle_self();
    retValue= XM_suspend_partition(P3);
    printf("[P%d] Partition %d  %d is suspended\n", XM_PARTITION_SELF, P3, retValue);
    if (retValue >=0) PrintStatus();

    XM_idle_self();
    XM_idle_self();

    retValue= XM_resume_partition(P2);
    printf("[P%d] Partition %d  %d is resumed \n", XM_PARTITION_SELF, P2, retValue);
    if (retValue >=0) PrintStatus();
    XM_idle_self();
    retValue= XM_resume_partition(P3);
    printf("[P%d] Partition %d  %d is resumed \n", XM_PARTITION_SELF, P3, retValue);
    if (retValue >=0) PrintStatus();
    XM_idle_self();
    XM_idle_self();

    retValue= XM_halt_partition(P2);
    printf("[P%d] Partition %d  %d is halted \n", XM_PARTITION_SELF, P2, retValue);
    if (retValue >=0) PrintStatus();
    XM_idle_self();
    retValue= XM_halt_partition(P3);
    printf("[P%d] Partition %d  %d is halted \n", XM_PARTITION_SELF, P3, retValue);
    if (retValue >=0) PrintStatus();
    XM_idle_self();

    retValue= XM_reset_partition(P2, XM_WARM_RESET, 0);
    printf("[P%d] Partition %d  %d is restarted \n", XM_PARTITION_SELF, P2, retValue);
    if (retValue >=0) PrintStatus();
    XM_idle_self();
    retValue= XM_reset_partition(P3,XM_WARM_RESET, 0);
    printf("[P%d] Partition %d  %d is restarted \n", XM_PARTITION_SELF, P3, retValue);
    if (retValue >=0) PrintStatus();
    XM_idle_self();


    printf("[P%d] Halting System ...\n", XM_PARTITION_SELF);
    retValue= XM_halt_system();


}

