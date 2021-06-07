/*
 * $FILE: supervisor.c
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
#include <stdio.h>
#include <xm.h>

#define P1 1
#define P2 2
#define PRINT(...) do { \
        printf("[P%d] ", XM_PARTITION_SELF); \
        printf(__VA_ARGS__); \
} while (0)

static xmPartitionStatus_t partStatus;

static inline int GetStatus(xmPartitionStatus_t *partStatus) {
    return (partStatus->state[0]);
}

void PrintStatus(void) {
    static xmPartitionStatus_t partStatus;
    int st1, st2;

    /* printf("GetSTATUS: %d\n", XM_get_partition_status(P1, &partStatus)); */
    XM_get_partition_status(P1, &partStatus);
    st1 = GetStatus(&partStatus);

    /* printf("GetSTATUS: %d\n", XM_get_partition_status(P2, &partStatus)); */
    XM_get_partition_status(P2, &partStatus);
    st2 = GetStatus(&partStatus);

    PRINT("Status P1 => 0x%x; P2 => 0x%x\n", st1, st2);
}

void PartitionMain(void) {

    int retValue;

    PRINT("Starting example 007...\n");
    
    PrintStatus();
    XM_idle_self();
	
    retValue = XM_suspend_partition(P1);
    PRINT("Suspends P%d (return value %d)\n", P1, retValue);
    if (retValue >= 0)
        PrintStatus();
    XM_idle_self();
    retValue = XM_suspend_partition(P2);
	PRINT("Suspends P%d (return value %d)\n", P2, retValue);
    if (retValue >= 0)
        PrintStatus();

    XM_idle_self();
    XM_idle_self();

    retValue = XM_resume_partition(P1);
    PRINT("Resumes P%d (return value %d)\n", P1, retValue);
    if (retValue >= 0)
        PrintStatus();
    XM_idle_self();
    retValue = XM_resume_partition(P2);
    PRINT("Resumes P%d (return value %d)\n", P2, retValue);
    if (retValue >= 0)
        PrintStatus();
	
    XM_idle_self();
    XM_idle_self();

    retValue = XM_halt_partition(P1);
    PRINT("Halts P%d (return value %d)\n", P1, retValue);
    if (retValue >= 0)
        PrintStatus();
    XM_idle_self();
    retValue = XM_halt_partition(P2);
    PRINT("Halts P%d (return value %d)\n", P2, retValue);
    if (retValue >= 0)
        PrintStatus();
	
    XM_idle_self();
     
    retValue = XM_reset_partition(P1, -1, XM_WARM_RESET, 0);
    PRINT("Restarts P%d (return value %d)\n", P1, retValue);
    if (retValue >= 0)
        PrintStatus();
    XM_idle_self();
    retValue = XM_reset_partition(P2, -1, XM_WARM_RESET, 0);
    PRINT("Restarts P%d (return value %d)\n", P2, retValue);
    if (retValue >= 0)
        PrintStatus();
	
    XM_idle_self();

    PRINT("Halting System ...\n");
    retValue = XM_halt_system();
}

