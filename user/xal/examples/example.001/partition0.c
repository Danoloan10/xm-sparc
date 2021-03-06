/*
 * $FILE: partition0.c
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

#define PRINT(...) do { \
        printf("[P%d] ", XM_PARTITION_SELF); \
        printf(__VA_ARGS__); \
} while (0)

volatile xm_s32_t lock;

void HwTimerHandler(trapCtxt_t *ctxt)                                       /* XAL trap API */
{
    xmTime_t hw, exec;

    XM_get_time(XM_HW_CLOCK, &hw);
    XM_get_time(XM_EXEC_CLOCK, &exec);
    PRINT("[%lld:%lld] IRQ HW Timer\n", hw, exec);
    ++lock;
}

void PartitionMain(void)
{
    xmTime_t hwClock, execClock;
    
    PRINT("Starting example 001...\n");

    InstallTrapHandler(XAL_XMEXT_TRAP(XM_VT_EXT_HW_TIMER), HwTimerHandler); /* Install timer handler */

    HwSti();                                                                /* Enable irqs */
    XM_clear_irqmask(0, (1<<XM_VT_EXT_HW_TIMER));                           /* Unmask timer irqs */

    XM_get_time(XM_HW_CLOCK, &hwClock);                                     /* Read hardware clock */
    XM_get_time(XM_EXEC_CLOCK, &execClock);                                 /* Read execution clock */

    lock = 0;

    PRINT("Setting HW timer at 1 sec period\n");
    XM_set_timer(XM_HW_CLOCK, hwClock+1000000LL, 1000000LL);                /* Set hardware time driven timer */

    while (lock < 10);

    PRINT("Halting\n");
    XM_halt_partition(XM_PARTITION_SELF);
}
