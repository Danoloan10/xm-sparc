/*
 * $FILE: lice_interface.h
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

#ifndef _XM_DEV_LICE_ITARFREE_H_
#define _XM_DEV_LICE_ITARFREE_H_

#if defined(CONFIG_LEON2) || defined(CONFIG_LEON3) || defined(CONFIG_LEON3FT)
#include <arch/leon.h>
#else
#error Processor not valid
#endif

#ifdef CONFIG_CPU_ITAR_FREE_SIM

/*Remove base address of LICE interface on board*/
#define FPGA_LICE_WR	0x00

/*Base address of LICE interface on sim*/
#define INFO_LICE_BASE_SIM	0x29002000

#ifndef __ASSEMBLY__

/* number of traces ports*/
#define NB_TRACE_PORTS 16

/* structure describing the debug device memory implementation */
typedef volatile struct
{
   /* used for tracing quick debug information (just write something inside) */
   xm_u32_t   ulTraceInfo;

   /* each element is a fake debug UART that is stored in a file Output_X.txt */
   xm_u8_t    ulTracePorts[NB_TRACE_PORTS];

   /* write something here to stop the simulator (halt the system) */
   xm_u32_t   isHalt;

   /* write an id here to reset a timer
    * 0<=id<256
    */
   xm_u32_t   ulInitChrono;

   /* write an id here to start a time measure
    * 0<=id<256
    */
   xm_u32_t   ulStartChrono;

   /* write an id here to stop a time measure and display the result
    * 0<=id<256
    */
   xm_u32_t   ulStopChrono;

   /* Implement random number generator 
    * just read here to have a random number 
    */
   xm_u32_t   ulRandom;

   /* Get Date Feature (without ns) */
   xm_u32_t ulTimeStamp;
} SSP_SInfoZone;


/* This macro ask the simulator to stop
 */
#define STOP_SIMU() ((SSP_SInfoZone*)INFO_LICE_BASE_SIM)->isHalt = 1;

/* this macro ask the simulator to set to 0 a chronometer
 */
#define INIT_CHRONO(id) ((SSP_SInfoZone*)INFO_LICE_BASE_SIM)->ulInitChrono = (id);

/* this macro ask the simulator to start a chronometer
 */
#define START_CHRONO(id) ((SSP_SInfoZone*)INFO_LICE_BASE_SIM)->ulStartChrono = (id);

/* this macro ask the simulator to stop a chronometer
 */
#define STOP_CHRONO(id) ((SSP_SInfoZone*)INFO_LICE_BASE_SIM)->ulStopChrono = (id);

/* this macro is used to output a fast trace in the simulator console */
#define SIMU_TRACE_INFO(info) \
   ((SSP_SInfoZone*)INFO_LICE_BASE_SIM)->ulTraceInfo = (info);

#endif   /* __ASSEMBLY */

#else
/*Base address of LICE interface on board*/
#define FPGA_LICE_WR	CONFIG_LICE_ADDRESS	

#endif   /* CONFIG_CPU_ITAR_FREE_SIM */


/*Definitions used in the LICE_Interfaces device*/
#define LICE_BASE_ADDRESS	FPGA_LICE_WR

#define LICE_CONSTANT_VALUE     0x05 << 8

/*LICE codes defined for XtratuM*/
// <track id="LICE-INTERNAL-TRACES">
#define LICE_CODE_START_XM		(0x01)
#define LICE_CODE_BEGIN_TRAP		(0x02)
#define LICE_CODE_END_TRAP		(0x03)
#define LICE_CODE_BEGIN_CS		(0x04)
#define LICE_CODE_END_CS		(0x05)
#define LICE_CODE_BEGIN_FTRAP		(0x06)
#define LICE_CODE_END_FTRAP		(0x07)
#define LICE_CODE_HYPERCALL_NR		(0x08)
#define LICE_CODE_ASM_HYPERCALL_NR	(0x09)
#define LICE_CODE_END_ASM_HYPERCALL     (0x0c)
// </track id="LICE-INTERNAL-TRACES">
// <track id="LICE-SCHEDULING-TRACES">
#define LICE_CODE_START_PARTITION	(0x0a)
#define LICE_CODE_END_PARTITION		(0x0b)
// </track id="LICE-SCHEDULING-TRACES">
#define LICE_CODE_START_WATCHDOG	(0x14)
#define LICE_CODE_END_WATCHDOG		(0x15)


