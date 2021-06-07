/*
 * $FILE: entry.S
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 * Modified: Javier O. Coronel <jacopa@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 *
 */

/*
   Changelog:
   - [03/09/2015:SPR-060815-01] FpDisableTrap and SparcSetPsrSys updated
   [04/07/2017] Ángel Esquinas
   - CP-170704-02 : Implement workaround for the "LEON3FT stale cache entry 
                    after store with a data tag parity error" (GRLIB-TN-0009)

*/
        
#include <linkage.h>
#include <hypercalls.h>
#include <arch/asm_offsets.h>
#include <arch/irqs.h>
#include <arch/processor.h>
#include <guest.h>

.global .Tbegin_trap, .Tend_trap, .Tend_ahp, .Tbegin_kpreempt, .Tend_kpreempt, .Tend_etrap
	
/*
  The SPARC manual denfines that after writing a control
  register, it is implementation defined the time that the operation
  is performed (it can spend up to three instruction slots), so to prevent
  error we have added a WR_DELAY after each difficult situation
*/

.macro WR_DELAY
	nop
	nop
	nop
.endm

.macro MOD_INTEGER2BITMAP _dst, _src, _tmp, _mod
	and \_src\(), (\_mod\())-1, \_tmp\()
	mov 1, \_dst\()
	sll \_dst\(), \_tmp\(), \_dst\()
.endm

.macro MOD_SHIFT2LEFT _dst, _tmp, _mod
	sll \_dst\(), 1, \_tmp\()
	srl \_dst\(), (\_mod\())-1, \_dst\()
	or \_dst\(), \_tmp\(), \_dst\()
	and \_dst\(), (1<<(\_mod\()))-1, \_dst\()
.endm

.macro MOD_SHIFT2RIGHT _dst, _tmp, _mod
	sll \_dst\(), (\_mod\())-1, \_tmp\()
	srl \_dst\(), 1, \_dst\()
	or \_dst\(), \_tmp\(), \_dst\()
	and \_dst\(), (1<<(\_mod\()))-1, \_dst\()
.endm

/* <track id="test-PSR-Returing-From-Interrupt"> */
.macro RESTORE_PSR _r0
	wr \_r0\(), %psr
	WR_DELAY
.endm
/* </track id="test-PSR-Returing-From-Interrupt"> */

.macro SET_WP_BIT _r0, _r1, _r2
#if defined(CONFIG_MPU)
#if defined CONFIG_AT697F
    set LEON_MEMORY_WPR_BASE+LEON_WPR2_OFFSET, \_r0\()
    set (1<<31), \_r2\()
    ld [\_r0\()], \_r1\()
    or \_r1\(), \_r2\(), \_r1\()
    st \_r1\(), [\_r0\()]
#endif
    set LEON_MEMORY_WPR_BASE+LEON_WPR1_OFFSET, \_r0\()
    set (1<<31), \_r2\()
    ld [\_r0\()], \_r1\()
    or \_r1\(), \_r2\(), \_r1\()
    st \_r1\(), [\_r0\()]
#endif
.endm
	
.macro CLEAR_WP_BIT _r0, _r1, _r2
#if defined(CONFIG_MPU)
    set LEON_MEMORY_WPR_BASE+LEON_WPR1_OFFSET, \_r0\()
    set (1<<31), \_r2\()
    ld [\_r0\()], \_r1\()
    andn \_r1\(), \_r2\(), \_r1\()
    st \_r1\(), [\_r0\()]
#if defined CONFIG_AT697F
    set LEON_MEMORY_WPR_BASE+LEON_WPR2_OFFSET, \_r0\()
    set (1<<31), \_r2\()
    ld [\_r0\()], \_r1\()
    andn \_r1\(), \_r2\(), \_r1\()
    st \_r1\(), [\_r0\()]
#endif
#endif
.endm

#if defined(CONFIG_MPU)&&defined(CONFIG_WA_PROTECT_MMAP_PREG_WPOINT)
#ifndef XM_CPU_FEATURE_WA1
#define XM_CPU_FEATURE_WA1 0x1
#endif
#endif

.macro CLEAR_WA_WATCHPOINT _r0
#if defined(CONFIG_MPU)&&defined(CONFIG_WA_PROTECT_MMAP_PREG_WPOINT)
	sethi %hi(xmcTab), \_r0\()
	or \_r0\(), %lo(xmcTab), \_r0\()
	ld [\_r0\()+_HPV_OFFSET+_CPUTAB_OFFSET+_FEATURES_OFFSET], \_r0\()
	andcc \_r0\(), XM_CPU_FEATURE_WA1, %g0
	be 5f
	nop
	rd %asr24, \_r0\()
	andn \_r0\(), 0x1, \_r0\()
	wr \_r0\(), %asr24
	rd %asr25, \_r0\()
	andn \_r0\(), 0x3, \_r0\()
	wr \_r0\(), %asr25
	WR_DELAY
5:
#endif
.endm

.macro SET_WA_WATCHPOINT _r0
#if defined(CONFIG_MPU)&&defined(CONFIG_WA_PROTECT_MMAP_PREG_WPOINT)
	sethi %hi(xmcTab), \_r0\()
	or \_r0\(), %lo(xmcTab), \_r0\()
	ld [\_r0\()+_HPV_OFFSET+_CPUTAB_OFFSET+_FEATURES_OFFSET], \_r0\()
	andcc \_r0\(), XM_CPU_FEATURE_WA1, %g0
	be 5f
	nop
	rd %asr24, \_r0\()
	or \_r0\(), 0x1, \_r0\()
	wr \_r0\(), %asr24
	rd %asr25, \_r0\()
	or \_r0\(), 0x3, \_r0\()
	wr \_r0\(), %asr25
	WR_DELAY
5:
#endif
.endm
	
.macro SET_IOP_BIT _r0, _r1, _r2
#if defined(CONFIG_MPU)
	set LEON_MEMORY_CFG_BASE, \_r0\()
	set (1<<19), \_r2\()
	ld [\_r0\()], \_r1\()
	or \_r1\(), \_r2\(), \_r1\()
	st \_r1\(), [\_r0\()]
#endif
.endm
	
.macro CLEAR_IOP_BIT _r0, _r1, _r2
#if defined(CONFIG_MPU)
	set LEON_MEMORY_CFG_BASE, \_r0\()
	set (1<<19), \_r2\()
	ld [\_r0\()], \_r1\()
	andn \_r1\(), \_r2\(), \_r1\()
	st \_r1\(), [\_r0\()]
