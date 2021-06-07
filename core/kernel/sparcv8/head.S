/*
 * $FILE: head.S
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <linkage.h>
#include <arch/asm_offsets.h>
#include <arch/irqs.h>
#include <arch/processor.h>
#include <arch/xm_def.h>
#include <arch/leon.h>
	
#define WIM_INIT  2

.macro DO_FLUSH_CACHE _r0, _r1
        FLUSH_ICACHE(\_r0\(), \_r1\())
        FLUSH_DCACHE(\_r0\(), \_r1\())

.endm
	
.macro WR_DELAY
        nop
        nop
        nop
.endm

.section .boot.text, "ax"
ASM_ALIGN
ENTRY(start)
ENTRY(_start)
#if defined(CONFIG_MMU)
	/* Disable paging */
	set LEON_MMU_CTRL_REG, %g1
	lda [%g1] LEON_MMU_ASI, %g2
	andn %g2, 0x1, %g2
	sta %g2, [%g1] LEON_MMU_ASI
#endif

#ifdef CONFIG_LEON4
        set LEON_L2CACHE_BASE, %g1
        set 0x80000000, %g2
#ifdef CONFIG_COPY_BACK
        or %g2,LEON_L2CACHE_COPY_BACK,%g2 
#elif CONFIG_WRITE_THROUGH
        or %g2,LEON_L2CACHE_WRITE_THROUGH,%g2 
#endif
        sta %g2, [%g1] LEON_MMU_BYPASS
#endif        
        /* Enabling Cache and Flushing it */
	set CCR_DCS_MASK|CCR_ICS_MASK, %g1
#ifdef CONFIG_CACHE_SNOOP
        set CCR_DS_BIT, %g2
        or %g1, %g2, %g1
#endif
#ifdef CONFIG_CACHE_IBURST_FETCH        
        set CCR_IB_BIT, %g2
        or %g1, %g2, %g1
#endif
	flush
#ifdef CONFIG_LEON2
	set LEON_CCR_BASE, %g2
	st %g1, [%g2]
#endif        
#if defined(CONFIG_LEON3) || defined(CONFIG_LEON4) || defined(CONFIG_LEON3FT)
	sta %g1, [%g0] LEON_CCR_BASE
#endif

        /* MEMORY CONTROLLER CONFIGURATION MISS -- MIG */
#if defined(CONFIG_LEON2)||defined(CONFIG_LEON3)||defined(CONFIG_LEON3FT)
	/* Set up the memory */
	set LEON_MEMORY_CFG_BASE, %g1
	set IOP_MASK, %g3
	ld [%g1], %g2
	or %g2, %g3, %g2
	st %g2, [%g1]
#endif

#ifndef CONFIG_MMU
	set LEON_MEMORY_WPR_BASE, %g1
	st %g0, [%g1]
	add %g1, 4, %g1
	st %g0, [%g1]
#endif

#ifdef CONFIG_SMP_SUPPORT       
        rd %asr17, %g1
        sra %g1, 0x1c, %g1
        cmp %g1, 0
        be 11f
        nop
        b 2f
        nop
11:
#endif
        
	set _VIRT2PHYS(_sbss), %o0  
	set _VIRT2PHYS(_ebss), %o1
1:	stb %g0, [%o0]
	add %o0, 0x1, %o0
	subcc %o0, %o1, %g0
	bl 1b
	nop

	/* Copy the cdata section to the data section */
	set _VIRT2PHYS(_sdata), %o0
	set _VIRT2PHYS(_edata), %o1
	set _VIRT2PHYS(_scdata), %o2
	set _VIRT2PHYS(_cpdata), %o3

	/* Check if data has already been backed up */
	ld [%o3], %o4
	cmp %o4, %g0
	bne 1f
	mov 1, %o4
	
	st %o4, [%o3]

3:	ldub [%o0], %o3
	stb %o3, [%o2]
	add %o0, 0x1, %o0
	subcc %o0, %o1, %g0
	bl 3b
	add %o2, 0x1, %o2
	
	b 2f
	nop
	
1:	ldub [%o2], %o3
	stb %o3, [%o0]
	add %o0, 0x1, %o0
	subcc %o0, %o1, %g0
	bl 1b
	add %o2, 0x1, %o2

