/*
 * $FILE: partition1.c
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

#define QPORT_NAME      "portQ"
#define SPORT_NAME      "portS"
#define MAX_NUMBER_OF_MESSAGES 5

#if (defined(CONFIG_MMU)) || (defined(CONFIG_MPU) && defined(CONFIG_AT697F))
#define SHARED_ADDRESS  0x40200000
#endif

#define PRINT(...) do { \
        printf("[P%d] ", XM_PARTITION_SELF); \
        printf(__VA_ARGS__); \
} while (0)

char sMessage[32];
char qMessage[32];
xm_s32_t qDesc, sDesc;
volatile xm_s32_t seq;

void QueuingExtHandler(trapCtxt_t *ctxt)
{
    if (XM_receive_queuing_message(qDesc, qMessage, sizeof(qMessage)) > 0)
    {
        seq++;
        PRINT("Received queuing message: %s\n", qMessage);
#if (defined(CONFIG_MMU)) ||  (defined(CONFIG_MPU) && defined(CONFIG_AT697F))
        PRINT("Writing on shared memory (0x%x) the current number of queuing messages received (%d) \n", SHARED_ADDRESS, seq);
        *(volatile xm_u32_t *)SHARED_ADDRESS = seq;
#endif
    }
}

void SamplingExtHandler(trapCtxt_t *ctxt)
{
    xm_u32_t flags;

    if (XM_read_sampling_message(sDesc, sMessage, sizeof(sMessage), &flags) > 0)
    {
        PRINT("Received sampling message: %s\n", sMessage);
    }
}

void PartitionMain(void)
{
    seq = 0;
    
    PRINT("Starting example 004...\n");
    
    PRINT("Creating queuing and sampling ports...\n");
    // Create queuing port
    qDesc = XM_create_queuing_port(QPORT_NAME, 16, 128, XM_DESTINATION_PORT);
    if (qDesc < 0)
    {
        // Calls must match XML configuration
        PRINT("Error opening queuing port (error %d)\n", qDesc);
        goto end;
    }
    
    // Create sampling port
    sDesc = XM_create_sampling_port(SPORT_NAME, 128, XM_DESTINATION_PORT, 0);
    if (sDesc < 0)
    {
        // Calls must match XML configuration
        PRINT("Error opening sampling port (error %d)\n", sDesc);
        goto end;
    }
    PRINT("Ports creation done\n");

    InstallTrapHandler(224+XM_VT_EXT_SAMPLING_PORT, SamplingExtHandler);
    InstallTrapHandler(224+XM_VT_EXT_QUEUING_PORT, QueuingExtHandler);
    HwSti();

    //Unmask port irqs
    XM_clear_irqmask(0, (1<<XM_VT_EXT_SAMPLING_PORT)|(1<<XM_VT_EXT_QUEUING_PORT));

    PRINT("Waiting for messages...\n");
    while (1)
    {
        if (seq == MAX_NUMBER_OF_MESSAGES)
        {
            goto end;
        }
    }

end:
    PRINT("Halting...\n");
    XM_halt_partition(XM_PARTITION_SELF);
}
