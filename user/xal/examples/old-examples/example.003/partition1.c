/*
 * $FILE: example 003
 *
 * Sampling ports example - IO partition
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
#include <irqs.h>

#define STR(s)      #s
#define ESTR(s)     STR(s)
#define ASSERT(x)	if(!x){printf(0, __BASE_FILE__":"ESTR(__LINE___)" A: "#x"\n");}

#define PORT1 "sport1_w"
#define PORT2 "sport2_r"
#define PORT3 "sport3_r"
#define PORT_SIZE 12
#define REFRESH_PERIOD 1000000L
#define INPUT_SLOT 0
#define OUTPUT_SLOT 3

int port1, port2, port3;
long long a;

void TrapHandler(trapCtxt_t *ctxt) {
    partitionControlTable_t *controlTable;
    long value2=0, value3=0;
    int r, w, nSlot;
    long random_number;

    controlTable = XM_params_get_PCT();
//    printf("[P%d] Slot: %d :", XM_PARTITION_SELF, nSlot);
    nSlot = controlTable->schedInfo.id;
    switch(nSlot) {
        case (INPUT_SLOT): 
            XM_get_time(XM_HW_CLOCK, &a);
            random_number = ((long) a) % (100*1024);
            w=XM_write_sampling_message(port1, &random_number, sizeof(long));
            printf("[P%d] (p1) =>  %d\n", XM_PARTITION_SELF, random_number);
            break;
        case (OUTPUT_SLOT):
            r=XM_read_sampling_message(port2, &value2, 4, 0);
            printf("[P%d] => (p2) %d ", XM_PARTITION_SELF,  value2);

            r=XM_read_sampling_message(port3, &value3, 4, 0);
            printf("[P%d] => (p3) %d  ", XM_PARTITION_SELF,  value3);
            printf("[P%d]\n", XM_PARTITION_SELF);
            break;
    }
}

void PartitionMain(void) {

    int nmess;

    if ((port1=XM_create_sampling_port(PORT1, PORT_SIZE, XM_SOURCE_PORT, REFRESH_PERIOD))<0) {
        printf("[P%d] port [%s] cannot be created\n", XM_PARTITION_SELF, PORT1);
        return;
    }  
    printf("[P%d] Port created %d\n", XM_PARTITION_SELF, port1);

    if ((port2=XM_create_sampling_port(PORT2, PORT_SIZE, XM_DESTINATION_PORT, REFRESH_PERIOD))<0) {
        printf("[P%d] port [%s] cannot be created\n", XM_PARTITION_SELF, PORT2);
        return;
    }  

    printf("[P%d] Port created %d\n", XM_PARTITION_SELF, port2);

    if ((port3=XM_create_sampling_port(PORT3, PORT_SIZE, XM_DESTINATION_PORT, REFRESH_PERIOD))<0) {
        printf("[P%d] port [%s] cannot be created\n", XM_PARTITION_SELF, PORT3);
        return;
    }  

    printf("[P%d] Port created %d\n", XM_PARTITION_SELF, port2);

    nmess = 0;
    printf("[P%d] waiting next MAF..\n",XM_PARTITION_SELF);
    XM_idle_self();
    
    SetupIrqs();
    InstallTrapHandler(XM_VT_EXT_CYCLIC_SLOT_START_TTAB, TrapHandler);
    XM_clear_irqmask(0, (1<<XM_VT_EXT_CYCLIC_SLOT_START));
    XM_sparc_clear_pil();

    while (nmess < 25) {
        XM_idle_self();
        nmess++;
    }
    XM_halt_partition(XM_PARTITION_SELF);
}
