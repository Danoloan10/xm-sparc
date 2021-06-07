/*
 * $FILE: reader.c
 *
 * Intra-core sampling channels example 002: A partition writes on and another two read from different sampling channels
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

    char msg[38];
    xm_s32_t port;
    xm_u32_t flags;
    xprintf("Partition %d is running\n",XM_PARTITION_SELF);

    if ((port=XM_create_sampling_port("port0", 64, XM_DESTINATION_PORT, 0))<0) {
        xprintf("Error creating port0\n");
        return;
    }

    if (XM_read_sampling_message(port, msg, 38, &flags)<0) {
        xprintf("Error reading from port0\n");
        return;
    } else {
        xprintf("Message: %s\n", msg);
    }

    xprintf("Partition %d has finished its tasks\n",XM_PARTITION_SELF);

    while(1);

 }