#endif
.endm
	
/* <track id="test-PSR-Enter-XM"> */
.macro PSR_SET_ET_PIL _r
	rd %psr, \_r\()
	or \_r\(), (PSR_ET_BIT|PSR_PIL_MASK), \_r\()
	wr \_r\(), %psr
	WR_DELAY
.endm
/* </track id="test-PSR-Enter-XM"> */
	
.macro PSR_UNSET_ET _r
	rd %psr, \_r\()
	and \_r\(), ~(PSR_ET_BIT), \_r\()
	wr \_r\(), %psr
	WR_DELAY
.endm

.macro PSR_UNSET_ET_PIL _r
        rd %psr, \_r\()
        and \_r\(), ~(PSR_ET_BIT|PSR_PIL_MASK), \_r\()
        wr \_r\(), %psr
        WR_DELAY
.endm
 
.macro GET_CPUID _r
#ifdef CONFIG_SMP_SUPPORT      
        rd %asr17, \_r\()
        sra \_r\(), 0x1c, \_r\()
#else
        mov %g0, \_r\()
#endif
.endm
        
.macro GET_CKTHREAD _r, _tmp
#ifdef CONFIG_SMP_SUPPORT
        GET_CPUID \_r\()
        sll  \_r\(), 3, \_tmp\()
        add  \_tmp\(), \_r\(), \_tmp\()
        sethi  %hi(localProcessorInfo), \_r\()
        sll  \_tmp\(), 3, \_tmp\()
        or  \_r\(), %lo(localProcessorInfo), \_r\()
        add  \_tmp\(), \_r\(), \_tmp\()
#else
        set localProcessorInfo, \_tmp\()
#endif
        ld [\_tmp\()+_CKTHREAD_OFFSET+_LOCALSCHED_OFFSET], \_r\()
.endm

.macro GET_CPUCTXT _r, _tmp
        GET_CKTHREAD \_r\(), \_tmp\()
        ld [\_r\()+_CTRL_OFFSET+_IRQCPUCTXT_OFFSET], \_r\()
.endm

.macro SET_CPUCTXT _ctxt, _tmp, _tmp2
        GET_CKTHREAD \_tmp\(), \_tmp2\()
        st \_ctxt\(), [\_tmp\()+_CTRL_OFFSET+_IRQCPUCTXT_OFFSET]        
.endm
        
.macro GET_CKTHREAD_STACK _r, _t
	GET_CKTHREAD \_r\(), \_t\()
	set (CONFIG_KSTACK_SIZE-8), \_t\()
	add \_r\(), \_t\(), \_r\()
.endm

.macro GET_CKTHREAD_TBR _r, _tmp
	GET_CKTHREAD \_r\(), \_tmp\()
	ld [\_r\()+_CTRL_OFFSET+_G_OFFSET], \_r\()
	ld [\_r\()+_KARCH_OFFSET+_TBR_OFFSET], \_r\()
.endm

.macro GET_CKTHREAD_FLAGS _r, _tmp
	GET_CKTHREAD \_r\(), \_tmp\()
	ld [\_r\()+_CTRL_OFFSET+_FLAGS_OFFSET], \_r\()
.endm

.macro GET_PARTITION_CFG _r, _tmp
        GET_CKTHREAD \_r\(), \_tmp\()
        
        ld [\_r\() + _CTRL_OFFSET +  _G_OFFSET], \_tmp\()
        ldub  [ \_tmp\() + 3 ], \_r\()

        set partitionTab, \_tmp\()
        ld  [ \_tmp\()], \_tmp\()
        
        umul  \_r\(), _PARTITION_T_SIZEOF, \_r\()
        add  \_tmp\(), \_r\(), \_tmp\()
        ld [\_tmp\()+_PARTITION_CFG_OFFSET], \_r\()
.endm
        
.macro SET_CKTHREAD_SWTRAP _swtrap, _t, _tmp
	GET_CKTHREAD \_t\(), \_tmp\()
	ld [\_t\()+_CTRL_OFFSET+_G_OFFSET], \_t\()
        st \_swtrap\(), [\_t\()+_SWTRAP_OFFSET]
.endm
        
.macro FROM_SV _psr, _to
	andcc \_psr\(), PSR_PS_BIT, %g0
        bne \_to\()
	nop
.endm

.macro FROM_USR _psr, _to
	andcc \_psr\(), PSR_PS_BIT, %g0
        be \_to\()
	nop
.endm
	
.macro SAVE_CWND _r
        std %i0, [\_r\()+I0_OFFS]
        nop
        std %i2, [\_r\()+I2_OFFS]
        nop
        std %i4, [\_r\()+I4_OFFS]
        nop
        std %i6, [\_r\()+I6_OFFS]
        nop
        std %l0, [\_r\()+L0_OFFS]
        nop
        std %l2, [\_r\()+L2_OFFS]
        nop
        std %l4, [\_r\()+L4_OFFS]
        nop
        std %l6, [\_r\()+L6_OFFS]
        nop
.endm
	
.macro RESTORE_CWND _r
	ldd [\_r\()+I0_OFFS], %i0
	ldd [\_r\()+I2_OFFS], %i2
	ldd [\_r\()+I4_OFFS], %i4
	ldd [\_r\()+I6_OFFS], %i6
	ldd [\_r\()+L0_OFFS], %l0
	ldd [\_r\()+L2_OFFS], %l2
	ldd [\_r\()+L4_OFFS], %l4
	ldd [\_r\()+L6_OFFS], %l6
.endm

.macro SAVE_REGRW _sp, _r
	std \_r\()0, [\_sp\()+0x0]
	nop
	std \_r\()2, [\_sp\()+0x8]
	nop
	std \_r\()4, [\_sp\()+0x10]
	nop
	std \_r\()6, [\_sp\()+0x18]
	nop
.endm
	
.macro RESTORE_REGRW _sp, _r
        ldd [\_sp\()+0x0], \_r\()0
        ldd [\_sp\()+0x8], \_r\()2
        ldd [\_sp\()+0x10], \_r\()4
        ldd [\_sp\()+0x18], \_r\()6
.endm
	
.macro SAVE_IRQCTXT _sp
	/* storing psr and pc */
	std %l0, [\_sp\()+48]
	nop
	/* storing npc and irqnr */
	std %l2, [\_sp\()+40]
	nop
	std %g6, [\_sp\()+32]
	nop
	std %g4, [\_sp\()+24]
	nop
	std %g2, [\_sp\()+16]
	nop
	st %g1, [\_sp\()+12]
	nop
	rd %y, %g1
	st %g1, [\_sp\()+8]
        GET_CPUCTXT %g1, %g2
        st %g1, [\_sp\()]
        SET_CPUCTXT \_sp\(), %g1, %g2