/*LICE address defined by XtratuM*/
#define ADDR_OFFSET_START_XM		(LICE_CODE_START_XM*4)
#define ADDR_OFFSET_BEGIN_TRAP		(LICE_CODE_BEGIN_TRAP*4)
#define ADDR_OFFSET_END_TRAP		(LICE_CODE_END_TRAP*4)
#define ADDR_OFFSET_BEGIN_CS		(LICE_CODE_BEGIN_CS*4)
#define ADDR_OFFSET_END_CS		(LICE_CODE_END_CS*4)

#define ADDR_OFFSET_BEGIN_FTRAP		(LICE_CODE_BEGIN_FTRAP*4)
#define ADDR_OFFSET_END_FTRAP		(LICE_CODE_END_FTRAP*4)
#define ADDR_OFFSET_HYPERCALL_NR	(LICE_CODE_HYPERCALL_NR*4)
#define ADDR_OFFSET_ASM_HYPERCALL_NR	(LICE_CODE_ASM_HYPERCALL_NR*4)

#define ADDR_OFFSET_START_PARTITION	(LICE_CODE_START_PARTITION*4)
#define ADDR_OFFSET_END_PARTITION	(LICE_CODE_END_PARTITION*4)

#define ADDR_OFFSET_END_ASM_HYPERCALL	(LICE_CODE_END_ASM_HYPERCALL*4)

#define ADDR_OFFSET_START_WATCHDOG	(LICE_CODE_START_WATCHDOG*4)
#define ADDR_OFFSET_END_WATCHDOG	(LICE_CODE_END_WATCHDOG*4)

#define LICE_CODES_NR		24

#ifndef __ASSEMBLY__

static inline xm_u32_t set_iop_bit() {
    xm_u32_t mem_cfg1;
    mem_cfg1=LoadIoReg(LEON_MEMORY_CFG_BASE);
    StoreIoReg(LEON_MEMORY_CFG_BASE,mem_cfg1|IOP_MASK);
    return mem_cfg1;
}

static inline void restore_iop_bit(xm_u32_t mem_cfg1) {
    StoreIoReg(LEON_MEMORY_CFG_BASE,mem_cfg1);
}

static inline void SendDataToLiceInterface(xm_u32_t info, xm_u32_t address) {
#ifdef CONFIG_CPU_ITAR_FREE_SIM
SIMU_TRACE_INFO(((info&0xffffff)|(address<<22)));
#else
   StoreIoReg(address,info);
#endif
}

static inline void LiceCmdStartXM() {
   SendDataToLiceInterface(0x00,LICE_BASE_ADDRESS+ADDR_OFFSET_START_XM);
}

static inline void LiceCmdBeginCS() {
   SendDataToLiceInterface(0x00,LICE_BASE_ADDRESS+ADDR_OFFSET_BEGIN_CS);
}

static inline void LiceCmdEndCS() {
   SendDataToLiceInterface(0x00,LICE_BASE_ADDRESS+ADDR_OFFSET_END_CS);
}

static inline void LiceCmdStartPartition(xm_u32_t id, xm_u32_t numMAF) {
    SendDataToLiceInterface((id<<16)|(numMAF & 0xFFFF),LICE_BASE_ADDRESS+ADDR_OFFSET_START_PARTITION);
    __asm__ __volatile__ ("nop\n\tnop\n\tnop\n\t"::);
}

static inline void LiceCmdEndPartition(xm_u32_t id, xm_u32_t numMAF) {
    SendDataToLiceInterface((id<<16)|(numMAF & 0xFFFF),LICE_BASE_ADDRESS+ADDR_OFFSET_END_PARTITION);
    __asm__ __volatile__ ("nop\n\tnop\n\tnop\n\t"::);
}

