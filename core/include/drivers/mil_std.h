/*
 * $FILE: mil_std.h
 *
 *
 * $VERSION$
 *
 * Author: Javier O. Coronel <jcoronel@fentiss.com>
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

/*
  Changelog:
  - [05/08/2015:SPR-010915-01] 1553 driver reimplemented.
*/

#ifndef _XM_DEV_MIL_STD_H_
#define _XM_DEV_MIL_STD_H_

/*Base address Core1553BRM*/
#define MIL_STD_BASE	0xFFF00000

/*Core1553BRM Registers*/
#define CORE1553BRM_CTRL        0x00   /*Register 0*/
   #define CORE1553BRM_CTRL_STEX    (1<<15)
   #define CORE1553BRM_CTRL_SBIT    (1<<14)
   #define CORE1553BRM_CTRL_SRST    (1<<13)
   #define CORE1553BRM_CTRL_BAEN    (1<<12)
   #define CORE1553BRM_CTRL_BBEN    (1<<11)
   #define CORE1553BRM_CTRL_ETCE    (1<<10)
   #define CORE1553BRM_CTRL_MSGTO   (1<<9)
   #define CORE1553BRM_CTRL_BUFM_BIT   7   //Set buffer mode BIT 7:8
   /*Buffer modes for RT*/
      #define CORE1553BRM_CTRL_BUFM_NCIR_M0      (0x00<<CORE1553BRM_CTRL_BUFM_BIT)
      #define CORE1553BRM_CTRL_BUFM_NCIR_BIS_M0  (0x01<<CORE1553BRM_CTRL_BUFM_BIT)
      #define CORE1553BRM_CTRL_BUFM_CIR_M1       (0x02<<CORE1553BRM_CTRL_BUFM_BIT)
      #define CORE1553BRM_CTRL_BUFM_CIR_M2       (0x03<<CORE1553BRM_CTRL_BUFM_BIT)
   #define CORE1553BRM_CTRL_BMC     (1<<5)
   #define CORE1553BRM_CTRL_BCEN    (1<<4)
   #define CORE1553BRM_CTRL_DYNBC   (1<<3)
   #define CORE1553BRM_CTRL_PPEN    (1<<2)
   #define CORE1553BRM_CTRL_INTEN    (1<<1)
   #define CORE1553BRM_CTRL_XMTSW   (1<<0)
#define CORE1553BRM_OP_STATUS   0x04   /*Register 1*/
   #define CORE1553BRM_OP_STATUS_RTA_BIT  11  // Setting these bits determines the RT address for the core - Bits 11:15
   #define CORE1553BRM_OP_STATUS_RTPTY  (1<<10)  //Set odd parity for the RT address
   #define CORE1553BRM_OP_STATUS_MSEL_BIT  8  //Mode Select - Bits 8:9
   /*Modes Core1554BRM*/
      #define CORE1553BRM_OP_STATUS_MODE_BC        (0x00<<CORE1553BRM_OP_STATUS_MSEL_BIT)
      #define CORE1553BRM_OP_STATUS_MODE_RT        (0x01<<CORE1553BRM_OP_STATUS_MSEL_BIT)
      #define CORE1553BRM_OP_STATUS_MODE_BM        (0x02<<CORE1553BRM_OP_STATUS_MSEL_BIT)
      #define CORE1553BRM_OP_STATUS_MODE_BM_RT     (0x03<<CORE1553BRM_OP_STATUS_MSEL_BIT)  //Bus Monitor and Remote Terminal
   #define CORE1553BRM_OP_STATUS_AB_STD  (1<<7)  //A/B STD is set LOW for MIL-STD-1553B operation and HIGH for MIL_STD-1553A.
