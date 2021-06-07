/*
 * $FILE: fpga_itarfree.h
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

#ifndef _XM_DEV_FPGA_ITARFREE_H_
#define _XM_DEV_FPGA_ITARFREE_H_

#ifdef CONFIG_DEV_FPGA_ITARFREE

/*Lines I/O*/
#define FPGA_TO_CPU_PIO4	      4
#define FPGA_TO_CPU_PIO5	      5

/*Line that generate an interruption in the processor*/
#define PIO_FPGA_TO_CPU		      FPGA_TO_CPU_PIO5

/*Number of interrupts in the FPGA*/
#define ITAR_FREE_NO_HWIRQS    	CONFIG_NO_HWIRQS-CONFIG_CPU_NO_IRQS
#define CPU_NO_HWIRQS		CONFIG_CPU_NO_IRQS

/*Interruption sources in the FPGA*/
#define IT_SPW_TX	14
#define IT_SPW_RX	13
#define IT_OSLNK_TX	12
#define IT_OSLNK_RX 	11
#define IT_IOTOP 	10
#define IT_UART_RX 	9
#define IT_UART_TX 	8
#define IT_IO3 		7
#define IT_IO2 		6
#define IT_IO1 		5
#define IT_IO0 		4
#define IT_32HZ 	3
#define IT_1HZ 		2
#define IT_EADR 	1
#define IT_SDRAM 	0

/* <track id="interrupt-to-trap-list-fpga-itarfree"> */
#define FPGA_INTERRUPT_LEVEL_15 0x3e // IT_SPW_TX // 30
#define FPGA_INTERRUPT_LEVEL_14 0x3d // IT_SPW_RX // 29
#define FPGA_INTERRUPT_LEVEL_13 0x3c // IT_OSLNK_TX // 28
#define FPGA_INTERRUPT_LEVEL_12 0x3b // IT_OSLNK_RX // 27
#define FPGA_INTERRUPT_LEVEL_11 0x3a // IT_IOTOP // 26
#define FPGA_INTERRUPT_LEVEL_10 0x39 // IT_UART_RX // 25
#define FPGA_INTERRUPT_LEVEL_9  0x38 // IT_UART_TX // 24
#define FPGA_INTERRUPT_LEVEL_8  0x37 // IT_IO3 // 23
#define FPGA_INTERRUPT_LEVEL_7  0x36 // IT_IO2 // 22
#define FPGA_INTERRUPT_LEVEL_6  0x35 // IT_IO1 // 21
#define FPGA_INTERRUPT_LEVEL_5  0x34 // IT_IO0 // 20
#define FPGA_INTERRUPT_LEVEL_4  0x33 // IT_32HZ // 19
#define FPGA_INTERRUPT_LEVEL_3  0x32 // IT_1HZ // 18
#define FPGA_INTERRUPT_LEVEL_2  0x31 // IT_EADR // 17 XM reserved
#define FPGA_INTERRUPT_LEVEL_1  0x30 // IT_SDRAM // 16 XM reserved
/* </track id="interrupt-to-trap-list-fpga-itarfree"> */

/*Base address of the secundary CTRL_IT*/
#define FPGA_CTRL_IT	0x28000D00

#define FPGA_IT_MASK	0x4
    #define IT_MASK_SPW_TX_BIT		(1<<14)
    #define IT_MASK_SPW_RX_BIT		(1<<13)
    #define IT_MASK_OSLNK_TX_BIT	(1<<12)
    #define IT_MASK_OSLNK_RX_BIT 	(1<<11)
    #define IT_MASK_IOTOP_BIT 		(1<<10)
    #define IT_MASK_UART_RX_BIT 	(1<<9)
    #define IT_MASK_UART_TX_BIT 	(1<<8)
    #define IT_MASK_IO3_BIT 		(1<<7)
    #define IT_MASK_IO2_BIT 		(1<<6)
    #define IT_MASK_IO1_BIT 		(1<<5)
    #define IT_MASK_IO0_BIT 		(1<<4)
    #define IT_MASK_32HZ_BIT 		(1<<3)
    #define IT_MASK_1HZ_BIT 		(1<<2)
    #define IT_MASK_EADR_BIT 		(1<<1)
    #define IT_MASK_SDRAM_BIT 		(1<<0)

#define FPGA_IT_PEND	0x8
    #define IT_PEND_SPW_TX_BIT		(1<<14)
    #define IT_PEND_SPW_RX_BIT		(1<<13)
    #define IT_PEND_OSLNK_TX_BIT	(1<<12)
    #define IT_PEND_OSLNK_RX_BIT 	(1<<11)
    #define IT_PEND_IOTOP_BIT 		(1<<10)
    #define IT_PEND_UART_RX_BIT 	(1<<9)
    #define IT_PEND_UART_TX_BIT 	(1<<8)
    #define IT_PEND_IO3_BIT 		(1<<7)
    #define IT_PEND_IO2_BIT 		(1<<6)
    #define IT_PEND_IO1_BIT 		(1<<5)
    #define IT_PEND_IO0_BIT 		(1<<4)
    #define IT_PEND_32HZ_BIT 		(1<<3)
    #define IT_PEND_1HZ_BIT 		(1<<2)
    #define IT_PEND_EADR_BIT 		(1<<1)
    #define IT_PEND_SDRAM_BIT 		(1<<0)

#define FPGA_IT_CLEAR	0xC
    #define IT_CLEAR_SPW_TX_BIT		(1<<14)
    #define IT_CLEAR_SPW_RX_BIT		(1<<13)
    #define IT_CLEAR_OSLNK_TX_BIT	(1<<12)
    #define IT_CLEAR_OSLNK_RX_BIT 	(1<<11)
    #define IT_CLEAR_IOTOP_BIT 		(1<<10)
    #define IT_CLEAR_UART_RX_BIT 	(1<<9)
    #define IT_CLEAR_UART_TX_BIT 	(1<<8)
    #define IT_CLEAR_IO3_BIT 		(1<<7)
    #define IT_CLEAR_IO2_BIT 		(1<<6)
    #define IT_CLEAR_IO1_BIT 		(1<<5)
    #define IT_CLEAR_IO0_BIT 		(1<<4)
    #define IT_CLEAR_32HZ_BIT 		(1<<3)
    #define IT_CLEAR_1HZ_BIT 		(1<<2)
    #define IT_CLEAR_EADR_BIT 		(1<<1)
    #define IT_CLEAR_SDRAM_BIT 		(1<<0)

#endif /*CONFIG_DEV_FPGA_ITARFREE*/

#endif /*_XM_DEV_FPGA_ITARFREE_H_*/

