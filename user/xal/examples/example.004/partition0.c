/*
 * $FILE: partition0.c
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
#include <string.h>
#include <stdio.h>
#include <xm.h>
#include <irqs.h>

#define QPORT_NAME "portQ"
#define SPORT_NAME "portS"
#define MAX_NUMBER_OF_MESSAGES 5

#define PRINT(...) do { \
        printf("[P%d] ", XM_PARTITION_SELF); \
        printf(__VA_ARGS__); \
} while (0)

char qMessage[32];
char sMessage[32];

void PartitionMain(void)
{
    xm_s32_t qDesc, sDesc, e;
    xm_u32_t flags, sSeq, qSeq;
    xmQueuingPortInfo_t qInfo;
    xmSamplingPortInfo_t sInfo;
    PRINT("Starting example 004...\n");

    PRINT("Creating queuing and sampling ports...\n");
    PRINT("Get queuing port info: %x\n", XM_get_queuing_port_info(QPORT_NAME, &qInfo));
    PRINT("Queuing port ID: %x\n", qInfo.objDesc);
    // Create queuing port
    qDesc = XM_create_queuing_port(QPORT_NAME, 16, 128, XM_SOURCE_PORT); //Parameters of creation
    if (qDesc < 0)
    {
        // Calls must match XML configuration
        PRINT("Error opening queuing port (error %d)\n", qDesc);
        goto end;
    }
    PRINT("Queuing port ID: %x\n", qDesc);
    PRINT("Get sampling port info: %x\n", XM_get_sampling_port_info(SPORT_NAME, &sInfo));
    PRINT("Sampling port ID: %x\n", sInfo.objDesc);
    // Create sampling port
    sDesc = XM_create_sampling_port(SPORT_NAME, 128, XM_SOURCE_PORT, 0);
    if (sDesc < 0)
    {
        // Calls must match XML configuration
        PRINT("Error opening sampling port (error %d)\n", sDesc);
        goto end;
    }
    PRINT("Sampling port ID: %x\n", sDesc);
    PRINT("Port creation done\n");
    
    XM_idle_self();

    PRINT("Generating messages...\n");
    sSeq = qSeq = 0;
    for (e=0; e<MAX_NUMBER_OF_MESSAGES; ++e) {
        sprintf(sMessage, "<<sampling message %d>>", ++sSeq);
        PRINT("Sending %s\n", sMessage);
        XM_write_sampling_message(sDesc, sMessage, sizeof(sMessage));
        XM_idle_self();

        sprintf(qMessage, "<<queuing message %d>>", ++qSeq);
        PRINT("Sending %s\n", qMessage);
        XM_send_queuing_message(qDesc, qMessage, sizeof(qMessage));
        XM_idle_self();
    }
    PRINT("Done.\n");

end:
    PRINT("Halting...\n");
    XM_halt_partition(XM_PARTITION_SELF);
}