.endm

.macro RESTORE_IRQCTXT _sp
        ld [\_sp\()], %g1
        SET_CPUCTXT %g1, %g2, %g3
	ld [\_sp\()+8], %g1
	wr %g1, %y
	ld [\_sp\()+12], %g1
	ldd [\_sp\()+16], %g2
	ldd [\_sp\()+24], %g4
	ldd [\_sp\()+32], %g6
	ldd [\_sp\()+40], %l2
	ldd [\_sp\()+48], %l0
.endm

.macro CKTHREAD_CTRL_GET_IFLAGS _r, _iFlags
      	GET_CKTHREAD \_r\(), \_iFlags\()
	ld [\_r\()+_CTRL_OFFSET+_G_OFFSET], \_r\()
	ld [\_r\()+_PARTCTRLTAB_OFFSET], \_r\()
	ld [\_r\()+_IFLAGS_OFFSET+_VAL_OFFSET], \_iFlags\()
        
.endm  
        
.macro CKTHREAD_CTRL_RESTORE_IFLAGS _r, _iFlags
        st \_iFlags\(), [\_r\()+_IFLAGS_OFFSET+_VAL_OFFSET]
.endm
        
.macro CKTHREAD_CTRL_SET_IFLAGS _r, _iFlags, _tmp
        GET_CKTHREAD \_r\(), \_tmp\()
	ld [\_r\()+_CTRL_OFFSET+_G_OFFSET], \_r\()
	ld [\_r\()+_PARTCTRLTAB_OFFSET], \_r\()
        st \_iFlags\(), [\_r\()+_IFLAGS_OFFSET+_VAL_OFFSET]
.endm

.macro SET_CACHE_CFG _r0, _r1, _r2, _r3
        GET_CKTHREAD \_r0\(), \_r1\()
        ld [\_r0\()+_CTRL_OFFSET+_FLAGS_OFFSET], \_r1\()

#ifdef CONFIG_LEON2
        set LEON_CCR_BASE, \_r3\()
        ld [\_r3\()], \_r2\()
#else
        lda [%g0] LEON_CCR_BASE, \_r2\()
#endif

        andn \_r2\(), 0x3|0xc, \_r2\()

        btst 1<<8, \_r1\() 
        be 1f
        nop
        or \_r2\(), 0x3, \_r2\()
1:
        btst 1<<7, \_r1\() 
        be 2f
        nop
        or \_r2\(), 0xc, \_r2\()
2:

#ifdef CONFIG_LEON2
        set LEON_CCR_BASE, \_r3\()
        st \_r2\(), [\_r3\()]
#else
        lda [%g0] LEON_CCR_BASE, \_r3\()
/*        and \_r2\(), \_r3\(), \_r3\()       */
        cmp \_r2\(), \_r3\()
        be 1f
        nop
        set (1<<22)|(1<<21), \_r3\()
        or \_r2\(), \_r3\(), \_r2\()
        sta \_r2\(), [%g0] LEON_CCR_BASE
1:
#endif
.endm
        
.macro DO_FLUSH_CACHE _r0, _r1, _r2, _r3
        GET_CKTHREAD \_r0\(), \_r1\()
        ld [\_r0\()+_CTRL_OFFSET+_FLAGS_OFFSET], \_r1\()

#ifdef CONFIG_MMU
        srl \_r1\(), 7, \_r2\()
        srl \_r1\(), 5, \_r1\()
        and \_r1\(), \_r2\(), \_r1\()
#elif CONFIG_MPU        
        srl \_r1\(), 5, \_r1\()
#endif
        
        andcc \_r1\(), 2, %g0
        bne 61f
        andcc \_r1\(), 1, %g0
        be 62f
        nop
        b 63f
        nop        
61:
        FLUSH_ICACHE(\_r2\(), \_r3\())
        andcc \_r1\(), 1, %g0
        be 62f
        nop
63:
        FLUSH_DCACHE(\_r2\(), \_r3\())
62:
        ld [\_r0\()+_CTRL_OFFSET+_FLAGS_OFFSET], \_r1\()
        andn \_r1\(), (1<<5)|(1<<6), \_r1\()
        st \_r1\(), [\_r0\()+_CTRL_OFFSET+_FLAGS_OFFSET]
.endm
        
.text
.align 4

ENTRY(WindowOverflowTrap)
	FROM_SV %l0, 3f
        
#if defined(CONFIG_MMU)
! Checking the correctness of %sp
	rd %wim, %l5
	wr %g0, %wim
	WR_DELAY
	mov %g2, %l6
	
	save
	mov %sp, %g2
	restore
	wr %l5, %wim
	WR_DELAY

        andcc %g2, 0x7, %g0
        bne 2f
        
       	set CONFIG_XM_OFFSET, %l5
        cmp %g2, %l5
        blu 1f
        nop
        set 16*1024*1024, %l7
        add %l5, %l7, %l5
        cmp %g2, %l5
        bgu 1f
        nop
2:
	clr %l3
	sethi %hi(DoUnrecovExcp), %l4
	b CommonTrapHandler
	or %l4, %lo(DoUnrecovExcp), %l4

1:	mov %l6, %g2
#endif        

#if defined(CONFIG_MPU)
! Checking the correctness of %sp
	rd %wim, %l5
	wr %g0, %wim
	WR_DELAY
	mov %g2, %l6
	
        save
        mov %sp, %g2
        restore
	wr %l5, %wim
	WR_DELAY

        andcc %g2, 0x7, %g0
        bne 2f

	set _sxm, %l5
        cmp %l5, %g2
        bg 2f
        nop
        set _exm, %l5        
        cmp %g2, %l5
        bg 1f
       	nop
2:
	clr %l3
	sethi %hi(DoUnrecovExcp), %l4
	b CommonTrapHandler
	or %l4, %lo(DoUnrecovExcp), %l4

1:      mov %l6, %g2
#endif

	CLEAR_WA_WATCHPOINT %l5
	CLEAR_WP_BIT %l5, %l6, %l7
	SET_IOP_BIT %l5, %l6, %l7
3:
        set __fromWH, %l5
        mov 1, %l6
        st %l6, [%l5]
