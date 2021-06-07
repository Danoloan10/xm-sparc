/*
 * $FILE: counter partition
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *         Alfons Crespo
 *
 */

#include <xm.h>
#include <stdio.h>

#define PORT_NAME "port1"
#define PORT_SIZE 64
#define REFRESH_PERIOD 10000000LL

volatile long counter=0;

void PartitionMain(void) {
  int port0;
  unsigned long addr;

  printf("P[%d] Counter start\n", XM_PARTITION_SELF);
//   Opens the port for writing
  if ((port0=XM_create_sampling_port(PORT_NAME, PORT_SIZE, 
                        XM_SOURCE_PORT, REFRESH_PERIOD))<0) {
    printf("port [%s] cannot be created %d\n", PORT_NAME, port0);
    return;
  }

//  Writes the counter'address in the port
  addr=(unsigned long)&counter;
  XM_write_sampling_message(port0, (char *)&addr, sizeof(unsigned long));
//   printf("address = 0x%x\n", addr);

//   Counting - the partition is never finished
  counter=1;
  while(1) {
    counter++;
  }

  XM_halt_partition(XM_PARTITION_SELF);
}