2:        
	/* Clear general registers */
	clr %g1
        clr %g2
        clr %g3
        clr %g4
        clr %g5
        clr %g6
        clr %g7
	
	/* Set up window registers */
	mov %g0, %wim
	WR_DELAY
	
	mov CONFIG_REGISTER_WINDOWS, %g1
	save
1:
        clr %i0
        clr %i1
        clr %i2
        clr %i3
        clr %i4
        clr %i5
        clr %i6
        clr %i7
        clr %l0
        clr %l1
        clr %l2
        clr %l3
        clr %l4
        clr %l5
        clr %l6
        clr %l7
        subcc %g1, 1, %g1
        bnz,a   1b
        save
	
	/* Set up PSR, WIM, TBR and Y registers  */	
	mov WIM_INIT, %wim
        set (PSR_PIL_MASK|PSR_ET_BIT|PSR_S_BIT|PSR_EF_BIT), %g1
        wr %g1, %psr
	WR_DELAY
	mov %g0, %y
	
#ifdef CONFIG_MMU
#ifdef CONFIG_SMP_SUPPORT        
        rd %asr17, %g1
        sra %g1, 0x1c, %g1
        cmp %g1, 0
        bne 113f
        nop
#endif
        
        set ((CONFIG_XM_LOAD_ADDR&~((16*1024*1024)-1))>>4)|_PG_ARCH_RW|_PG_ARCH_CACHE|_PG_ARCH_PTE_PRESENT, %o0
        set _VIRT2PHYS(_ptdL1), %o1
        set (CONFIG_XM_LOAD_ADDR>>24)<<2, %o2
        add %o1, %o2, %o1
        st %o0, [%o1]

        set ((CONFIG_XM_LOAD_ADDR&~((16*1024*1024)-1))>>4)|_PG_ARCH_RW|_PG_ARCH_CACHE|_PG_ARCH_PTE_PRESENT, %o0
        set _VIRT2PHYS(_ptdL1), %o1
        set (CONFIG_XM_OFFSET>>24)<<2, %o2
        add %o1, %o2, %o1
        st %o0, [%o1]
        
	set _VIRT2PHYS(_ptdL1), %o0
	srl %o0, 4, %o0
	or %o0, _PG_ARCH_PTD_PRESENT, %o0
        
	set _VIRT2PHYS(contextTab), %o1	
	st %o0, [%o1]
113:  
	/* Load MMU CTXT PTR <- (contextTab>>4) */
	set _VIRT2PHYS(contextTab), %o1
	srl %o1, 4, %o1
	
	set LEON_MMU_CPR_REG, %g1
	sta %o1, [%g1] LEON_MMU_ASI

	/* Load MMU CTXT <- 0 */
	set LEON_MMU_CTXT_REG, %g1
	sta %g0, [%g1] LEON_MMU_ASI

	/* Enable MMU CTRL_REG <- 1 */
	set LEON_MMU_CTRL_REG, %g1
	lda [%g1] LEON_MMU_ASI, %g2
	or %g2, 0x1, %g2
	sta %g2, [%g1] LEON_MMU_ASI        
#endif
	
	sethi %hi(StartXM), %g1
	jmpl %g1 + %lo(StartXM), %g0
	nop

__HaltSystem2:
#if defined(CONFIG_MMU)
	/* Disable paging */
	set LEON_MMU_CTRL_REG, %g1
	lda [%g1] LEON_MMU_ASI, %g2
	andn %g2, 0x1, %g2
	sta %g2, [%g1] LEON_MMU_ASI

	set LEON_MMU_CPR_REG, %g1
	sta %g0, [%g1] LEON_MMU_ASI
#endif
1:     
#if defined(CONFIG_ENABLE_POWERDOWN)
#if defined(CONFIG_GR_712_RC)
        mov %g0, %asr19
        set 0xFFFFFFF0, %g1
        lda [%g1] 0x1C, %g0
#else
    wr %g0, %asr19 //PowerDown
#endif
#endif
        b 1b
	nop
        
_Reset2:
#if defined(CONFIG_MMU)
	/* Disable paging */
	set LEON_MMU_CTRL_REG, %g1
	lda [%g1] LEON_MMU_ASI, %g2
	andn %g2, 0x1, %g2
	sta %g2, [%g1] LEON_MMU_ASI

	set LEON_MMU_CPR_REG, %g1
	sta %g0, [%g1] LEON_MMU_ASI
