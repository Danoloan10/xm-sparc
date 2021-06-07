/*
 * $FILE: partition.c
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

#include <stdio.h>
#include <xm.h>

#define PRINT(...) do { \
        printf("[P%d] ", XM_PARTITION_SELF); \
		printf(__VA_ARGS__); \
} while (0)

double a = 0.5, b = 0.5;
#define SPARC_PSR_EF 0x1000U
int PartitionMain(void) 
{
    PRINT("float %d\n", (int)(a+b));
    XM_sparc_set_psr(XM_sparc_get_psr() & ~SPARC_PSR_EF);
    PRINT("float %d\n", (int)(a+b));
    return 0;
}

 