#define CORE1553BRM_CMD         0x08   /*Register 2*/
#define CORE1553BRM_IT_MASK     0x0C   /*Register 3*/
#define CORE1553BRM_IT_PEND     0x10   /*Register 4*/
#define CORE1553BRM_IT_PTR      0x14   /*Register 5*/
#define CORE1553BRM_BUILT_IN    0x18   /*Register 6*/
#define CORE1553BRM_TIME_TAG    0x1C   /*Register 7*/
#define CORE1553BRM_DESC_PTR    0x20   /*Register 8*/
#define CORE1553BRM_STATUS_WORD 0x24   /*Register 9*/
#define CORE1553BRM_ENH_FEAT    0x80   /*Register 32*/
   #define CORE1553BRM_ENH_FEAT_CLKFREQ_BIT 0
      #define CORE1553BRM_ENH_FEAT_CLKFREQ_12 (0x0)
      #define CORE1553BRM_ENH_FEAT_CLKFREQ_16 (0x1)
      #define CORE1553BRM_ENH_FEAT_CLKFREQ_20 (0x2)
      #define CORE1553BRM_ENH_FEAT_CLKFREQ_24 (0x3)
#define CORE1553BRM_RT_CMD_LEGAL 0x40


/*Interrupts on Core1553BRM*/
#define CORE1553BRM_IRQ_DMAF    (1<<15)  /*HW IRQ*/
#define CORE1553BRM_IRQ_WRAPF   (1<<14)  /*HW IRQ*/
#define CORE1553BRM_IRQ_TAPF    (1<<13)  /*HW IRQ*/
#define CORE1553BRM_IRQ_BITF    (1<<12)  /*HW IRQ*/
#define CORE1553BRM_IRQ_MERR    (1<<11)  /*MSG IRQ*/
#define CORE1553BRM_IRQ_SUBAD   (1<<10)  /*MSG IRQ*/
#define CORE1553BRM_IRQ_BDRCV   (1<<9)   /*MSG IRQ*/
#define CORE1553BRM_IRQ_IXEQ0   (1<<8)   /*MSG IRQ*/
#define CORE1553BRM_IRQ_ILLCMD  (1<<7)   /*MSG IRQ*/
#define CORE1553BRM_IRQ_N_A     (1<<6)
#define CORE1553BRM_IRQ_EOL     (1<<5)   /*MSG IRQ*/
#define CORE1553BRM_IRQ_ILLOCMD (1<<4)   /*MSG IRQ*/
#define CORE1553BRM_IRQ_ILLOP   (1<<3)   /*MSG IRQ*/
#define CORE1553BRM_IRQ_RTF     (1<<2)   /*MSG IRQ*/
#define CORE1553BRM_IRQ_CBA     (1<<1)   /*MSG IRQ*/
#define CORE1553BRM_IRQ_MBC     (1<<0)   /*MSG IRQ*/


/*Mode Core1554BRM*/
/*#define CORE1553BRM_MODE_BC        0x00
#define CORE1553BRM_MODE_RT        0x01
#define CORE1553BRM_MODE_BM        0x10
#define CORE1553BRM_MODE_BM_RT     0x11*/

#define MIL_STD_CORE1553BRM_ADDR_RT  (CONFIG_MIL_RT_ADDR)

/*B1553BRM status/control*/
#define MIL_STD_STATUS_CTRL  0xFFF00100
   #define MIL_STD_STATUS_CTRL_RESET (1<<12)
   #define MIL_STD_STATUS_CTRL_CLK_SEL (1<<9)
   #define MIL_STD_STATUS_CTRL_CLK_DIV (1<<5)
   #define MIL_STD_STATUS_CTRL_SSYSFN (1<<0)

/*B1553BRM interrupt settings*/
#define MIL_STD_IT_SETTING   0xFFF00104
   #define MIL_STD_IT_SETTING_INTACKM       (1 << 2)
   #define MIL_STD_IT_SETTING_INTACKH       (1 << 1)
   #define MIL_STD_IT_SETTING_INTLEVEL      (1 << 0)

/*AHB page address register*/
#define MIL_STD_AHB_PAGE_ADR 0xFFF00108

/*MIL-STD-1553B CLOCK in GRGPREG*/
#define MIL_STD_1553_CLK_SELECT_BIT       5
  #define CLK_SELECT_GEN        0
  #define CLK_SELECT_CLK_INPUT  1

