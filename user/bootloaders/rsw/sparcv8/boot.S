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
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

/*
 Changelog:
 - [30/09/2015:SPR-300915-01] LEON_MEMORY_CFG_BASE is configured according to
        mass memory configuration parameters.
 - [14/10/2015:SPR-141015-02] MCFG* is not longer configured by the RSW (SPR-300915-01)
*/
        
#include <xm_inc/sparcv8/leon.h>
	
#define MIN_STACK_FRAME 0x60

#define LEON_IOMMU_CTRLREG 0x10
        
#ifdef CONFIG_CPU_ITAR_FREE
#define VALUE_MCFG1  0x14080233
#define VALUE_MCFG2  0x00001caf
#define VALUE_MCFG3  0x00000000
#define VALUE_CCR    0x00000000
#endif

#ifdef CONFIG_SPW_RTC
#define LEON_GRPULSE_BASE  (LEON_IOREGS+0x20000)
#define LEON_ITC1_BASE     (LEON_IOREGS+0xA0)

#define VALUE_CCR       0x50910003
#define VALUE_MCFG1     0x105800ff
#define VALUE_MCFG2     0x00001060
#define VALUE_MCFG3     0x00000000
#define VALUE_ITC1      0x000000ff
#define VALUE_GPIO_OUT  0xfffffdb6
#define VALUE_GPIO_DIR  0x000003ff
#define VALUE_ZERO      0x00000000
#endif
	
.text
.align 4

.global start, _start, _halt_system

_start:	
start:
#ifdef CONFIG_SMP_SUPPORT
        rd %asr17, %g1
        sra %g1, 0x1c, %g1
        cmp %g1, 0
        be 1f
        nop
// If DSU enabled, the start address for all the processors is set up
// to the ELF-one.
// This work-around jumps to XM directly.
        set hpvEntryPoint, %g1
        ld [%g1], %g1
        jmp %g1
        nop
1:
#endif
#ifdef CONFIG_LEON4
        set LEON_L2CACHE_BASE, %g1
        set 0x80000000, %g2
        sta %g2, [%g1] LEON_MMU_BYPASS
#endif
        rd %psr, %g1
        andn %g1, 0x1f, %g1
        wr %g1, 0x20, %psr
        nop
        nop
        nop
#ifdef CONFIG_MMU
        /* Disable paging */
        set LEON_MMU_CTRL_REG, %g1
        lda [%g1] LEON_MMU_ASI, %g2
        andn %g2, 0x1, %g2
        sta %g2, [%g1] LEON_MMU_ASI
#endif

#ifdef CONFIG_LEON4
       /*Disable IOMMU Device*/
       set LEON_IOMMU_CTRLREG, %g1
       set 0, %g2
       st %g2, [%g1] 
#endif
        
	/* Set up Memory Configuration Registers */
#ifndef CONFIG_MMU
        set LEON_MEMORY_WPR_BASE+LEON_WPR1_OFFSET, %g1
        st %g0, [%g1]
        set LEON_MEMORY_WPR_BASE+LEON_WPR2_OFFSET, %g1
        st %g0, [%g1]
#endif
#ifdef CONFIG_CPU_ITAR_FREE
        /* Set up Memory Configuration Registers */
        set LEON_MEMORY_CFG_BASE, %g1
        set VALUE_MCFG1,%o0
        st %o0, [%g1]
        add %g1, 4, %g1      /*MCFG2*/
        set VALUE_MCFG2,%o0
        st %o0, [%g1]
        add %g1, 4, %g1      /*MCFG3*/
        set VALUE_MCFG3,%o0
        st %o0, [%g1]
        set LEON_MEMORY_CCR_BASE, %g1
        set VALUE_CCR,%o0
        st %o0, [%g1]
#endif
        
#ifdef CONFIG_SPW_RTC
       // Set up Memory Configuration Registers
        set LEON_MEMORY_CFG_BASE, %g1
        set VALUE_MCFG1,%o0
        st %o0, [%g1]
        add %g1, 4, %g1      //MCFG2
        set VALUE_MCFG2,%o0
        st %o0, [%g1]
        add %g1, 4, %g1      //MCFG3
        set VALUE_MCFG3,%o0
        st %o0, [%g1]
        set LEON_CCR_BASE, %g1
        set VALUE_CCR,%o0
        st %o0, [%g1]
        set LEON_ITC1_BASE, %g1
        set VALUE_ITC1,%o0
        st %o0, [%g1]

#endif

	/* Zero out our BSS section. */
        set _sbss, %o0
        set _ebss, %o1
        add %o0, 0x1, %o0
1:      stb %g0, [%o0]
        subcc %o0, %o1, %g0
        bl 1b
        add %o0, 0x1, %o0
	
	/* copying our BSS section. */
        set _sdata, %o0
        set _edata, %o1
	set _sdata_lma, %o2
	
1:	ldub [%o2], %g1
	stb %g1, [%o0]
	
	add %o2, 0x1, %o2
        subcc %o0, %o1, %g0
        bl 1b
        add %o0, 0x1, %o0
	
	set __stack_top, %fp

	set 0x2, %g1
	wr %g1, %wim
        
	set trapTable, %g1
	wr %g1, %tbr

	call RSwMain
	sub %fp, MIN_STACK_FRAME, %sp

_halt_system:
	b _halt_system
	nop