!	PSR_SET_ET_PIL %l5

	save ! Decrement CWP
	nop
	
	SAVE_CWND %sp

	rd %wim, %l0	! Rotate WIM register one bit to the right
	sll %l0, CONFIG_REGISTER_WINDOWS-1, %l1
	srl %l0, 1, %l0	! Next three instructions following wr
	wr %l0, %l1, %wim	! must not be restore, save nor rett
	WR_DELAY

	restore			! Increment CWP
	FROM_SV %l0, 3f
        set __fromWH, %l5
        st %g0, [%l5]
	SET_WP_BIT %l5, %l6, %l7
	CLEAR_IOP_BIT %l5, %l6, %l7
	SET_WA_WATCHPOINT %l5
        b 4f
3:
        set __fromWH, %l5
        st %g0, [%l5]
4:        
	RESTORE_PSR %l0

	jmpl %l1, %g0
	rett %l2
ENTRY(EWindowOverflowTrap)
        
ENTRY(WindowUnderflowTrap)
	FROM_SV %l0, 3f

#if defined(CONFIG_MMU)
! Checking the correctness of %sp
	rd %wim, %l5
	wr %g0, %wim
	WR_DELAY
	mov %g2, %l6
	
	restore
	restore
	mov %sp, %g2
	save
	save
	wr %l5, %wim
	WR_DELAY

        andcc %g2, 0x7, %g0
        bne 2f

 	set CONFIG_XM_OFFSET, %l5
        cmp %g2, %l5
        blu 1f
        nop
        set 16*1024*1024, %l7
        add %l5, %l7, %l5
        cmp %g2, %l5
        bgu 1f
        nop
        
2:      
	clr %l3
	sethi %hi(DoUnrecovExcp), %l4
	b CommonTrapHandler
	or %l4, %lo(DoUnrecovExcp), %l4

1:
        mov %l6, %g2
#endif

#if defined(CONFIG_MPU)
! Checking the correctness of %sp
	rd %wim, %l5
	wr %g0, %wim
	WR_DELAY
	mov %g2, %l6
	
	restore
	restore
	mov %sp, %g2
	save
	save
	wr %l5, %wim
	WR_DELAY

        andcc %g2, 0x7, %g0
        bne 2f

	set _sxm, %l5
        cmp %l5, %g2
        bg 2f
        nop
        set _exm, %l5        
        cmp %g2, %l5
        bg 1f
        nop
2:
	clr %l3
	sethi %hi(DoUnrecovExcp), %l4
	b CommonTrapHandler
	or %l4, %lo(DoUnrecovExcp), %l4        

1:      mov %l6, %g2
#endif

	CLEAR_WA_WATCHPOINT %l5
	CLEAR_WP_BIT %l5, %l6, %l7
	SET_IOP_BIT %l5, %l6, %l7
3:
        set __fromWH, %l5
        mov 1, %l6
        st %l6, [%l5]
	PSR_SET_ET_PIL %l5
			! Rotate WIM register one bit to the left
	rd %wim, %l5	! Instruction in the entry
	sll %l5, 1, %l6	! Instruction delayed in the entry
	srl %l5, CONFIG_REGISTER_WINDOWS-1, %l5
	wr %l5, %l6, %wim
	WR_DELAY

	restore			! CWP -> Window saved on the stack
	restore

	RESTORE_CWND %sp

	save			! Come back to the trap window
	save			! where return address is located

	FROM_SV %l0, 3f
        set __fromWH, %l5
        st %g0, [%l5]
	SET_WP_BIT %l5, %l6, %l7
	CLEAR_IOP_BIT %l5, %l6, %l7
	SET_WA_WATCHPOINT %l5
        b 4f
3:
        set __fromWH, %l5
        st %g0, [%l5]
4:
	RESTORE_PSR %l0
	
	jmpl %l1, %g0
	rett %l2
ENTRY(EWindowUnderflowTrap)
        
ENTRY(FpDisabledTrap)
	FROM_SV %l0, 3f
	CLEAR_WA_WATCHPOINT %l5
	CLEAR_WP_BIT %l5, %l6, %l7
	SET_IOP_BIT %l5, %l6, %l7
3:
	!PSR_SET_ET_PIL %l5

	! Is current thread allowed to use FP?
        GET_PARTITION_CFG %l5, %l6
        ld [%l5+_CFG_FLAGS_OFFSET], %l5
	andcc %l5, 0x200, %g0 ! XM_PART_FP
	bne 4f
	nop

41:	
	sethi %hi(DoTrap), %l4
	b CommonTrapHandler
	or %l4, %lo(DoTrap), %l4

4:
        CKTHREAD_CTRL_GET_IFLAGS %l5, %l6
        set PSR_EF_BIT, %l5
        andcc %l6, %l5, %l6
        be 41b
        nop
	
	rd %psr, %l5
	set PSR_EF_BIT, %l6
	or %l5, %l6, %l5
	wr %l5, %psr
	WR_DELAY
        
// FIX THIS
	! Current FPU's owner
	set localProcessorInfo, %l7	
	ld [%l7+_FPUOWNER_OFFSET+_LOCALSCHED_OFFSET], %l6

	cmp %l6, 0
	be 1f
	nop

	// Current ckthread
	ld [%l7+_CKTHREAD_OFFSET+_LOCALSCHED_OFFSET], %l5
	
	cmp %l5, %l6
	be 2f
	nop
	
	ld [%l6+_CTRL_OFFSET+_G_OFFSET], %l5
	! Store the floating point unit state and retore
	! the current partition's one
	st %fsr, [%l5+128+_FPUREGS_OFFSET+_KARCH_OFFSET]
	std %f0, [%l5+0+_FPUREGS_OFFSET+_KARCH_OFFSET]
	nop
	std %f2, [%l5+8+_FPUREGS_OFFSET+_KARCH_OFFSET]
	nop
	std %f4, [%l5+16+_FPUREGS_OFFSET+_KARCH_OFFSET]
	nop
	std %f6, [%l5+24+_FPUREGS_OFFSET+_KARCH_OFFSET]
	nop
	std %f8, [%l5+32+_FPUREGS_OFFSET+_KARCH_OFFSET]
	nop
	std %f10, [%l5+40+_FPUREGS_OFFSET+_KARCH_OFFSET]
	nop
	std %f12, [%l5+48+_FPUREGS_OFFSET+_KARCH_OFFSET]
	nop
	std %f14, [%l5+56+_FPUREGS_OFFSET+_KARCH_OFFSET]
	nop
	std %f16, [%l5+64+_FPUREGS_OFFSET+_KARCH_OFFSET]
	nop
	std %f18, [%l5+72+_FPUREGS_OFFSET+_KARCH_OFFSET]
	nop
	std %f20, [%l5+80+_FPUREGS_OFFSET+_KARCH_OFFSET]
	nop
	std %f22, [%l5+88+_FPUREGS_OFFSET+_KARCH_OFFSET]
	nop
	std %f24, [%l5+96+_FPUREGS_OFFSET+_KARCH_OFFSET]
	nop
	std %f26, [%l5+104+_FPUREGS_OFFSET+_KARCH_OFFSET]
	nop
	std %f28, [%l5+112+_FPUREGS_OFFSET+_KARCH_OFFSET]
	nop
	std %f30, [%l5+120+_FPUREGS_OFFSET+_KARCH_OFFSET]
	nop