#define MIL_STD_1553_CLK_DIVH_BIT         14
#define MIL_STD_1553_CLK_DIVL_BIT         16

#define MIL_STD_1553_CLK_DIV_OUT_BIT      18
  #define CLK_DIV_OUT_SYS_CLK   1
  #define CLK_DIV_OUT_DIV_CLK   0


/*CLOCK GATE*/
#define CLKGATE_UNLOCK_REG 0x0
#define CLKGATE_CLOCK_EN_REG 0x4
#define CLKGATE_CLOCK_RST_REG 0x8

#define CLKGATE_MIL_STD_1553	11

/*Mode Codes*/
#define SYNC_MODE_CODE 17

#define IAW_MODE_CODE (1<<8)

/*Special structures to receive data - from external driver*/

/* Descriptor Block Control Word bits */
#define C_DBCW_INTX              (1 << 7)
#define C_DBCW_IWA               (1 << 6)
#define C_DBCW_IBRD              (1 << 5)
#define C_DBCW_BAC               (1 << 4)
#define C_DBCW_LAB               (1 << 3)
#define C_DBCW_AB                (1 << 2)
#define C_DBCW_BRD               (1 << 1)
#define C_DBCW_NII               (1)

/* Message Information Word bits */
#define C_MIW_CHA              (1 << 9)
#define C_MIW_RTRT             (1 << 8)
#define C_MIW_ME               (1 << 7)
#define C_MIW_BC               (1 << 5)
#define C_MIW_ILL              (1 << 4)
#define C_MIW_TO               (1 << 3)
#define C_MIW_OVR              (1 << 2)
#define C_MIW_PRTY             (1 << 1)
#define C_MIW_MAN              (1)

#define C_NB_MAX_SA          (32)

#define SIZE_DATA_POINTER_STRUCT  ((C_NB_MAX_SA+2)*2) /*Size of Data Pointer A/B and Broadcast Pointer structs*/

#define OFFSET_ADD(ofs) (((unsigned int) &(ofs) & 0x1ffff)>>1)

/*Descriptor table*/
typedef struct descBlocks
{
    volatile xm_u16_t controlWord;     /* Control word */
    volatile xm_u16_t dataPointerA;    /* data pointer A */
    volatile xm_u16_t dataPointerB;     /* data pointer B */
    volatile xm_u16_t broadcastPointer;   /* broadcast pointer */
}descBlocks_t;


/*Log list*/
typedef struct irqLogList
{
    volatile xm_u16_t iiw;
    volatile xm_u16_t iaw;
}irqLogList_t;

typedef struct circBuff
{
    volatile xm_u8_t msgs[SIZE_DATA_POINTER_STRUCT];
}circBuff_t;

typedef struct desc1553Dev
{
    /* Descriptor tables (4 X (4 X 32) = 512 words = 1024 bytes) */
    descBlocks_t descRx[C_NB_MAX_SA]           __attribute__((aligned(1024)));
    descBlocks_t descTx[C_NB_MAX_SA];
    descBlocks_t descModeCodeRx[C_NB_MAX_SA];
    descBlocks_t descModeCodeTx[C_NB_MAX_SA];

    /* RX Sub Address messages */
    circBuff_t rxSubAddrMsgs[C_NB_MAX_SA];
    /* TX Sub Address messages */
    circBuff_t txSubAddrMsgs[C_NB_MAX_SA];
    /* RX Mode Code messages */
    circBuff_t rxModeCodeMsgs[C_NB_MAX_SA];
    /* TX Mode Code messages */
    circBuff_t txModeCodeMsgs[C_NB_MAX_SA];

    /* offset to last 64bytes of 128k tot-used-needed */
    xm_u16_t unused[(64*1024-(4*32*4+4*32*9*34))-16*2];

    /* interrupt log*/
    irqLogList_t irqLogs[16]               __attribute__((aligned(1024)));
}desc1553Dev_t;

#endif /* _XM_DEV_MIL_STD_H_ */


