/*
 * $FILE: hello.c
 *
 * hello example
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
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

#include "std_c.h"
#include <xm.h>

void PartitionMain(void) {
	extern void _start(void); 
	int i;
        xprintf("Hello World\n");
	//xprintf("I am VCPU %d of partition %d!\n",XM_get_vcpuid(),XM_PARTITION_SELF);
/*
	if (XM_get_vcpuid()== 0){
		for (i=1; i<4; i++) {
                    xprintf("RESET %d\n", XM_reset_vcpu(i, _start, 0)) ;
		}
                }*/
	while(1);
}
