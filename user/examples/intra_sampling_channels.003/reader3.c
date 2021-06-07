/*
 * $FILE: reader3.c
 *
 * Intra-core sampling channels example 003: Several partitions write to and read from different sampling channels
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

 char msg2[38], msg3[38];
    xm_s32_t port2, port3;
    xm_u32_t flags;
    xprintf("Partition %d is running\n",XM_PARTITION_SELF);

    if ((port2=XM_create_sampling_port("port2", 64, XM_DESTINATION_PORT, 0))<0) {
        xprintf("Error creating port2\n");
        return;
    }

    if ((port3=XM_create_sampling_port("port3", 64, XM_DESTINATION_PORT, 0))<0) {
        xprintf("Error creating port3\n");
        return;
    }

    if (XM_read_sampling_message(port2, msg2, 38, &flags)<0) {
        xprintf("Error reading from port2\n");
        return;
    } else {
        xprintf("Message 2: %s\n", msg2);
    }

    if (XM_read_sampling_message(port3, msg3, 38, &flags)<0) {
        xprintf("Error reading from port3\n");
        return;
    } else {
        xprintf("Message 3: %s\n", msg3);
    }
  xprintf("Partition %d has finished its tasks\n",XM_PARTITION_SELF);

    while(1);

 }