window_overflow_trap:
	/* rotate WIM on bit right, we have 8 windows */
        mov %wim,%l3
        sll %l3,7,%l4
        srl %l3,1,%l3
        or  %l3,%l4,%l3
        and %l3,0xff,%l3

        /* disable WIM traps */
        mov %g0,%wim
        nop; nop; nop

        /* point to correct window */
        save

        /* dump registers to stack */
        std %l0, [%sp +  0]
        std %l2, [%sp +  8]
        std %l4, [%sp + 16]
        std %l6, [%sp + 24]
        std %i0, [%sp + 32]
        std %i2, [%sp + 40]
        std %i4, [%sp + 48]
        std %i6, [%sp + 56]

        /* back to where we should be */
        restore

        /* set new value of window */
	mov %l0, %psr
        mov %l3,%wim
        nop; nop; nop

        /* go home */
        jmp %l1
        rett %l2

	
window_underflow_trap:
	/* rotate WIM on bit LEFT, we have 8 windows */ 
        mov %wim,%l3
        srl %l3, 7,%l4
        sll %l3, 1,%l3
        or  %l3,%l4,%l3
        and %l3,0xff,%l3

        /* disable WIM traps */
        mov %g0,%wim
        nop; nop; nop

        /* point to correct window */
        restore
        restore

        /* dump registers to stack */
        ldd [%sp +  0], %l0
        ldd [%sp +  8], %l2
        ldd [%sp + 16], %l4
        ldd [%sp + 24], %l6
        ldd [%sp + 32], %i0
        ldd [%sp + 40], %i2
        ldd [%sp + 48], %i4
        ldd [%sp + 56], %i6

        /* back to where we should be */
        save
        save

        /* set new value of window */
	mov %l0, %psr
        mov %l3,%wim
        nop; nop; nop

        /* go home */
        jmp %l1
        rett %l2

CommonTrapHandler:	
	rd %psr, %g1
	or %g1, (0x20|0xf00), %g1
	wr %g1, %psr
	nop
	nop
	nop
	
	sub %fp, MIN_STACK_FRAME, %sp 
	b DoTrap
	nop
	
.section .traptab, "ax"
	
#define BUILD_TRAP(trapNr) \
	rd %psr, %l0 	;\
	mov trapNr, %o0 	;\
	b CommonTrapHandler	;\
	nop
/*
	sub %fp, MIN_STACK_FRAME, %sp  ;\
	b DoTrap	  ;\
	mov trapNr, %o0	  ;\
	nop		  ;\
*/
	
trapTable:
	
! + 0x00: reset
t_reset: b start
	nop
	nop
	nop

! + 0x01: instruction_access_exception 
	BUILD_TRAP(1)
	
! + 0x02: illegal_instruction 
	BUILD_TRAP(2)
	
! + 0x03: privileged_instruction 
	BUILD_TRAP(3)
	
! + 0x04: fp_disabled
	BUILD_TRAP(4)
	
! + 0x05: window_overflow
        mov %psr, %l0
	sethi %hi(window_overflow_trap), %l4
	jmp %l4+%lo(window_overflow_trap)
	nop
	
! + 0x06: window_underflow
	mov %psr, %l0
	sethi %hi(window_underflow_trap), %l4
	jmp %l4+%lo(window_underflow_trap)
	nop
	
! + 0x07: mem_address_not_aligned 
	BUILD_TRAP(7)
	
! + 0x08: fp_exception 
	BUILD_TRAP(8)
	
! + 0x09: data_access_exception 
	BUILD_TRAP(9)
	
! + 0x0A: tag overflow 
	BUILD_TRAP(10)
	
! + 0x0B: watchpoint_detected 
	BUILD_TRAP(11)

	BUILD_TRAP(12)
	BUILD_TRAP(13)
	BUILD_TRAP(14)
	BUILD_TRAP(15)
	BUILD_TRAP(16)
	
! - 0x11: internal bus error 
	BUILD_TRAP(17)
	
! - 0x12: UART2 
	BUILD_TRAP(18)
	
! - 0x13: UART1 
	BUILD_TRAP(19)
	
! - 0x14: I/O interrupt[0] 
	BUILD_TRAP(20)
	
! - 0x15: I/O interrupt[1] 
	BUILD_TRAP(21)
	
! - 0x16: I/O interrupt[2] 
	BUILD_TRAP(22)
	
! - 0x17: I/O interrupt[3] 
	BUILD_TRAP(23)
	
! - 0x18: timer 1
	BUILD_TRAP(24)
	
! - 0x19: timer 2 
	BUILD_TRAP(25)
	
! - 0x1A: unused 
	BUILD_TRAP(26)
	
! - 0x1B: DSU trace buffer 	
	BUILD_TRAP(27)
	
! - 0x1C: unused 
	BUILD_TRAP(28)
	
! - 0x1D: unused 
	BUILD_TRAP(29)
	
! - 0x1E: PCI 
	BUILD_TRAP(30)
	
! - 0x1F: unused 
	BUILD_TRAP(31)

! + 0x20: register_hardware_error 
	BUILD_TRAP(32)
	
	BUILD_TRAP(33)
	BUILD_TRAP(34)
	BUILD_TRAP(35)
	
! + 0x24: cp_disabled 
	BUILD_TRAP(36)

	BUILD_TRAP(37)
	BUILD_TRAP(38)
	BUILD_TRAP(39)
	BUILD_TRAP(40)
	BUILD_TRAP(41)
	
! + 0x2A: divide_exception 
	BUILD_TRAP(42)
	
! + 0x2B: write error 
	BUILD_TRAP(43)

.section ".bss"
.align 8

__stack:
	.zero (CONFIG_STACK_SIZE*1024)
__stack_top:	
	
.previous
