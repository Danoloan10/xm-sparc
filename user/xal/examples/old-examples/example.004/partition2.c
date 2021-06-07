/* $Example 007
 *
 *
 * Author: Miguel Masmano / Alfons Crespo
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

#include <xm.h>
#include <stdio.h>

#define LIMIT 1000

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

  printf("[P%d]Example 004. Partition %d\n", XM_PARTITION_SELF,  XM_PARTITION_SELF);
  while(counter < LIMIT) {
    counter++;
    printf("[P%d] cnt: %d\n", XM_PARTITION_SELF, counter);
    SpentTime(1000);
  }
  XM_halt_partition(XM_PARTITION_SELF);
}
