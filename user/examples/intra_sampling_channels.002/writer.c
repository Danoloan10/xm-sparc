/*
 * $FILE: writer.c
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
    char msg[]="This message should be overwritten";
    char msg0[]="Partition 0 writes, partition 1 reads";
    char msg1[]="Partition 0 writes, partition 2 reads";
    char msg2[]="Partition 0 writes, partition 2 reads";
    xm_s32_t port0, port1, port2;
    xprintf("Partition %d is running\n",XM_PARTITION_SELF);

    if ((port0=XM_create_sampling_port("port0", 64, XM_SOURCE_PORT, 0))<0) {
        xprintf("Error creating port0\n");
        return;
    }

    if ((port1=XM_create_sampling_port("port1", 64, XM_SOURCE_PORT, 0))<0) {
        xprintf("Error creating port1\n");
        return;
    }

    if ((port2=XM_create_sampling_port("port2", 64, XM_SOURCE_PORT, 0))<0) {
        xprintf("Error creating port2\n");
        return;
    }

    if (XM_write_sampling_message(port0, msg0, 38)!=XM_OK) {
        xprintf("Error writing in port0\n");
        return;
    }

    if (XM_write_sampling_message(port1, msg, 35)!=XM_OK) {
        xprintf("Error writing in port1\n");
        return;
    }

    if (XM_write_sampling_message(port1, msg1, 38)!=XM_OK) {
        xprintf("Error writing in port1\n");
        return;
    }

    if (XM_write_sampling_message(port2, msg2, 38)!=XM_OK) {
        xprintf("Error writing in port2\n");
        return;
    }

    xprintf("Partition %d has completed its tasks\n",XM_PARTITION_SELF);
    while(1);
}
