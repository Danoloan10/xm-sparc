/*
 * $FILE: start.S
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

/*
 Changelog:
 - [02/10/2015:SPR-300915-02] build_info removed
*/
        
#include <comp.h>
#include <linkage.h>
#include <xmef.h>
#include <hypercalls.h>
#include <arch/asm_offsets.h>
#include <arch/leon.h>

/*
	Register state when starting an interrupt:
	%i6 <- FP (Old stack)
	%l0 <- PSR (BUILD_IRQ)
	%l1 <- PC
	%l2 <- nPC
	%l3 <- irqnr (BUILD_IRQ)
*/

.macro BUILD_IRQ irqNr
	BUILD_COMMON_TRAP \irqNr\(), DoIrq
.endm

.macro BUILD_EXCEPTION excNr
	BUILD_COMMON_TRAP \excNr\(), DoTrap
.endm

.macro BUILD_HYPERCALL hpNr
	BUILD_COMMON_TRAP \hpNr\(), DoHypercall
.endm
	
.macro BUILD_ASM_HYPERCALL syscNr
	BUILD_TRAP \syscNr\(), AsmHypercallHandler
.endm

.macro BUILD_TRAP trapNr, handler
	rd %psr, %l0
	sethi %hi(\handler\()), %l4
	jmpl %l4 + %lo(\handler\()), %g0
	mov \trapNr\(), %l3
.endm

.macro BUILD_COMMON_TRAP trapNr, handler
	sethi %hi(\handler\()), %l4
	or %l4, %lo(\handler\()), %l4
	b CommonTrapHandler
	mov \trapNr\(), %l3
.endm

// SOFT_TRAP are dispatched to the partition where they were triggered
.macro SOFT_TRAP trapNr
	rd %psr, %l0
	sethi %hi(EmulateTrap), %l4
	jmpl %l4 + %lo(EmulateTrap), %g0
	mov \trapNr\(), %l3
.endm

.macro BUILD_SOFT_TRAP_BLOCK stTrapNr endTrapNr
vector=\stTrapNr\()
.rept \endTrapNr\()-\stTrapNr\()+1
	/*\noTraps\() */
	SOFT_TRAP vector
vector=vector+1
.endr
.endm
	
.global .Tbegin_hp, .Tbegin_ahp
.section .irqtab, "ax"

ASM_ALIGN
/* Trap table */
ENTRY(trapTable)
        
! + 0x00: reset
t_reset:
	sethi %hi(start), %g1
	jmpl %g1 + %lo(start), %g0
	nop
	nop

! + 0x01: instruction_access_exception
	BUILD_EXCEPTION 4
	
! + 0x02: illegal_instruction 
	BUILD_EXCEPTION 6
	
! + 0x03: privileged_instruction 
	BUILD_EXCEPTION 5
	
! + 0x04: fp_disabled 
	BUILD_TRAP 7, FpDisabledTrap
	
! + 0x05: window_overflow
	rd %psr, %l0
	sethi %hi(WindowOverflowTrap), %l4
	jmpl %l4 + %lo(WindowOverflowTrap), %g0
	nop
	
! + 0x06: window_underflow
	rd %psr, %l0
	sethi %hi(WindowUnderflowTrap), %l4
	jmpl %l4 + %lo(WindowUnderflowTrap), %g0
	nop
	
! + 0x07: mem_address_not_aligned 
	BUILD_EXCEPTION 11
	
! + 0x08: fp_exception 
	BUILD_EXCEPTION 12
	
! + 0x09: data_access_exception 
	BUILD_EXCEPTION 16
	
! + 0x0A: tag overflow 
	BUILD_EXCEPTION 17
	
! + 0x0B: watchpoint_detected 
	BUILD_EXCEPTION 10

	BUILD_SOFT_TRAP_BLOCK 0xC 0x10

! - 0x11: internal bus error ! if LEON2 || LEON3
	BUILD_IRQ 1
	
! - 0x12: UART2 ! if LEON2 || LEON3
	BUILD_IRQ 2
	
! - 0x13: UART1 ! if LEON2 || LEON3
	BUILD_IRQ 3
	
! - 0x14: I/O interrupt[0] ! if LEON2 || LEON3
	BUILD_IRQ 4
	
! - 0x15: I/O interrupt[1] ! if LEON2 || LEON3
	BUILD_IRQ 5
	
! - 0x16: I/O interrupt[2] ! if LEON2 || LEON3
	BUILD_IRQ 6
	
! - 0x17: I/O interrupt[3] ! if LEON2 || LEON3
	BUILD_IRQ 7
	
! - 0x18: timer 1 ! if LEON2 || LEON3
	BUILD_IRQ 8
	
! - 0x19: timer 2 ! if LEON2 || LEON3
	BUILD_IRQ 9
	
! - 0x1A: unused ! if LEON2 || LEON3
	BUILD_IRQ 10
	
! - 0x1B: DSU trace buffer ! if LEON2 || LEON3
	BUILD_IRQ 11
	
! - 0x1C: unused ! if LEON2 || LEON3
	BUILD_IRQ 12
	
! - 0x1D: unused ! if LEON2 || LEON3
	BUILD_IRQ 13
	
! - 0x1E: PCI ! if LEON2 || LEON3
	BUILD_IRQ 14
	
! - 0x1F: unused ! if LEON2 || LEON3
	BUILD_IRQ 15
	
! + 0x20: register_hardware_error 
	BUILD_EXCEPTION 3
	
	BUILD_SOFT_TRAP_BLOCK 0x21 0x23
	
! + 0x24: cp_disabled 
	BUILD_EXCEPTION 8
	
	BUILD_SOFT_TRAP_BLOCK 0x25 0x29
	
! + 0x2A: divide_exception 
	BUILD_EXCEPTION 18
	
! + 0x2B: write error 
	BUILD_EXCEPTION 0
	
! + 0x2C: data_access_MMU_miss
	BUILD_EXCEPTION 15
	
	BUILD_SOFT_TRAP_BLOCK 0x2D 0x3B

! + 0x3C: instruction_access_MMU_miss
	BUILD_EXCEPTION 1
	BUILD_SOFT_TRAP_BLOCK 0x3D 0x7F

! + 0x80-0xFF: trap_instruction
	BUILD_SOFT_TRAP_BLOCK 0x80 0xEF
	
.Tbegin_hp:
	BUILD_HYPERCALL 0xF0

.Tbegin_ahp:
	BUILD_ASM_HYPERCALL 0xF1
	
	BUILD_SOFT_TRAP_BLOCK 0xF2 0xFF
