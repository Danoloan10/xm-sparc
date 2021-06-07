/*
 * $FILE: boot.S
 *
 * Generic boot code, sets up a valid stack and jumps to the C code
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

/* <track id="PARTITION_BOOT_CODE"> */		
#include <xm.h>
#include <xm_inc/arch/asm_offsets.h>
//#include <xm_inc/hypercalls.h>
	
#define STACK_SIZE 8192
#define MIN_STACK_FRAME 0x60
	
.text
.align 8

.global start, _start

_start:	
start:
	/* Zero out our BSS section. */
	set _sbss, %o0
	set _ebss, %o1

1:
	st %g0, [%o0]
	subcc %o0, %o1, %g0
	bl 1b
	add %o0, 0x4, %o0
	
	set __stack_top, %fp

        mov %g1, %o0
	call init_libxm
	sub %fp, MIN_STACK_FRAME, %sp        

	! Set up TBR
	set sparc_write_tbr_nr, %o0
	set _traptab, %o1
	__XM_HC

	call PartitionMain
	sub %fp, MIN_STACK_FRAME, %sp

        set libXmParams, %o0
        ld    [%o0], %o1
        ld    [%o1 + 0x1c], %o1
        mov halt_partition_nr, %o0
        __XM_HC

/*        set ~0, %o1
	mov halt_partition_nr, %o0
	__XM_HC*/

1:	b 1b
	nop

ExceptionHandlerAsm:
        mov sparc_get_psr_nr, %o0
        __XM_AHC
        mov %o0, %l0
!	set sparc_flush_regwin_nr, %o0
!	__XM_AHC
        sub %fp, 48, %fp
        std %l0, [%fp+40]
        std %l2, [%fp+32]
        std %g6, [%fp+24]
        std %g4, [%fp+16]
        std %g2, [%fp+8]
        st %g1, [%fp+4]
        rd %y, %g5
        st %g5, [%fp]

	mov %l5, %o0
	call ExceptionHandler
	sub %fp, 0x80, %sp
	ld [%fp], %g1
        wr %g1, %y
        ld [%fp+4], %g1
        ldd [%fp+8], %g2
        ldd [%fp+16], %g4
        ldd [%fp+24], %g6
        ldd [%fp+32], %l2
        ldd [%fp+40], %l0
        add %fp, 48, %fp
        mov %l0, %o1
        mov sparc_set_psr_nr, %o0
        __XM_AHC
	set sparc_iret_nr, %o0
	__XM_AHC
	
ExtIrqHandlerAsm:
        mov sparc_get_psr_nr, %o0
        __XM_AHC
        mov %o0, %l0
!	set sparc_flush_regwin_nr, %o0
!	__XM_AHC
        sub %fp, 48, %fp
        std %l0, [%fp+40]
        std %l2, [%fp+32]
        std %g6, [%fp+24]
        std %g4, [%fp+16]
        std %g2, [%fp+8]
        st %g1, [%fp+4]
        rd %y, %g5
        st %g5, [%fp]
	mov %l5, %o0
	call ExtIrqHandler
	sub %fp, 0x80, %sp
	ld [%fp], %g1
        wr %g1, %y
        ld [%fp+4], %g1
        ldd [%fp+8], %g2
        ldd [%fp+16], %g4
        ldd [%fp+24], %g6
        ldd [%fp+32], %l2
        ldd [%fp+40], %l0
        add %fp, 48, %fp
        mov %l0, %o1
        mov sparc_set_psr_nr, %o0
        __XM_AHC
	set sparc_iret_nr, %o0
	__XM_AHC

HwIrqHandlerAsm:
        mov sparc_get_psr_nr, %o0
        __XM_AHC
        mov %o0, %l0
!	set sparc_flush_regwin_nr, %o0
!	__XM_AHC
        sub %fp, 48, %fp
        std %l0, [%fp+40]
        std %l2, [%fp+32]
        std %g6, [%fp+24]
        std %g4, [%fp+16]
        std %g2, [%fp+8]
        st %g1, [%fp+4]
        rd %y, %g5
        st %g5, [%fp]
	mov %l5, %o0
	call HwIrqHandler
	sub %fp, 0x80, %sp

	ld [%fp], %g1
        wr %g1, %y
        ld [%fp+4], %g1
        ldd [%fp+8], %g2
        ldd [%fp+16], %g4
        ldd [%fp+24], %g6
        ldd [%fp+32], %l2
        ldd [%fp+40], %l0
        add %fp, 48, %fp
        mov %l0, %o1
        restore
        save
        mov sparc_set_psr_nr, %o0
        __XM_AHC
	set sparc_iret_nr, %o0
	__XM_AHC
	
