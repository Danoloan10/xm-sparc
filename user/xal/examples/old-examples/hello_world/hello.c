/*
 * $FILE: hello.c
 *
 * hello example
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

#include <string.h>
#include <stdio.h>
#include <xm.h>
#include <irqs.h>

void PartitionMain(void) {
    XM_write_console("I am ", 6);
    XM_write_console(XM_params_get_PCT()->name, strlen(XM_params_get_PCT()->name)+1);
    XM_write_console(". Hello World!\n", 16);
    XM_halt_partition(XM_PARTITION_SELF);
}

