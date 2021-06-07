/*
 * $FILE: spw_irq.h
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

#ifndef _XM_DEV_SPW_H_
#define _XM_DEV_SPW_H_

#ifdef CONFIG_SPW_RTC

/*Base address of the secundary CTRL_IT*/
#define RTC_CTRL_IT2	0x800000B0

#define RTC_SPW_IT2_TO_CPU_PIO5	      5

#define PIO_IT2_TO_CPU		      RTC_SPW_IT2_TO_CPU_PIO5

#define RTC_SPW_NO_HWIRQS	CONFIG_NO_HWIRQS-CONFIG_CPU_NO_IRQS
#define CPU_NO_HWIRQS		CONFIG_CPU_NO_IRQS

/*Interruption sources in the IT2*/
#define RTC_IT2_SPW1_IRQ	15
#define RTC_IT2_SPW0_IRQ	13

#define RTC_IT2_MASK	0x0
    #define IT2_MASK_SPW1_BIT		(1<<RTC_IT2_SPW1_IRQ)
    #define IT2_MASK_SPW0_BIT		(1<<RTC_IT2_SPW0_IRQ)
    
#define RTC_IT2_PEND	0x4
    #define IT2_PEND_SPW1_BIT		(1<<RTC_IT2_SPW1_IRQ)
    #define IT2_PEND_SPW0_BIT		(1<<RTC_IT2_SPW0_IRQ)

#define RTC_IT2_CLEAR	0xC
    #define IT2_CLEAR_SPW1_BIT		(1<<RTC_IT2_SPW1_IRQ)
    #define IT2_CLEAR_SPW0_BIT		(1<<RTC_IT2_SPW0_IRQ)

#define SPW_LINK_IF0	0x80060000
#define SPW_LINK_IF1	0x80070000
    #define SPW_PISR	0x8    /*Pending Interrupt Status Register*/
    #define SPW_LISCR	0x1c   /*Interrupt Status Clear Register*/
    #define SPW_LISR    0x14   /*Link Interrupt Status Register*/
    #define SPW_IMR     0x10   /*Interrupt Mask Register*/
	#define SPW_DISS_ERR_BIT      (1<<0)
	#define SPW_PAR_ERR_BIT       (1<<1)
	#define SPW_ESC_ERR_BIT       (1<<2)
	#define SPW_CR_ERR_BIT        (1<<3)
	#define SPW_RX_TIME_CODE_BIT  (1<<4)
	#define SPW_TX_TIME_CODE_BIT  (1<<5)
	#define SPW_PKT_REJ_BIT       (1<<6)
	#define SPW_LINK_ABORT_BIT    (1<<7)


/* <track id="interrupt-to-trap-list-spw-rtc"> */
#define SPW_INTERRUPT_LEVEL_16 0x3f // IT2_SPW1 // 31
#define SPW_INTERRUPT_LEVEL_15 0x3e
#define SPW_INTERRUPT_LEVEL_14 0x3d // IT2_SPW0 // 29
#define SPW_INTERRUPT_LEVEL_13 0x3c
#define SPW_INTERRUPT_LEVEL_12 0x3b
#define SPW_INTERRUPT_LEVEL_11 0x3a
#define SPW_INTERRUPT_LEVEL_10 0x39
#define SPW_INTERRUPT_LEVEL_9  0x38
#define SPW_INTERRUPT_LEVEL_8  0x37
#define SPW_INTERRUPT_LEVEL_7  0x36
#define SPW_INTERRUPT_LEVEL_6  0x35
#define SPW_INTERRUPT_LEVEL_5  0x34
#define SPW_INTERRUPT_LEVEL_4  0x33
#define SPW_INTERRUPT_LEVEL_3  0x32
#define SPW_INTERRUPT_LEVEL_2  0x31
#define SPW_INTERRUPT_LEVEL_1  0x30
/* </track id="interrupt-to-trap-list-spw-rtc"> */







#endif 
#endif

