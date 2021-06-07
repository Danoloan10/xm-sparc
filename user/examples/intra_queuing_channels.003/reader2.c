/*
 * $FILE: reader2.c
 *
 * Intra-core queuing channels example 003: Several partitions write to and read from different queuing channels
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

    char msg[38], msg0[38], msg3[]="Partition 2 writes, partition 3 reads";
    xm_s32_t port0, port3;
    xprintf("Partition %d is running\n",XM_PARTITION_SELF);

    if ((port0=XM_create_queuing_port("port0", 4, 64, XM_DESTINATION_PORT))<0) {
        xprintf("Error creating port0\n");
        return;
    }

    if ((port3=XM_create_queuing_port("port3", 4, 64, XM_SOURCE_PORT))<0) {
        xprintf("Error creating port3\n");
        return;
    }

    if (XM_receive_queuing_message(port0, msg, 38)<0) {
        xprintf("Error reading first message from port0\n");
        return;
    } else {
        xprintf("Message: %s\n", msg);
    } 

    if (XM_receive_queuing_message(port0, msg0, 38)<0) {
        xprintf("Error reading second message from port0\n");
        return;
    } else {
        xprintf("Message: %s\n", msg0);
    } 


    if (XM_send_queuing_message(port3, msg3, 38)!=XM_OK) {
        xprintf("Error writing in port3\n");
        return;
    }

    xprintf("Partition %d has finished its tasks\n",XM_PARTITION_SELF);

    while(1);

 }
