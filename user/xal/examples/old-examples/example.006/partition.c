#include <stdio.h>
#include <xm.h>
#include <irqs.h>

#define __DIV(a, b, c) __asm__ __volatile__ ("udiv %1, %2, %0\n\t" : "=r" (c) : "r" (a), "r" (b))

volatile  int a=1, b=2;

void TrapHandler(trapCtxt_t *ctxt) {
    partitionControlTable_t *controlTable;
    controlTable = XM_params_get_PCT();

    printf("[P%d] Ext Interrupt handled: %d\n", XM_PARTITION_SELF, ctxt->irqNr);
    if (controlTable->resetCounter < 2) {
        printf("[P%d] Partition is restarting\n", XM_PARTITION_SELF);
        XM_reset_partition(XM_PARTITION_SELF, XM_WARM_RESET, 0);
    } else {
        printf("[P%d] Partition is halting\n", XM_PARTITION_SELF);
        XM_halt_partition(XM_PARTITION_SELF);
    }
}

void SpentTime(int n) {
   int i,j;
   int x,y = 1;
    for (i= 0; i <=n; i++) {
        for (j= 0; j <=n; j++) {
            x = x + x - y;
        }
    }
}

void PartitionMain(void) {
    unsigned long counter=0;

    float x=1.0;

    printf("[P%d] Test 15.4\n", XM_PARTITION_SELF);

    SetupIrqs();
    InstallTrapHandler(XM_VT_EXT_SHUTDOWN_TTAB, TrapHandler);
    XM_clear_irqmask(0, (1<<XM_VT_EXT_SHUTDOWN));
    XM_sparc_clear_pil();

    while (counter < 25) {
        counter++;
        printf("[P%d]: %d\n", XM_PARTITION_SELF, counter);
        SpentTime(1000);
        x = x * (float)(counter);
        if ((counter % 8)== 0) {
            a=0;
            __DIV(counter,a, b);
            printf("[P%d]: %d %d\n", XM_PARTITION_SELF, a,b);
        }
    }
}
