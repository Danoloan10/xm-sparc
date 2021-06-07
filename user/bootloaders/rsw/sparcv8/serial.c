/*
 * $FILE: serial.c
 *
 * Generic code to access the serial
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifdef CONFIG_OUTPUT_ENABLED
#include <xm_inc/sparcv8/leon.h>

#define DATA_REG 0x0
#define STTS_REG 0x4
#define   STTS_FE_B 0x40
#define   STTS_PE_B 0x20
#define   STTS_OV_B 0x10
#define   STTS_BR_B 0x08
#define   STTS_TH_B 0x04
#define   STTS_TS_B 0x02
#define   STTS_DR_B 0x01
#define CTRL_REG 0x8
#define   CTRL_EC_B 0x100
#define   CTRL_LB_B 0x080
#define   CTRL_FL_B 0x040
#define   CTRL_PE_B 0x020
#define   CTRL_PS_B 0x010
#define   CTRL_TI_B 0x008
#define   CTRL_RI_B 0x004
#define   CTRL_TE_B 0x002
#define   CTRL_RE_B 0x001
#define SCLR_REG 0xC
#define   SCLR_VAL_M 0xfff
#define StoreIoReg(_r, _v) (*((volatile xm_u32_t *)(_r))=(volatile xm_u32_t)(_v))
#define LoadIoReg(_r) *((volatile xm_u32_t *)(_r))

#ifdef CONFIG_UART1
#define DEFAULT_UART_BASE LEON_UART1_BASE
#endif

#ifdef CONFIG_UART2
#define DEFAULT_UART_BASE LEON_UART2_BASE
#endif
#define SCAR_REG 0x4

void InitOutput(void) {
    xm_u32_t cpuFreq;
    StoreIoReg(STTS_REG+DEFAULT_UART_BASE, 0);

#ifdef CONFIG_UART_THROUGH_DSU
    StoreIoReg(CTRL_REG+DEFAULT_UART_BASE, CTRL_TE_B|CTRL_RE_B|CTRL_LB_B|CTRL_FL_B);
#else
#ifdef CONFIG_UART_FLOWCONTROL
    StoreIoReg(CTRL_REG+DEFAULT_UART_BASE, CTRL_TE_B|CTRL_RE_B|CTRL_FL_B);
#else
    StoreIoReg(CTRL_REG+DEFAULT_UART_BASE, CTRL_TE_B|CTRL_RE_B);
#endif
#endif
#ifdef CONFIG_CPU_FREQ_AUTO
    cpuFreq=(LoadIoReg(LEON_TIMER_CFG_BASE+SCAR_REG)+1)*1000;
#else
    cpuFreq=CONFIG_CPU_KHZ;
#endif
    StoreIoReg(SCLR_REG+DEFAULT_UART_BASE, (((cpuFreq*1000*10)/(CONFIG_UART_BAUDRATE*8))-5)/10);
}

void xputchar(int c) {
    xm_s32_t loop=0;
    while(!(LoadIoReg(DEFAULT_UART_BASE+STTS_REG)&STTS_TH_B)&&loop<CONFIG_UART_TIMEOUT) loop++;
    StoreIoReg(DATA_REG+DEFAULT_UART_BASE, c);
    loop=0;
    while(!(LoadIoReg(DEFAULT_UART_BASE+STTS_REG)&STTS_TS_B)&&loop<CONFIG_UART_TIMEOUT) loop++;
}

#else

void InitOutput(void) {}
void xputchar(int c) {}

#endif
