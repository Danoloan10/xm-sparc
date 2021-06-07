/*
 * $FILE: lice_interface.c
 *
 *
 * $VERSION$
 *
 * Author: Javier O. Coronel <jacopa@ai2.upv.es>
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

#include <kdevice.h>
#include <drivers/lice_interface.h>
#include <linkage.h>

/*Define numbers of reserved address by XtratuM*/
#define RESERVED_ADDRESS_NR	LICE_CODES_NR

// #ifdef CONFIG_CPU_ITAR_FREE_SIM
// 
// RESERVE_IOPORTS(INFO_LICE_BASE_SIM, 1);
// 
// #endif

RESERVE_IOPORTS(LICE_BASE_ADDRESS, LICE_CODES_NR);