1:
	ld [%l7+_CKTHREAD_OFFSET+_LOCALSCHED_OFFSET], %l6
	st %l6, [%l7+_FPUOWNER_OFFSET+_LOCALSCHED_OFFSET]

	ld [%l6+_CTRL_OFFSET+_G_OFFSET], %l5
	
	ldd [%l5+0+_FPUREGS_OFFSET+_KARCH_OFFSET], %f0
	ldd [%l5+8+_FPUREGS_OFFSET+_KARCH_OFFSET], %f2
	ldd [%l5+16+_FPUREGS_OFFSET+_KARCH_OFFSET], %f4
	ldd [%l5+24+_FPUREGS_OFFSET+_KARCH_OFFSET], %f6
	ldd [%l5+32+_FPUREGS_OFFSET+_KARCH_OFFSET], %f8
	ldd [%l5+40+_FPUREGS_OFFSET+_KARCH_OFFSET], %f10
	ldd [%l5+48+_FPUREGS_OFFSET+_KARCH_OFFSET], %f12
	ldd [%l5+56+_FPUREGS_OFFSET+_KARCH_OFFSET], %f14
	ldd [%l5+64+_FPUREGS_OFFSET+_KARCH_OFFSET], %f16
	ldd [%l5+72+_FPUREGS_OFFSET+_KARCH_OFFSET], %f18
	ldd [%l5+80+_FPUREGS_OFFSET+_KARCH_OFFSET], %f20
	ldd [%l5+88+_FPUREGS_OFFSET+_KARCH_OFFSET], %f22
	ldd [%l5+96+_FPUREGS_OFFSET+_KARCH_OFFSET], %f24
	ldd [%l5+104+_FPUREGS_OFFSET+_KARCH_OFFSET], %f26
	ldd [%l5+112+_FPUREGS_OFFSET+_KARCH_OFFSET], %f28
	ldd [%l5+120+_FPUREGS_OFFSET+_KARCH_OFFSET], %f30
	ld [%l5+128+_FPUREGS_OFFSET+_KARCH_OFFSET], %fsr

2:
	//FROM_SV %l0, 4f
	SET_WP_BIT %l5, %l6, %l7
	CLEAR_IOP_BIT %l5, %l6, %l7
	SET_WA_WATCHPOINT %l5
4:	
	set PSR_EF_BIT, %l5
	or %l0, %l5, %l0

	RESTORE_PSR %l0
	
	jmpl %l1, %g0
	rett %l2
	
3:
	sethi %hi(DoTrap), %l4
	b CommonTrapHandler
	or %l4, %lo(DoTrap), %l4

KPreempt:
.Tbegin_kpreempt:
!        sub %fp, 0x20, %l5
!        SAVE_REGRW %l5, %o
        
	sub %fp, (MIN_STACK_FRAME+_CPUCTXT_SIZEOF), %sp ! The stack if we came from SV context
	
	! save irqctxt
	sub %fp, _CPUCTXT_SIZEOF, %l5
	SAVE_IRQCTXT %l5
        
	rd %wim, %g1

	! check if there is an overflow
        srl %g1, %l0, %g3
        cmp %g3, 1
        bne 1f
        nop
	
	wr %g0, %wim
	WR_DELAY
	
	! so, solve the overflow
        save
	MOD_SHIFT2RIGHT %g1, %g7, CONFIG_REGISTER_WINDOWS
	SAVE_CWND %sp
	restore

	wr %g1, %wim
	WR_DELAY
	
1:
	PSR_SET_ET_PIL %g1

	! call handler
	call %l4
	sub %fp, _CPUCTXT_SIZEOF, %o0

	PSR_UNSET_ET %g1

        rd %wim, %l4
        add %l0, 1, %l6
        and %l6, PSR_CWP_MASK, %l6
        srl %l4, %l6, %l5
        cmp %l5, 1
        bne 2f
        
        MOD_SHIFT2LEFT %l4, %l5, CONFIG_REGISTER_WINDOWS
        
        wr %l4, 0, %wim
       	WR_DELAY

        restore
        RESTORE_CWND %sp
        save
2:              
	sub %fp, _CPUCTXT_SIZEOF, %l5
	RESTORE_IRQCTXT %l5

!        sub %fp, 0x20, %l5
!        RESTORE_REGRW %l5, %o
        
//        DO_FLUSH_CACHE %l5, %l6
	RESTORE_PSR %l0
.Tend_kpreempt:
	jmpl %l1, %g0
	rett %l2
        
#ifdef CONFIG_DEV_LICE_INTERFACE
#include <drivers/lice_interface.h>
#endif
        
ENTRY(CommonTrapHandler)
.Tbegin_trap:
        rd %psr, %l0
        set __fromWH, %l5
        ld [%l5], %l5
        cmp %l5, 1
        be 111f
        nop

	FROM_SV %l0, KPreempt
111:
	CLEAR_WA_WATCHPOINT %l5
	CLEAR_WP_BIT %l5, %l6, %l7
	SET_IOP_BIT %l5, %l6, %l7

FromIRet:
#ifdef CONFIG_DEV_LICE_INTERFACE
!	SET_IOP_BIT(%l5, %l6, %l7)
        LICE_CMD_BEGIN_TRAP(%l5, %l6)
