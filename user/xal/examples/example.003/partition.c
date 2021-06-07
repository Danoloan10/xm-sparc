/*
 * $FILE: partition.c
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

char PAYLOAD_INFO=0x00;

void IncreasePayloadInfo()
{
    if (XM_PARTITION_SELF % 2 == 0)
    {
        PAYLOAD_INFO++;
    }
    else
    {
        PAYLOAD_INFO<<=1;
        if (PAYLOAD_INFO == 0)
        {
            PAYLOAD_INFO = 1;
        }
    }
}

void PartitionMain(void)
{
    char eventPayload[XM_TRACE_PAYLOAD_LENGTH];
    int e, wr;
    PRINT("Starting example 003...\n");
    for (e=0; e<6; e++)
    {
        eventPayload[0]=PAYLOAD_INFO;
        IncreasePayloadInfo();
        eventPayload[1]=PAYLOAD_INFO;
        IncreasePayloadInfo();
        eventPayload[2]=PAYLOAD_INFO;
        IncreasePayloadInfo();
        wr=XM_trace_event(eventPayload);
        PRINT("Sent trace %d\n", e);
    }

    XM_idle_self();
    PRINT("Halting...\n");
    XM_halt_partition(XM_PARTITION_SELF);
}

