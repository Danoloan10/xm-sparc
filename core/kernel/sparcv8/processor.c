/*
 * $FILE: processor.c
 *
 * Processor
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
  - [02/09/2015:SPR-020915-01] MISRA rules.9.1 violation fixed.
  - [02/09/2015:SPR-020915-04] Remove VBOOT macro and boot.h
  - [18/11/2015:SPR-221015-01] InitEDAC added
*/
#include <assert.h>
#include <linkage.h>
#include <kdevice.h>
#include <processor.h>
#include <physmm.h>
#include <sched.h>
#include <stdc.h>
#include <xmconf.h>
#ifdef CONFIG_LEON_EDAC_SUPPORT
#include <sparcv8/leon_edac.h>
#endif
#include <arch/xm_def.h>

#if defined(CONFIG_LEON2)||defined(CONFIG_LEON3)||defined(CONFIG_LEON3FT)
RESERVE_IOPORTS(LEON_MEMORY_CFG_BASE, 3);
#endif
#if defined(CONFIG_LEON2)||defined(CONFIG_LEON3)
RESERVE_IOPORTS(LEON_MEMORY_WPR_BASE, 2);
#endif

#ifdef CONFIG_LEON2
RESERVE_IOPORTS(LEON_PCR_BASE, 1);
RESERVE_IOPORTS(LEON_FAILAR_BASE, 1);
RESERVE_IOPORTS(LEON_FAILSR_BASE, 1);
RESERVE_IOPORTS(LEON_CCR_BASE, 1);
RESERVE_IOPORTS(LEON_IDLE_BASE, 1);
RESERVE_IOPORTS(LEON_WATCHDOG_BASE, 1);
#endif

#define SCAR_REG 0x4

xm_u32_t GetCpuKhz(void) {
#if defined(CONFIG_LEON3)||defined(CONFIG_LEON4)||defined(CONFIG_LEON3FT)
    xm_u32_t pFreq=(LoadIoReg(LEON_TIMER_CFG_BASE+SCAR_REG)+1)*1000;

    if (xmcTab.hpv.cpuTab[GET_CPU_ID()].freq==XM_CPUFREQ_AUTO)
        return pFreq;

    if (xmcTab.hpv.cpuTab[GET_CPU_ID()].freq!=pFreq) {
        PWARN("XMC freq (%dKhz) mismatches hw detected (%dKhz)\n", xmcTab.hpv.cpuTab[GET_CPU_ID()].freq, pFreq);
    }
#endif
    return xmcTab.hpv.cpuTab[GET_CPU_ID()].freq;
}

#define PCR_CPUID_MASK 0xF0000000   
#define PCR_CPUID_SHIFT 28

xm_u32_t __GetCpuHwId(void) {
    xm_u32_t asr17 = 0U;
    __asm__ __volatile__("rd %%asr17, %0\n\t"
                         : "=r"(asr17) :);
    return (asr17&PCR_CPUID_MASK)>>PCR_CPUID_SHIFT;
}

xm_u32_t __GetCpuId(void) {
#ifdef CONFIG_SMP_SUPPORT
    return  __GetCpuHwId();
#else
    return 0;
#endif
}

void SetupCpu(void) {
    xm_u32_t cache=0U;
#ifdef CONFIG_ENABLE_CACHE
    cache=DCACHE|ICACHE;
#endif
    SetCacheState(cache);

#if !defined(CONFIG_MMU) && defined(CONFIG_WA_PROTECT_MMAP_PREG_WPOINT)
#define WA_3G_AND       0xC0000000
#define WA_3G_XOR       0x80000000
#define WA_PROM_AND     0x10000000
#define WA_PROM_XOR     0x10000000
#define NO_WATCHPOINT   0x51da
#define CONF_DSU_REG    0x90000000
    if ((xmcTab.hpv.cpuTab[0].features&XM_CPU_FEATURE_WA1)) {
        __asm__ __volatile__ ("set "TO_STR(WA_3G_AND)", %%g1\n\t" \
                              "wr %%g1, %%asr25\n\t" \
                              "set "TO_STR(WA_3G_XOR)", %%g1\n\t" \
                              "wr %%g1, %%asr24\n\t" \
                              "set "TO_STR(WA_PROM_AND)", %%g1\n\t" \
                              "wr %%g1, %%asr27\n\t" \
                              "set "TO_STR(WA_PROM_XOR)", %%g1\n\t" \
                              "wr %%g1, %%asr26\n\t" \
                              "nop;nop;nop\n\t"::: "g1" );
#ifndef CONFIG_CPU_ITAR_FREE_SIM
        __asm__ __volatile__ ("set "TO_STR(CONF_DSU_REG)", %%g2\n\t"\
                                "set "TO_STR(NO_WATCHPOINT)", %%g1\n\t"\
                                "st %%g1, [%%g2]\n\t"\
                              "nop;nop;nop\n\t"::: "g1" );
#endif
    }    
#endif

#ifdef CONFIG_SMP_SUPPORT
    SetupLocalSmp();
#endif
#ifdef CONFIG_LEON_EDAC_SUPPORT
    InitEDAC();
#endif
}

void EarlySetupCpu(void) {
    cpuKhz=GetCpuKhz();
}