static inline void LiceCmdStartWatchdog(void) {
    SendDataToLiceInterface(0x00,LICE_BASE_ADDRESS+ADDR_OFFSET_START_WATCHDOG);
}

static inline void LiceCmdEndWatchdog(void) {
    SendDataToLiceInterface(0x00,LICE_BASE_ADDRESS+ADDR_OFFSET_END_WATCHDOG);
}

/*if __ASSEMBLY__ is defined*/
#else  

#ifdef CONFIG_CPU_ITAR_FREE_SIM
#define LICE_CMD_BASE_DATA(_r0, _r1, _INFO, _ADDR_OFFSET) \
	set 0xffffff, _r1; \
	set _INFO, _r0 ; \
	and _r0, _r1, _r0; \
	set _ADDR_OFFSET, _r1 ; \
	sll _r1, 22, _r1 ; \
	or _r1, _r0, _r1; \
	sethi  %hi(INFO_LICE_BASE_SIM), _r0 ; \
	st _r1 , [ _r0 ]

#define LICE_CMD_BASE_REG(_r0, _r1, _INFO_REG, _ADDR_OFFSET) \
	set 0xffffff, _r0 ; \
	mov _INFO_REG, _r1 ; \
	and _r1, _r0, _r1;\
	set _ADDR_OFFSET, _r0 ; \
	sll _r0, 22, _r0 ; \
	or _r1, _r0, _r1; \
	sethi  %hi(INFO_LICE_BASE_SIM), _r0 ; \
	st _r1 , [ _r0 ]
#else
#define LICE_CMD_BASE_DATA(_r0, _r1, _INFO, _ADDR_OFFSET) \
	sethi  %hi(LICE_BASE_ADDRESS), _r0 ; \
	or  _r0 , _ADDR_OFFSET, _r0 ; \
	set _INFO, _r1 ; \
	st _r1 , [ _r0 ]

#define LICE_CMD_BASE_REG(_r0, _r1, _INFO_REG, _ADDR_OFFSET) \
	sethi  %hi(LICE_BASE_ADDRESS), _r0 ; \
	or  _r0 , _ADDR_OFFSET, _r0 ; \
	mov _INFO_REG, _r1 ; \
	st _r1 , [ _r0 ]
#endif

#define LICE_CMD_BEGIN_TRAP(_r0, _r1) \
	LICE_CMD_BASE_DATA(_r0, _r1, 0, ADDR_OFFSET_BEGIN_TRAP)

#define LICE_CMD_END_TRAP(_r0, _r1) \
	LICE_CMD_BASE_DATA(_r0, _r1, 0, ADDR_OFFSET_END_TRAP)

#define LICE_CMD_BEGIN_FTRAP(_r0, _r1) \
	LICE_CMD_BASE_DATA(_r0, _r1, 0, ADDR_OFFSET_BEGIN_FTRAP)

#define LICE_CMD_END_FTRAP(_r0, _r1) \
	LICE_CMD_BASE_DATA(_r0, _r1, 0, ADDR_OFFSET_END_FTRAP)

#define LICE_CMD_HYPERCALL_NR(_r0, _r1, info_r2) \
	LICE_CMD_BASE_REG(_r0, _r1, info_r2, ADDR_OFFSET_HYPERCALL_NR)

#define LICE_CMD_ASM_HYPERCALL_NR(_r0, _r1, info_r2) \
	LICE_CMD_BASE_REG(_r0, _r1, info_r2, ADDR_OFFSET_ASM_HYPERCALL_NR)

#define LICE_CMD_END_ASM_HYPERCALL(_r0, _r1) \
	LICE_CMD_BASE_DATA(_r0, _r1, 0, ADDR_OFFSET_END_ASM_HYPERCALL)


#endif   /* __ASSEMBLY */

#endif   /*CONFIG_DEV_LICE_INTERFACE OR CONFIG_DEV_LICE_SCHEDULING*/

