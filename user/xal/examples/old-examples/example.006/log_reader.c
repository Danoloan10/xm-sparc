/*
 * $FILE: log_reader.c
 *
 * Read the traces written by partition
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *         Alfons Crespo
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


#define HALTED 3

static void PrintHmLog(xmHmLog_t *hmLog) {
    printf("part_Id: %x eventId: %x timeStamp: %lld\n", hmLog->opCode&HMLOG_OPCODE_PARTID_MASK, hmLog->opCode&HMLOG_OPCODE_EVENT_MASK, hmLog->timestamp);
}

void PartitionMain(void) {

    xmPartitionStatus_t partStatus;
    xmHmStatus_t hmStatus;
    xmHmLog_t hmLog;
    int logbook, stat, e;

    //logbook = XM_hm_open();
    printf("[P%d] Open HM: %d\n", XM_PARTITION_SELF, logbook);
    XM_idle_self();
    
    while(1) {
        printf("[P%d] ---------------------------------------------\n",  XM_PARTITION_SELF);
        stat = XM_hm_status(&hmStatus);
        printf("[P%d]HM status noEvents:%d  currentEvent:%d ret: %d \n", XM_PARTITION_SELF, hmStatus.noEvents,  hmStatus.currentEvent, stat);
        printf("[P%d]==============================================\n",  XM_PARTITION_SELF);
        if (hmStatus.noEvents > 0) {
            stat = XM_hm_seek(0, XM_OBJ_SEEK_SET);
            for (e = 1; e <=hmStatus.noEvents; e++) {
                XM_hm_read(&hmLog);
                printf("[P%d]Log %d => ",  XM_PARTITION_SELF,e);
                PrintHmLog(&hmLog);
            }
        }
        printf("[P%d]-----------------------------------------------\n",  XM_PARTITION_SELF);
        XM_get_partition_status(1, &partStatus);
        if (partStatus.state == HALTED) {
           XM_halt_partition(XM_PARTITION_SELF);
        }
        XM_idle_self();
    }
}