.data
.align 8
__stack:
        .fill (STACK_SIZE/4),4,0
__stack_top:

.previous


#define BUILD_IRQ(irqnr) \
	sethi %hi(HwIrqHandlerAsm), %l4 ;\
	jmpl %l4 + %lo(HwIrqHandlerAsm), %g0 ;\
	mov irqnr, %l5	;\
        nop
	
#define BUILD_TRAP(trapnr) \
	sethi %hi(ExceptionHandlerAsm), %l4 ;\
	jmpl %l4 + %lo(ExceptionHandlerAsm), %g0 ;\
	mov trapnr, %l5	;\
	nop		;
	
#define BAD_TRAP(trapnr) \
1:      b 1b	;\
        nop 	;\
        nop 	;\
        nop
	
#define SOFT_TRAP(trapnr) \
1:      b 1b	;\
        nop 	;\
        nop 	;\
        nop
	
#define BUILD_EXTIRQ(trapnr) \
	sethi %hi(ExtIrqHandlerAsm), %l4 ;\
	jmpl %l4 + %lo(ExtIrqHandlerAsm), %g0 ;\
	mov (trapnr+224), %l5	;\
        nop

.align 4096
_traptab:
! + 0x00: reset
t_reset: b start
	nop
	nop
	nop

! + 0x01: instruction_access_exception 
	BUILD_TRAP(0x1)
	
! + 0x02: illegal_instruction 
	BUILD_TRAP(0x2)
	
! + 0x03: privileged_instruction 
	BUILD_TRAP(0x3)
	
! + 0x04: fp_disabled 
	BUILD_TRAP(0x4)
	
! + 0x05: window_overflow
	BUILD_TRAP(0x5)
	

! ..........
/* </track id="PARTITION_BOOT_CODE"> */
! + 0x06: window_underflow 
	BUILD_TRAP(0x6)
	
! + 0x07: mem_address_not_aligned 
	BUILD_TRAP(0x7)
	
! + 0x08: fp_exception 
	BUILD_TRAP(0x8)
	
! + 0x09: data_access_exception 
	BUILD_TRAP(0x9)
	
! + 0x0A: tag overflow 
	BUILD_TRAP(0xA)
	
! + 0x0B: watchpoint_detected 
	BUILD_TRAP(0xB)

	BAD_TRAP(0xC)
	BAD_TRAP(0xD)
	BAD_TRAP(0xE)
	BAD_TRAP(0xF)
	BAD_TRAP(0x10)
	
! - 0x11: internal bus error 
	BUILD_TRAP(0x11)
	
! - 0x12: UART2 
	BUILD_IRQ(0x2)
	
! - 0x13: UART1 
	BUILD_IRQ(0x3)
	
! - 0x14: I/O interrupt[0] 
	BUILD_IRQ(0x4)
	
! - 0x15: I/O interrupt[1] 
	BUILD_IRQ(0x5)
	
! - 0x16: I/O interrupt[2] 
	BUILD_IRQ(0x6)
	
! - 0x17: I/O interrupt[3] 
	BUILD_IRQ(0x7)
	
! - 0x18: timer 1 
	BUILD_IRQ(0x8)
	
! - 0x19: timer 2 
	BUILD_IRQ(0x9)
	
! - 0x1A: unused 
	BUILD_IRQ(0xA)
	
! - 0x1B: DSU trace buffer 	
	BUILD_IRQ(0xB)
	
! - 0x1C: unused 
	BUILD_IRQ(0xC)
	
! - 0x1D: unused 
	BUILD_IRQ(0xD)
	
! - 0x1E: PCI 
	BUILD_IRQ(0xE)
	
! - 0x1F: unused 
	BUILD_IRQ(0xF)
	
! + 0x20: register_hardware_error 
	BUILD_TRAP(0x20)
	
	BAD_TRAP(0x21)
	BAD_TRAP(0x22)
	BAD_TRAP(0x23)
	
