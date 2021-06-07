/*
 * $FILE: reader.c
 *
 * Sampling ports example - reader partition
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

#include <stdio.h>
#include <xm.h>
#include <irqs.h>

#define PORT1_NAME "sport1_r"
#define PORT2_NAME "sport2_w"
#define PORT_SIZE 12
#define REFRESH_PERIOD 1000000L

static int port1, port2;
int niter;

void TrapHandler(trapCtxt_t *ctxt) {
    //partitionControlTable_t *controlTable;
    long value, value2;
    int r,w;

    //controlTable = XM_params_get_PCT();
    //printf("[P%d]    IRQ received: %d 0x%x\n", XM_PARTITION_SELF, ctxt->irqNr, controlTable->extIrqsPend);

    r=XM_read_sampling_message(port1, &value, sizeof(long), 0);
    value2 = value * 3;
    w=XM_write_sampling_message(port2, &value2, sizeof(long));
    printf("[P%d] (p1) %d =>(p3) %d \n", XM_PARTITION_SELF, value, value2);
    niter = 0;
}

void PartitionMain(void) {

    if ((port1=XM_create_sampling_port(PORT1_NAME, PORT_SIZE, XM_DESTINATION_PORT, REFRESH_PERIOD))<0) {
        printf("[P%d] port [%s] cannot be created\n", XM_PARTITION_SELF, PORT1_NAME);
        return;
    }
    printf("[P%d] Sampling Port created %d\n", XM_PARTITION_SELF, port1);

    if ((port2=XM_create_sampling_port(PORT2_NAME, PORT_SIZE, XM_SOURCE_PORT, REFRESH_PERIOD))<0) {
        printf("[P%d] port [%s] cannot be created\n", XM_PARTITION_SELF, PORT2_NAME);
        return;
    }
    printf("[P%d] Sampling Port created %d\n", XM_PARTITION_SELF, port2);

    SetupIrqs();
    InstallTrapHandler(XAL_XMEXT_TRAP(XM_VT_EXT_SAMPLING_PORT), TrapHandler);
    XM_clear_irqmask(0, (1<<XM_VT_EXT_SAMPLING_PORT));
    XM_sparc_clear_pil();

    niter = 0;
    printf("[P%d] Waiting messages ... \n", XM_PARTITION_SELF ); 
    while (niter < 5) {
        niter++;
        XM_idle_self();
    }
    printf("[P%d] Halting ...\n", XM_PARTITION_SELF); 
    XM_halt_partition(XM_PARTITION_SELF);
}
