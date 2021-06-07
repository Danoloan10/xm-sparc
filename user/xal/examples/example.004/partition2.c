/*
 * $FILE: partition2.c
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

#define SPORT_NAME "portS"
#define MAX_NUMBER_OF_MESSAGES 5

#if (defined(CONFIG_MMU)) ||  (defined(CONFIG_MPU) && defined(CONFIG_AT697F))
#define SHARED_ADDRESS  0x40200000
#endif

#define PRINT(...) do { \
        printf("[P%d] ", XM_PARTITION_SELF); \
        printf(__VA_ARGS__); \
} while (0)

char sMessage[32];
xm_s32_t sDesc;
xm_u32_t flags;
volatile xm_u32_t seq;

void SamplingExtHandler(trapCtxt_t *ctxt)
{
    xm_u32_t flags;

    if (XM_read_sampling_message(sDesc, sMessage, sizeof(sMessage), &flags) > 0)
    {
        PRINT("Received sampling message:  %s\n", sMessage);
        seq++;
    }
}

void PartitionMain(void)
{
    seq = 0;
    xm_u32_t lastNumberQueuingMessages = -1;
    xm_u8_t canHalt = 0;
    
#if (defined(CONFIG_MMU)) ||  (defined(CONFIG_MPU) && defined(CONFIG_AT697F))
    *(volatile xm_u32_t *)SHARED_ADDRESS = lastNumberQueuingMessages;
#endif
    
    PRINT("Starting example 004...\n");

    PRINT("Opening ports...\n");
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
    HwSti();

    //Unmask port irqs
    XM_clear_irqmask(0, (1<<XM_VT_EXT_SAMPLING_PORT));

    PRINT("Waiting for messages\n");
    while (1)
    {
#if (defined(CONFIG_MMU)) ||  (defined(CONFIG_MPU) && defined(CONFIG_AT697F))
        if (*(volatile xm_s32_t *)SHARED_ADDRESS >= MAX_NUMBER_OF_MESSAGES && canHalt)
        {
            goto end;
        }
        else
        {
            if (*(volatile xm_u32_t *)SHARED_ADDRESS != lastNumberQueuingMessages)
            {
                PRINT("Read shared memory (0x%x): The number of queuing messages received by P1 is %d\n", SHARED_ADDRESS,  *(volatile xm_u32_t *)SHARED_ADDRESS);
                lastNumberQueuingMessages = *(volatile xm_u32_t *)SHARED_ADDRESS;
                
                if (lastNumberQueuingMessages == MAX_NUMBER_OF_MESSAGES)
                {
                    canHalt = 1;
                }
            }
        }
#else
        if (seq == MAX_NUMBER_OF_MESSAGES)
        {
            goto end;
        }
#endif
    }

end:
    PRINT("Halting...\n");
    XM_halt_partition(XM_PARTITION_SELF);
}