! + 0x24: cp_disabled 
	BUILD_TRAP(0x24)

	BAD_TRAP(0x25)
	BAD_TRAP(0x26)
	BAD_TRAP(0x27)
	BAD_TRAP(0x28)
	BAD_TRAP(0x29)
	
! + 0x2A: divide_exception 
	BUILD_TRAP(0x2A)
	
! + 0x2B: write error 
	BUILD_TRAP(0x2B)
	
	BAD_TRAP(0x2C); BAD_TRAP(0x2D); BAD_TRAP(0x2E); BAD_TRAP(0x2F)
	
	BAD_TRAP(0x30); BAD_TRAP(0x31); BAD_TRAP(0x32); BAD_TRAP(0x33)
	BAD_TRAP(0x34); BAD_TRAP(0x35); BAD_TRAP(0x36); BAD_TRAP(0x37)
	BAD_TRAP(0x38); BAD_TRAP(0x39); BAD_TRAP(0x3A); BAD_TRAP(0x3B)
	BAD_TRAP(0x3C); BAD_TRAP(0x3D); BAD_TRAP(0x3E); BAD_TRAP(0x3F)

	BAD_TRAP(0x40); BAD_TRAP(0x41); BAD_TRAP(0x42); BAD_TRAP(0x43)
	BAD_TRAP(0x44); BAD_TRAP(0x45); BAD_TRAP(0x46); BAD_TRAP(0x47)
	BAD_TRAP(0x48); BAD_TRAP(0x49); BAD_TRAP(0x4A); BAD_TRAP(0x4B)
	BAD_TRAP(0x4C); BAD_TRAP(0x4D); BAD_TRAP(0x4E); BAD_TRAP(0x4F)

	BAD_TRAP(0x50); BAD_TRAP(0x51); BAD_TRAP(0x52); BAD_TRAP(0x53)
	BAD_TRAP(0x54); BAD_TRAP(0x55); BAD_TRAP(0x56); BAD_TRAP(0x57)
	BAD_TRAP(0x58); BAD_TRAP(0x59); BAD_TRAP(0x5A); BAD_TRAP(0x5B)
	BAD_TRAP(0x5C); BAD_TRAP(0x5D); BAD_TRAP(0x5E); BAD_TRAP(0x5F)

	BAD_TRAP(0x60); BAD_TRAP(0x61); BAD_TRAP(0x62); BAD_TRAP(0x63)
	BAD_TRAP(0x64); BAD_TRAP(0x65); BAD_TRAP(0x66); BAD_TRAP(0x67)
	BAD_TRAP(0x68); BAD_TRAP(0x69); BAD_TRAP(0x6A); BAD_TRAP(0x6B)
	BAD_TRAP(0x6C); BAD_TRAP(0x6D); BAD_TRAP(0x6E); BAD_TRAP(0x6F)

	BAD_TRAP(0x70); BAD_TRAP(0x71); BAD_TRAP(0x72); BAD_TRAP(0x73)
	BAD_TRAP(0x74); BAD_TRAP(0x75); BAD_TRAP(0x76); BAD_TRAP(0x77)
	BAD_TRAP(0x78); BAD_TRAP(0x79); BAD_TRAP(0x7A); BAD_TRAP(0x7B)
	BAD_TRAP(0x7C); BAD_TRAP(0x7D); BAD_TRAP(0x7E); BAD_TRAP(0x7F)
	
