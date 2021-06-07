/* $Example 002
 *
 *
 * Author: Miguel Masmano / Alfons Crespo
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

#define MAX_VALUES 20
#define PERIOD_HW 330000LL
#define PERIOD_EX 220000LL

static long long current_hwclock;
static long long current_exclock;

static long long stored_hwclocks[MAX_VALUES];
static long long stored_exclocks[MAX_VALUES];

volatile int nhwc=0, nexc = 0;

static void exec_clock_handler(trapCtxt_t *ctxt) {
    XM_get_time(XM_EXEC_CLOCK, &current_exclock);
    if (nexc < MAX_VALUES) {
        stored_exclocks[nexc++] = current_exclock;
    }
}

static void hw_clock_handler(trapCtxt_t *ctxt) {
    XM_get_time(XM_HW_CLOCK, &current_hwclock);
    if (nhwc < MAX_VALUES) {
        stored_hwclocks[nhwc++] = current_hwclock;
    }
}

void PartitionMain(void) {
    int i;
    
    printf("[P%d]: Example 002 \n", XM_PARTITION_SELF);
    
    XM_get_time(XM_HW_CLOCK, &current_hwclock);
    XM_get_time(XM_EXEC_CLOCK, &current_exclock);

    trapHandlersTab[224]=hw_clock_handler;
    trapHandlersTab[225]=exec_clock_handler;

    XM_sparc_clear_pil();
/*    flags=XM_sparc_get_flags();
    flags&=~(0xf00);
    XM_sparc_set_flags(flags);
*/
    XM_clear_irqmask(0, (1<<XM_VT_EXT_HW_TIMER)|(1<<XM_VT_EXT_EXEC_TIMER));

    XM_set_timer(XM_HW_CLOCK, current_hwclock+100LL,PERIOD_HW);
    XM_set_timer(XM_EXEC_CLOCK, current_exclock+100LL,PERIOD_EX);

    while ((nhwc < MAX_VALUES)&&(nexc < MAX_VALUES));

    printf("[P%d]: HW_TIMER ocurrences at\n",XM_PARTITION_SELF);
    for (i=1; i < nhwc; i++) {
        printf("[P%d]: %d => at:  %lld  diff: %lld\n",XM_PARTITION_SELF,i, stored_hwclocks[i], stored_hwclocks[i]- stored_hwclocks[i-1]);
    }
    printf("[P%d]: EXEC_TIMER ocurrences at\n",XM_PARTITION_SELF);
    for (i=1; i < nexc; i++) {
        printf("[P%d]: %d => at:  %lld  diff: %lld\n",XM_PARTITION_SELF, i,  stored_exclocks[i], stored_exclocks[i]- stored_exclocks[i-1]);
    }
    XM_halt_partition(XM_PARTITION_SELF);
}


