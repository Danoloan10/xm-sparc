/*
 * $FILE: writer.c
 *
 * Intra-core sampling channels example 001: A partition writes on and another one reads from a sampling channel
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
    char msg[]="Partition 0 writes, partition 1 reads";
    xm_s32_t port;
    xprintf("Partition %d is running\n",XM_PARTITION_SELF);

    if ((port=XM_create_sampling_port("port0", 64, XM_SOURCE_PORT, 0))<0) {
        xprintf("Error creating port0\n");
        return;
    }

    if (XM_write_sampling_message(port, msg, 38)!=XM_OK) {
        xprintf("Error writing in port0\n");
        return;
    }

    xprintf("Partition %d has completed its tasks\n",XM_PARTITION_SELF);
    while(1);
}
