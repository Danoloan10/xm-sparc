/*
 * $FILE: reader partition
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *         Alfons Crespo
 *
 */

#include <xm.h>
#include <stdio.h>

#define PORT_SIZE 64
#define REFRESH_PERIOD 10000000LL
#define NPARTS 7
#define MAX_ITER 5
#define MAFS 1


long max[NPARTS], min[NPARTS], mean[NPARTS], suma[NPARTS];
long prev_mean[NPARTS];

void printStats(void) {
    int n;

    printf("\nAverage:\t");
    for(n=1; n <= NPARTS; n++) {
        printf ("%d\t" ,mean[n]);
    }
    printf("\n   Max: \t");
    for(n=1; n <= NPARTS; n++) {
        printf("%d\t", max[n]);
    }
    printf("\n   Min: \t");
    for(n=1; n <= NPARTS; n++) {
        printf("%d\t",min[n]);
    }
    printf("\n");
}

void PartitionMain(void) {
  char portName[16];
  long counter[NPARTS];
  unsigned long addr[NPARTS];
  int port[NPARTS];

  int niter, n, ok, currentMAF;
  long old[NPARTS], diff[NPARTS];
  int plan_id = 0;

  printf("P[%d] Reader start\n", XM_PARTITION_SELF);
  for (n=1; n<NPARTS; n++) {
      sprintf(portName, "port%d", (n));
      if ((port[n]=XM_create_sampling_port(portName, PORT_SIZE, XM_DESTINATION_PORT, REFRESH_PERIOD))<0) {
          printf("port [%s] cannot be created %d\n", portName, port[n]);
          return;
      }
  }
// Reads the addresses of counter variables at COUNTER partitions
  for (n=1; n<NPARTS; n++) {
      XM_read_sampling_message(port[n], &addr[n], sizeof(unsigned long), 0);
      old[n] = 0;
//    printf("address = 0x%x\n", addr[n]);
  }

    XM_switch_sched_plan(1, &plan_id);
    printf("Plan change requested\n");
    XM_idle_self();

// Start the reading phase
  niter = 0;
  currentMAF = 0;
  while(niter <= MAX_ITER) {
    currentMAF++;
    if (currentMAF == MAFS) {
      // Reading the counters
      for (n=1; n<NPARTS; n++) {
      ok = XM_memory_copy(XM_PARTITION_SELF, &counter[n], n, addr[n], sizeof(unsigned long));
          diff[n] = (counter[n]) - old[n];
          old[n] = counter[n];
      }
      // Print the partial results
      printf("%i:",  niter);
      for (n=1; n<NPARTS; n++) printf(" %d", diff[n] );
      printf("\n");

      //Compute max, min and suma
      for (n=1; n<NPARTS; n++) {
          if (max[n] < diff[n]) {max[n] = diff[n];};
          if (min[n] > diff[n]) {min[n] = diff[n];};
          suma[n] = suma[n] + diff[n];
          if (niter <= 1) {
                  min[n] = diff[n];
                  max[n] = diff[n];
                  mean[n] = diff[n];
                  prev_mean[n] = mean[n];
                  //std[n] = 0.0;
          } else {
                  mean[n] = prev_mean[n] + (diff[n] - prev_mean[n]) / (niter-1);
                // std[n] = std[n] + (diff[n] - prev_mean[n]) * (diff[n] - mean[n]);
                  prev_mean[n] = mean[n];
          }
      }
      niter++;
      currentMAF = 0;
    }
    XM_idle_self();
  }
  // Print the summary
    printf("\nAverage:\t");
    for(n=1; n < NPARTS; n++) {
        printf ("%d\t" ,mean[n]);
    }
    printf("\n   Max: \t");
    for(n=1; n < NPARTS; n++) {
        printf("%d\t", max[n]);
    }
    printf("\n   Min: \t");
    for(n=1; n < NPARTS; n++) {
        printf("%d\t",min[n]);
    }
    printf("\n");

    XM_halt_system();
}
