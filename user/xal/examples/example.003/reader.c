/*
 * $FILE: reader.c
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
#include <irqs.h>
#include <xm.h>

#define PRINT(...) do { \
        printf("[P%d] ", XM_PARTITION_SELF); \
        printf(__VA_ARGS__); \
} while (0)

#define PARTITION0 0
#define PARTITION1 1

static void PrintEvent(xmTraceEvent_t *event)
{
    xm_s32_t e;
    
    PRINT("Event => timeStamp: %d; Payload:", event->timestamp);

    for (e=0; e<3; e++)
    {
        printf(" %d ", event->payload[e]);
    }

    printf("\n");
}

static void PrintEventSummary(xmTraceEvent_t *event)
{

    PRINT("Event => timeStamp: %d\n", event->timestamp);
}

void PartitionMain(void)
{
    int rd;
    int nslots = 0;
    xmTraceEvent_t event;
    xmTraceStatus_t status;

    PRINT("Starting example 003...\n");

    while(nslots < 6)
    {
        PRINT("Opening traces of P%d\n", PARTITION0);
        if (XM_trace_status(PARTITION0, &status)<0)
        {
            PRINT("Unable to read status\n");
        }
 
        PRINT("Status: No. events: %d\n", status.noEvents);
        
        if ((rd=XM_trace_read(PARTITION0, &event, 1))==1)
        {
            PrintEvent(&event);
            nslots = 0;
        }
        else
        {
            PRINT("No new events\n");            
            nslots++;
        }

        PRINT("Opening traces of P%d\n", PARTITION1);
        if (XM_trace_status(PARTITION1, &status)<0)
        {
            PRINT("Unable to read status\n");
        }

        PRINT("Status: No. events: %d\n", status.noEvents);

        if ((rd=XM_trace_read(PARTITION1, &event, 1))==1)
        {
            PrintEvent(&event);
            nslots = 0;
        }
        else
        {
            PRINT("No new events\n");
            nslots++;
        }
        
        XM_idle_self();
    }

    PRINT("Partition is halting...\n");
    XM_halt_partition(XM_PARTITION_SELF);
}