#endif
#ifdef CONFIG_SMP_SUPPORT        
    rd %asr17, %g1
	sra %g1, 0x1c, %g1
	cmp %g1, 0
	be 2f
	nop

	set _VIRT2PHYS(resetMask), %g2
	sll %g1, 1, %g4     

1:
	ld [%g2], %g3       
	and %g4, %g3, %g3   
	cmp %g3, 0
	be 1b
	nop

	set start, %g1
	jmp %g1
	nop
2:
#endif
	jmp %o0
	nop
        
.text

ENTRY(StartXM)
	set trapTable, %g1
	wr %g1, %tbr
        
	/* Setting up a kernel stack */
#ifdef CONFIG_SMP_SUPPORT
        rd %asr17, %o0
        sra %o0, 0x1c, %o0
#else
        mov %g0, %o0
#endif       

        set CONFIG_KSTACK_SIZE, %g3
        add %o0, 1, %g1

        set __idle_kthread, %fp
1:      dec %g1
        cmp %g1, 0
        bg 1b        
        add %fp, %g3, %fp

        sub %fp, %g3, %o1
        
#ifdef CONFIG_SMP_SUPPORT        
   	rd %asr17, %g1
        sra %g1, 0x1c, %g1
        cmp %g1, 0
        bne 111f
        nop
#endif
	/* calling our first C function */
	call Setup
        sub %fp, MIN_STACK_FRAME, %sp
        
#ifdef CONFIG_SMP_SUPPORT
111:
        call InitSecondaryCpu
	sub %fp, MIN_STACK_FRAME, %sp
#endif
        
ENTRY(__HaltSystem)
#ifdef CONFIG_MMU
        set ((CONFIG_XM_LOAD_ADDR&~((16*1024*1024)-1))>>4)|_PG_ARCH_RW|_PG_ARCH_CACHE|_PG_ARCH_PTE_PRESENT, %l0
        set _ptdL1, %l1
        set (CONFIG_XM_LOAD_ADDR>>24)<<2, %l2
        add %l1, %l2, %l1
        st %l0, [%l1]
        
	/* Load MMU CTXT <- 0 */
	set LEON_MMU_CTXT_REG, %g1
	sta %g0, [%g1] LEON_MMU_ASI

        DO_FLUSH_CACHE  %l0, %l1
        set FLUSH_ENTIRE, %l3
        sta %g0, [%l3] LEON_MMU_FLUSH
#endif

        set __HaltSystem2, %g1
        jmp %g1
        nop
        
#if defined(CONFIG_ENABLE_POWERDOWN)
#if defined(CONFIG_GR_712_RC)
        mov %g0, %asr19
        set 0xFFFFFFF0, %g1
        lda [%g1] 0x1C, %g0
#else
    wr %g0, %asr19 //PowerDown
#endif
#endif
1:      b 1b
	nop


ENTRY(ArchStartupGuest)
	ldd [%sp], %o0
	jmpl %g4, %g0
	add %sp, 8, %sp
	
ENTRY(_Reset)
#ifdef CONFIG_SMP_SUPPORT
	call ClearResetMask
	nop
#endif
#ifdef CONFIG_MMU
        set ((CONFIG_XM_LOAD_ADDR&~((16*1024*1024)-1))>>4)|_PG_ARCH_RW|_PG_ARCH_CACHE|_PG_ARCH_PTE_PRESENT, %l0
        set _ptdL1, %l1
        set (CONFIG_XM_LOAD_ADDR>>24)<<2, %l2
        add %l1, %l2, %l1
        st %l0, [%l1]
        
	/* Load MMU CTXT <- 0 */
	set LEON_MMU_CTXT_REG, %g1
	sta %g0, [%g1] LEON_MMU_ASI

        DO_FLUSH_CACHE  %l0, %l1
        set FLUSH_ENTIRE, %l3
        sta %g0, [%l3] LEON_MMU_FLUSH
#endif
	set _Reset2, %g1
	jmp %g1
	nop
	
.section ".bss"

#ifdef CONFIG_MMU
.align 1024

ENTRY(contextTab)
	.zero 1024

.align 1024
ENTRY(_pgTables)
ENTRY(_ptdL1)
	.zero 1024        
#endif
ASM_ALIGN
__idle_kthread:
        .zero CONFIG_KSTACK_SIZE*CONFIG_NO_CPUS
ASM_ALIGN
__idle_kthread_top:

.previous