! + 0x80-0xFF: trap_instruction 
	SOFT_TRAP(0x80); SOFT_TRAP(0x81); SOFT_TRAP(0x82); SOFT_TRAP(0x83)
	SOFT_TRAP(0x84); SOFT_TRAP(0x85); SOFT_TRAP(0x86); SOFT_TRAP(0x87)
	SOFT_TRAP(0x88); SOFT_TRAP(0x89); SOFT_TRAP(0x8A); SOFT_TRAP(0x8B)
	SOFT_TRAP(0x8C); SOFT_TRAP(0x8D); SOFT_TRAP(0x8E); SOFT_TRAP(0x8F)

	SOFT_TRAP(0x90); SOFT_TRAP(0x91); SOFT_TRAP(0x92); SOFT_TRAP(0x93)
	SOFT_TRAP(0x94); SOFT_TRAP(0x95); SOFT_TRAP(0x96); SOFT_TRAP(0x97)
	SOFT_TRAP(0x98); SOFT_TRAP(0x99); SOFT_TRAP(0x9A); SOFT_TRAP(0x9B)
	SOFT_TRAP(0x9C); SOFT_TRAP(0x9D); SOFT_TRAP(0x9E); SOFT_TRAP(0x9F)

	SOFT_TRAP(0xA0); SOFT_TRAP(0xA1); SOFT_TRAP(0xA2); SOFT_TRAP(0xA3)
	SOFT_TRAP(0xA4); SOFT_TRAP(0xA5); SOFT_TRAP(0xA6); SOFT_TRAP(0xA7)
	SOFT_TRAP(0xA8); SOFT_TRAP(0xA9); SOFT_TRAP(0xAA); SOFT_TRAP(0xAB)
	SOFT_TRAP(0xAC); SOFT_TRAP(0xAD); SOFT_TRAP(0xAE); SOFT_TRAP(0xAF)

	SOFT_TRAP(0xB0); SOFT_TRAP(0xB1); SOFT_TRAP(0xB2); SOFT_TRAP(0xB3)
	SOFT_TRAP(0xB4); SOFT_TRAP(0xB5); SOFT_TRAP(0xB6); SOFT_TRAP(0xB7)
	SOFT_TRAP(0xB8); SOFT_TRAP(0xB9); SOFT_TRAP(0xBA); SOFT_TRAP(0xBB)
	SOFT_TRAP(0xBC); SOFT_TRAP(0xBD); SOFT_TRAP(0xBE); SOFT_TRAP(0xBF)

	SOFT_TRAP(0xC0); SOFT_TRAP(0xC1); SOFT_TRAP(0xC2); SOFT_TRAP(0xC3)
	SOFT_TRAP(0xC4); SOFT_TRAP(0xC5); SOFT_TRAP(0xC6); SOFT_TRAP(0xC7)
	SOFT_TRAP(0xC8); SOFT_TRAP(0xC9); SOFT_TRAP(0xCA); SOFT_TRAP(0xCB)
	SOFT_TRAP(0xCC); SOFT_TRAP(0xCD); SOFT_TRAP(0xCE); SOFT_TRAP(0xCF)

	SOFT_TRAP(0xD0); SOFT_TRAP(0xD1); SOFT_TRAP(0xD2); SOFT_TRAP(0xD3)
	SOFT_TRAP(0xD4); SOFT_TRAP(0xD5); SOFT_TRAP(0xD6); SOFT_TRAP(0xD7)
	SOFT_TRAP(0xD8); SOFT_TRAP(0xD9); SOFT_TRAP(0xDA); SOFT_TRAP(0xDB)
	SOFT_TRAP(0xDC); SOFT_TRAP(0xDD); SOFT_TRAP(0xDE); SOFT_TRAP(0xDF)

	BUILD_EXTIRQ(0); BUILD_EXTIRQ(1); BUILD_EXTIRQ(2); BUILD_EXTIRQ(3)
	BUILD_EXTIRQ(4); BUILD_EXTIRQ(5); BUILD_EXTIRQ(6); BUILD_EXTIRQ(7)
	BUILD_EXTIRQ(8); BUILD_EXTIRQ(9); BUILD_EXTIRQ(10); BUILD_EXTIRQ(11)
	BUILD_EXTIRQ(12); BUILD_EXTIRQ(13); BUILD_EXTIRQ(14); BUILD_EXTIRQ(15)
	BUILD_EXTIRQ(16); BUILD_EXTIRQ(17); BUILD_EXTIRQ(18); BUILD_EXTIRQ(19)
	BUILD_EXTIRQ(20); BUILD_EXTIRQ(21); BUILD_EXTIRQ(22); BUILD_EXTIRQ(23)
	BUILD_EXTIRQ(24); BUILD_EXTIRQ(25); BUILD_EXTIRQ(26); BUILD_EXTIRQ(27)
	BUILD_EXTIRQ(28); BUILD_EXTIRQ(29); BUILD_EXTIRQ(30); BUILD_EXTIRQ(31)
	
.previous