#endif
	! Getting the partition's sv stack	
	GET_CKTHREAD_STACK %l5, %l6
	
	! save irqctxt
	sub %l5, _CPUCTXT_SIZEOF, %l6
	SAVE_IRQCTXT %l6
        
	mov %l6, %g2

	rd %wim, %g1	
	wr %g0, %wim
	WR_DELAY

	mov %g1, %l6

	! %g1 <- %wim
	! %g2 <- %sp
	! %g3 <- counter
	! %g4 <- CWP

	rd %psr, %g6
	
	! replacing all the reg_wind's %sp
	MOD_INTEGER2BITMAP %g4, %g6, %g7, CONFIG_REGISTER_WINDOWS
	MOD_SHIFT2LEFT %g4, %g7, CONFIG_REGISTER_WINDOWS
	andcc %g4, %g1, %g0
	bne 1f
	clr %g3

	restore
2:
	sub %g2, 8, %g2
	st %sp, [%g2]
	sub %g2, REG_WND_FRAME, %g2
	mov %g2, %sp
	MOD_SHIFT2LEFT %g4, %g7, CONFIG_REGISTER_WINDOWS
	inc %g3
	andcc %g4, %g1, %g0
	be,a 2b
	restore
1:
	! comming back to the original register window
	rd %psr, %g5
	sub %g5, %g3, %g6	     
	and %g6, CONFIG_REGISTER_WINDOWS-1, %g6
	andn %g5, CONFIG_REGISTER_WINDOWS-1, %g5
	wr %g5, %g6, %psr
	WR_DELAY

	mov %g3, %l7
	
	! %l5 <- start %sp
	! %l6 <- %wim
	! %l7 <- counter (%g3)

!3:
	! check if there is an overflow
        srl %g1, %l0, %g3
        cmp %g3, 1
        bne 1f
        nop

	! so solve the overflow
        save
	MOD_SHIFT2RIGHT %g1, %g7, CONFIG_REGISTER_WINDOWS
	SAVE_CWND %sp
	restore

1:
	wr %g1, %wim
	WR_DELAY

	sub %g2, MIN_STACK_FRAME, %sp
	PSR_SET_ET_PIL %g1

	! call handler
	call %l4
	sub %l5, _CPUCTXT_SIZEOF, %o0

	call RaisePendIrqs
	sub %l5, _CPUCTXT_SIZEOF, %o0

	! %l4 <- RaisePendIrqs
	mov %o0, %l4
    
	PSR_UNSET_ET %g1

	! restore all

	mov %l6, %g1
	sub %l5, _CPUCTXT_SIZEOF, %g2
	mov %l7, %g3
	rd %wim, %g4

	wr %g0, %wim
	WR_DELAY

!	FROM_SV %l0, 5f
	! %g1 <- wim from CWP
	! %g2 <- %sp (%l5-_CPUCTXT_SIZEOF)
	! %g3 <- counter (%l7)
	! %g4 <- stored %wim
	
	cmp %g3, %g0 ! counter
	ble 4f
	mov %g3, %g5

	rd %psr, %g6	
	! replacing all the reg_wind's %sp
	MOD_INTEGER2BITMAP %g1, %g6, %g7, CONFIG_REGISTER_WINDOWS
2:
	MOD_SHIFT2LEFT %g1, %g7, CONFIG_REGISTER_WINDOWS
	andcc %g1, %g4, %g0
	bne 3f
	nop

	restore
	sub %g2, 8, %g2
	ld [%g2], %sp
	dec %g5
	cmp %g5, %g0
	bg 2b
	sub %g2, REG_WND_FRAME, %g2
	
	b 4f
	nop
	
3:	
	restore
	sub %g2, 8, %g2
	ld [%g2], %g7
	sub %g2, REG_WND_FRAME, %g2
	RESTORE_CWND %g2
	
	dec %g5
	cmp %g5, %g0
	bg 3b
	mov %g7, %sp	
4:
	! comming back to the original register window
	rd %psr, %g5
	sub %g5, %g3, %g6
	and %g6, CONFIG_REGISTER_WINDOWS-1, %g6
	andn %g5, CONFIG_REGISTER_WINDOWS-1, %g5
	wr %g5, %g6, %psr
	WR_DELAY
!5:
	wr %l6, %wim
	WR_DELAY
        
	sub %l5, _CPUCTXT_SIZEOF, %l6
	RESTORE_IRQCTXT %l6

	! Is it required to trigger a virq?
	! %l4 will hold vTBR address
!	andcc %l4, 0x1, %g0
        cmp %l4, -1
	bne,a EmulateTrapSv
        mov %l4, %l3
!	srl %l4, 1, %l3

!	FROM_SV %l0, 3f
#ifdef CONFIG_DEV_LICE_INTERFACE
        LICE_CMD_END_TRAP(%l5, %l6)
#endif
        SET_CACHE_CFG %l4, %l5, %l6, %l7
        DO_FLUSH_CACHE %l4, %l5, %l6, %l7
	SET_WP_BIT %l5, %l6, %l7
	CLEAR_IOP_BIT %l5, %l6, %l7
	SET_WA_WATCHPOINT %l5

	RESTORE_PSR %l0
.Tend_trap:
	jmpl %l1, %g0
	rett %l2

! Called from CommonTrapHandler
ENTRY(DoHypercall)
#ifdef CONFIG_DEV_LICE_INTERFACE
	 ! %i0 will not be modified.
        LICE_CMD_HYPERCALL_NR(%l4, %o1, %i0)
#endif
	mov %o7, %l4
	cmp %i0, (NR_HYPERCALLS-1)
	bg 1f
	nop

	set hypercallsTab, %g1
	sll %i0, 2, %g2
	ld [%g1+%g2], %g1
	
	cmp  %g1, 0
        be 1f
        nop

	mov %i1, %o0
	mov %i2, %o1
	mov %i3, %o2
	mov %i4, %o3

	call %g1
	mov %i5, %o4
       	mov %o0, %i0
        
	b 2f
	nop
1:
	mov XM_UNKNOWN_HYPERCALL, %o0
2:
	ld [%l5-4], %l1
	ld [%l5-16], %l2
	
	mov %l2, %l1
	add %l2, 4, %l2

	st %l1, [%l5-4]
	st %l2, [%l5-16]

	! store returned value
	jmp %l4+4
        nop
	
ENTRY(AsmHypercallHandler)
	! l0 <- psr
	! l1 <- pc
	! l2 <- npc
	
	FROM_SV %l0, 3f
	CLEAR_WA_WATCHPOINT %l5
	CLEAR_WP_BIT %l5, %l6, %l7
	SET_IOP_BIT %l5, %l6, %l7
3:
#ifdef CONFIG_DEV_LICE_INTERFACE
	 ! %i0 will not be modified.
        LICE_CMD_ASM_HYPERCALL_NR(%l5, %l6, %i0)
