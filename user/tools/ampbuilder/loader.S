.text
.align 4
.global _start, addr

#define PCR_CPUID_MASK 0xF0000000
#define IRQMP_STATUS_REG_ADDR 0x80000210
#define IRQMP_CPU1_ON 0x1 << 1

_start:
       rd %asr17, %g1
       set PCR_CPUID_MASK, %g2
       andcc %g1, %g2, %g2
       bne addr
       nop
       add %g0, IRQMP_CPU1_ON, %g5
       set IRQMP_STATUS_REG_ADDR, %g6
       st %g5, [%g6]       
       add %g0, 0x2A, %g3
       set ePoint0, %g5
       ld [%g5], %g5
       jmp %g5
       nop
1:     b 1b
       nop

addr:  
       add %g0, 0x2B, %g4
       set ePoint1, %g5
       ld [%g5], %g5
       jmp %g5
       nop
1:     b 1b
       nop


