/*
 * $FILE: sdp.c
 *
 * $VERSION$
 *
 * Author: María Soler Heredia <msolher@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <irqs.h>
#include <hypercalls.h>

extern xm_u32_t IoPortLogSearch(xm_u32_t port);

__hypercall xm_s32_t DrvIoOutportSDPSys(xm_u8_t *dst, xm_u8_t *src, xm_u32_t size) {
        int *MCFG1 = (int*)0x80000000;
        int old_MCFG1=*MCFG1;
        int i;
        volatile xm_u8_t* Dst;
        volatile xm_u8_t* Src;
        //volatile xm_u8_t* addrRef;
        volatile xm_u8_t* EEPROM5555;
        volatile xm_u8_t* EEPROMAAAA;

        xm_u32_t EEPROM_BASE,mask;
	
        Dst = dst;
        Src = src;
        //addrRef  = Dst;
	
	if(Dst<(xm_u8_t*)0x10000000||Dst>=(xm_u8_t*)0x10080000) {
	  return XM_INVALID_PARAM;
	}	  
	
	if (!(mask=IoPortLogSearch((xm_u32_t)Dst))) {
	  return XM_PERM_ERROR;
	}
        
        EEPROM_BASE= (xm_u32_t)Dst & 0xFFFF0000;
       
	if(size>0) {
	        if (((xm_u32_t)(Dst+size-1)& 0xFFFF0000) != EEPROM_BASE) {
        	  return XM_INVALID_PARAM;
	        }
	
        	EEPROM5555 = (xm_u8_t*)(EEPROM_BASE + 0x00005555);
	        EEPROMAAAA = (xm_u8_t*)(EEPROM_BASE + 0x0000AAAA);
                //addrRef = (xm_u8_t*)(EEPROM_BASE + 0x0000FFFF);

		HwDisableIrq(TIMER1_TRAP_NR);
		HwDisableIrq(TIMER2_TRAP_NR);
	
		*MCFG1 = 0x102808ff;
	
	        *(EEPROM5555) = (xm_u8_t)0xAA;
	        *(EEPROMAAAA) = (xm_u8_t)0x55;
        	*(EEPROM5555) = (xm_u8_t)0xA0;

	        for(i=0;i<size;i++)
        	    Dst[i] = Src[i];
	        // while ((*(addrRef) != *(addrRef)));
        	*MCFG1 = old_MCFG1;

		HwEnableIrq(TIMER1_TRAP_NR);
		HwEnableIrq(TIMER2_TRAP_NR);
	}
	else{
                //while(*Dst!=*Dst);
		*MCFG1=old_MCFG1&~0x800;
	}
	return XM_OK;
}

__hypercall xm_s32_t DrvIoDisableSDPSys(xm_u32_t chipSelect) {
	int *MCFG1 = (int*)0x80000000;
        static volatile xm_u32_t Dst;
        static volatile xm_u8_t *EEPROM5555;
        static volatile xm_u8_t *EEPROMAAAA;
	xm_u32_t mask;

        xm_u32_t EEPROM_BASE;
        Dst=chipSelect;
	
	if (!(mask=IoPortLogSearch((xm_u32_t)Dst))) {
          return XM_PERM_ERROR;
        }	

        EEPROM_BASE= Dst & 0xFFFF0000;
	if(EEPROM_BASE!=0x10000000 && EEPROM_BASE!=0x10020000 
		&& EEPROM_BASE!= 0x10040000 && EEPROM_BASE!=0x10060000) {
		return XM_INVALID_PARAM;
	}
        EEPROM5555 = (xm_u8_t*)(EEPROM_BASE + 0x00005555);
        EEPROMAAAA = (xm_u8_t*)(EEPROM_BASE + 0x0000AAAA);

	HwDisableIrq(TIMER1_TRAP_NR);
	HwDisableIrq(TIMER2_TRAP_NR);
	
	*MCFG1|=0x808ff;
	
        *(EEPROM5555) = (xm_u8_t)0xAA;
        *(EEPROMAAAA) = (xm_u8_t)0x55;
        *(EEPROM5555) = (xm_u8_t)0x80;
	*(EEPROM5555) = (xm_u8_t)0xAA;
        *(EEPROMAAAA) = (xm_u8_t)0x55;
        *(EEPROM5555) = (xm_u8_t)0x20;
	
	//while ((*(EEPROM5555) != *(EEPROM5555)));
	
	HwEnableIrq(TIMER1_TRAP_NR);
	HwEnableIrq(TIMER2_TRAP_NR);
	
	return XM_OK;
}