#endif	
        
	cmp %i0,(NR_ASM_HYPERCALLS-1)
	bg 1f
	nop

	set asmHypercallsTab, %l5
	sll %i0, 2, %l6
	ld [%l5+%l6], %l5

 	cmp  %l5, 0
	be 1f
	nop
	
	jmpl %l5, %l7
	nop
#ifdef CONFIG_DEV_LICE_INTERFACE
        LICE_CMD_END_ASM_HYPERCALL(%l5, %l6)
#endif	
	
1:
	FROM_SV %l0, 3f

        SET_CACHE_CFG %l4, %l5, %l6, %l7
        DO_FLUSH_CACHE %l4, %l5, %l6, %l7
	SET_WP_BIT %l5, %l6, %l7
	CLEAR_IOP_BIT %l5, %l6, %l7
	SET_WA_WATCHPOINT %l5
        b 4f
3:

        SET_CACHE_CFG %l4, %l5, %l6, %l7
        DO_FLUSH_CACHE %l4, %l5, %l6, %l7
4:
	RESTORE_PSR %l0
	
.Tend_ahp:	
	jmp %l2
	rett %l2+4

ENTRY(EmulateTrap)
        FROM_SV %l0, 3f
	CLEAR_WA_WATCHPOINT %l5
	CLEAR_WP_BIT %l5, %l6, %l7
	SET_IOP_BIT %l5, %l6, %l7
3:
        sll %l3, 1, %l3
        or %l3, 1, %l3
        SET_CKTHREAD_SWTRAP %l3, %l5, %l6
        
        set _retl, %l4
	set FromIRet, %l5

	jmp %l5
	nop
        
EmulateTrapSv:
        CKTHREAD_CTRL_GET_IFLAGS %l4, %l5
	andn %l5, PSR_ET_BIT, %l5
        CKTHREAD_CTRL_RESTORE_IFLAGS %l4, %l5

! The kthread stack is used as free room to store the current
! rw, in the case an overflow/underflow trap is raised then,
! its content will be smashed
        
        GET_CKTHREAD_STACK %l4, %l5

        mov %g4, %l5
        sub %l4, 0x20, %g4
        SAVE_REGRW %g4, %l
                
        PSR_SET_ET_PIL %l4

        restore
        save
        save
        restore

        GET_CKTHREAD_STACK %l4, %l5

        sub %l4, 0x20, %g4
        RESTORE_REGRW %g4, %l
        mov %l5, %g4
        
        PSR_UNSET_ET %l4
        
        mov %l3, %o0

       	andn %l0, PSR_CWP_MASK, %o2
        
       	set PSR_PIL_MASK|PSR_ET_BIT, %l5
        andn %l0, %l5, %l0 ! <- saving old PSR (FLAGS)

        CKTHREAD_CTRL_GET_IFLAGS %l5, %l4
        or %l0, %l4, %l0

       	save
	
	FROM_SV %l0, 3f

        SET_CACHE_CFG %l4, %l5, %l6, %l7
        DO_FLUSH_CACHE %l4, %l5, %l6, %l7
	SET_WP_BIT %l5, %l6, %l7
	CLEAR_IOP_BIT %l5, %l6, %l7
	SET_WA_WATCHPOINT %l5
        b 4f
3:

        SET_CACHE_CFG %l4, %l5, %l6, %l7
        DO_FLUSH_CACHE %l4, %l5, %l6, %l7
4:
       	rd %psr, %l0
	and %l0, PSR_CWP_MASK, %l0
	wr %i2, %l0, %psr        
       	WR_DELAY

.Tend_etrap: 
	jmp %i0
	rett %i0+4
        
! CWP-1(%l0) <- old PSR
! CWP-1(%l1) <- PC
! CWP-1(%l2) <- nPC
ENTRY(SparcIRetSys)
	restore

ENTRY(SIRetCheckRetAddr)
	! Checking the correctness of %l1 and %l2
        PSR_SET_ET_PIL %l5
!       	set CONFIG_XM_OFFSET, %l5
!       	cmp %l1, %l5
!	bgu 14f
!        nop
        
!        cmp %l2, %l5
!	bleu 24f
!        nop
        
!14:
!        save 
!       	clr %l3
!	sethi %hi(DoUnrecovExcp), %l4
!	b CommonTrapHandler
!	or %l4, %lo(DoUnrecovExcp), %l4
!24:

        ld [%l1], %l5
        ld [%l2], %l5
        PSR_UNSET_ET_PIL %l5
ENTRY(EIRetCheckRetAddr)

        rd %psr, %l3
        
        rd %wim, %l4
        add %l3, 1, %l5
        and %l5, CONFIG_REGISTER_WINDOWS-1, %l5
        srl %l4, %l5, %l5
        cmp %l5, 1
        be 123f
        nop

        set PSR_ICC_MASK, %l4

        andn %l3, %l4, %l3
        and %l0, %l4, %l0

        or %l0, %l3, %l0

        CKTHREAD_CTRL_GET_IFLAGS %l4, %l3
        or %l3, PSR_ET_BIT, %l3
        CKTHREAD_CTRL_RESTORE_IFLAGS %l4, %l3
        
        and %l3, PSR_PIL_MASK, %l3
        cmp %l3, PSR_PIL_MASK
        be 112f
        nop

	set _retl, %l4
	set FromIRet, %l5

	jmp %l5
	nop

112:
       	! returning to userland so it isn't necessary to check where we return
        SET_CACHE_CFG %l4, %l5, %l6, %l7
        DO_FLUSH_CACHE %l4, %l5, %l6, %l7
	SET_WP_BIT %l5, %l6, %l7
	CLEAR_IOP_BIT %l5, %l6, %l7
	SET_WA_WATCHPOINT %l5
        
       	RESTORE_PSR %l0

	! returning to the old pc and npc
	jmp %l1
	rett %l2

123:
        save
	clr %l3
	sethi %hi(DoUnrecovExcp), %l4
	b CommonTrapHandler
	or %l4, %lo(DoUnrecovExcp), %l4

_retl:
	retl
	nop
	
ENTRY(SparcFlushRegWinSys)
	! %g3 <- counter
	! %g4 <- WIM
	! %g5 <- CWIM
	! %g6 <- tmp
	PSR_SET_ET_PIL %l5
	mov %g3, %l3
	mov %g4, %l4
	mov %g5, %l5
	mov %g6, %l6

	rd %wim, %g4
	wr %g0, %wim
	WR_DELAY

	mov CONFIG_REGISTER_WINDOWS-2, %g3

	MOD_INTEGER2BITMAP %g5, %l0, %g6, CONFIG_REGISTER_WINDOWS
	andcc %g5, %g4, %g0
	bne 3f
	nop

1:	dec %g3
	save
	MOD_SHIFT2RIGHT %g5, %g6, CONFIG_REGISTER_WINDOWS
	andcc %g5, %g4, %g0
	bne 2f
	nop

	b 1b
	nop
	
2:	cmp %g3, 0
	be 4f
	nop
	
3:	dec %g3
	save
	SAVE_CWND %sp
	cmp %g3, 0
	bne 3b
	nop
4:	
	save
	save

	add %l0, 2, %g6
	and %g6, CONFIG_REGISTER_WINDOWS-1, %g6
	
	mov 1, %g5
	sll %g5, %g6, %g5
	wr %g5, %wim
	WR_DELAY
	
	mov %l4, %g4
	mov %l5, %g5
	mov %l6, %g6
	jmp %l7+4
	mov %l3, %g3

ENTRY(SparcGetPsrSys)        
        set PSR_PIL_MASK|PSR_ET_BIT|PSR_EF_BIT, %l3
       	andn %l0, %l3, %i0
        CKTHREAD_CTRL_GET_IFLAGS %l5, %l4
        
        and %l4, %l3, %l4
        or %i0, %l4, %i0
        
	jmp %l7+4
	nop
	
ENTRY(SparcSetPsrSys)
	set PSR_ICC_MASK, %l3
	and %i1, %l3, %l4
	andn %l0, %l3, %l0
	or %l4, %l0, %l0

        set PSR_PIL_MASK|PSR_ET_BIT, %l3
        
        GET_PARTITION_CFG %l4, %l5
        ld [%l4+_CFG_FLAGS_OFFSET], %l4
	andcc %l4, 0x200, %g0 ! XM_PART_FP
        be 111f
        nop

        set PSR_EF_BIT, %l5
        or %l3, %l5, %l3

        and %i1, %l5, %l4
        andn %l0, %l5, %l0
        or %l4, %l0, %l0

111:
      	and %i1, %l3, %l3

        CKTHREAD_CTRL_SET_IFLAGS %l4, %l3, %l5
        
        and %l3, PSR_ET_BIT, %l4
        cmp %l4, PSR_ET_BIT
        bne 112f
        nop

        and %l3, PSR_PIL_MASK, %l4
        cmp %l4, PSR_PIL_MASK
        be 112f
        nop

        mov %l2, %l1
        add %l2, 4, %l2
        
	set _retl, %l4
	set FromIRet, %l5

	jmp %l5
        nop
        
112:
       	jmp %l7+4
	nop
        
/*ENTRY(SparcFlushCacheSys)
        FLUSH_CACHE()
       	jmp %l7+4
	nop
*/
        
!ENTRY(SparcFlushTlbSys)
!        FLUSH_CACHE()        
!        set FLUSH_ENTIRE, %l3
!        sta %g0, [%l3] LEON_MMU_FLUSH
!        jmp %l7+4
!	nop 

ENTRY(SparcSetPilSys)
!        set PSR_PIL_MASK|PSR_ET_BIT, %l3
!        andn %l0, %l3, %i0
        
        CKTHREAD_CTRL_GET_IFLAGS %l4, %l5
!        and %l5, %l3, %l5
 !       or %i0, %l5, %i0        
	or %l5, PSR_PIL_MASK, %l5
        CKTHREAD_CTRL_RESTORE_IFLAGS %l4, %l5
       	jmp %l7+4
	nop

#define KTHREAD_TRAP_PENDING_F (1<<31)
ENTRY(SparcClearPilSys)
!        set PSR_PIL_MASK|PSR_ET_BIT, %l3
!        andn %l0, %l3, %i0        
        CKTHREAD_CTRL_GET_IFLAGS %l4, %l5
!        and %l5, %l3, %l5
!        or %i0, %l5, %i0
	andn %l5, PSR_PIL_MASK, %l5
        CKTHREAD_CTRL_RESTORE_IFLAGS %l4, %l5

        GET_CKTHREAD %l3, %l5
        ld [%l3+_CTRL_OFFSET+_G_OFFSET], %l5
        ld [%l5+_SWTRAP_OFFSET],%l5
        andcc %l5, 1, %l5
        bne 113f
        nop

        ld [%l3+_CTRL_OFFSET+_FLAGS_OFFSET], %l5
        set KTHREAD_TRAP_PENDING_F, %l3
        andcc %l5, %l3, %l3
        bne 113f
        nop

        ld [%l4+_HWIRQSPEND_OFFSET+_VAL_OFFSET], %l5
        ld [%l4+_HWIRQSMASK_OFFSET+_VAL_OFFSET], %l3        
        andncc %l5, %l3, %l3
        bne 113f
        nop

        ld [%l4+_EXTIRQSPEND_OFFSET+_VAL_OFFSET], %l5
        ld [%l4+_EXTIRQSMASK_OFFSET+_VAL_OFFSET], %l3        
        andncc %l5, %l3, %l3
        bne 113f
        nop

        set XM_EXT_TRAPS,%l3
        andcc %l5, %l3, %l3
        bne 113f
        nop

       	jmp %l7+4
	nop

113:
        mov %l2, %l1
        add %l2, 4, %l2
        
       	set _retl, %l4
	set FromIRet, %l5

	jmp %l5
        nop

ENTRY(SparcCtrlWinFlowSys)

        ! Preveting an underflow trap

        rd      %wim, %l5
        add     %l0, 2, %l6     ! cwp + 2     (1 current windows of user + 1) =2
        and     %l6, 0x1f, %l6
        subcc   %l6, CONFIG_REGISTER_WINDOWS, %l4
        bneg,a  1f
        nop

        mov     %l4,%l6
1:
        srl     %l5, %l6, %l6   ! win >> cwp + 2

        cmp     %l6, 1
        bne     2f
        nop

        rd %wim, %l5    ! Instruction in the entry
        sll %l5, 1, %l6 ! Instruction delayed in the entry
        srl %l5, (CONFIG_REGISTER_WINDOWS)-1, %l5
        wr %l5, %l6, %wim
        WR_DELAY

        restore                 ! CWP -> Window saved on the stack
        restore

        RESTORE_CWND %sp

        save                    ! Come back to the trap window
        save                    ! where return address is located

2:
        jmp %l7+4
        nop
        
.previous

.data
.align 4        
__fromWH:
        .long 0
.previous

