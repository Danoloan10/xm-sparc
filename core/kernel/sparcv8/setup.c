/*
 * $FILE: setup.c
 *
 * Setting up and starting up the kernel (arch dependent part)
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
  - [02/09/2015:SPR-020915-04] Remove VBOOT macro and boot.h
*/
#include <assert.h>
#include <stdc.h>
#include <physmm.h>

#if defined(CONFIG_MPU) && defined(CONFIG_AT697F)
#define LEON_WPR1_OFFSET 0x0
#define LEON_WPR2_OFFSET 0x4
#define REG_OFF 15
#define REG_MASK 0x3FFF8000
#endif

void SetupArchLocal(xm_s32_t cpuid) {
}

#if defined(CONFIG_MPU) && defined(CONFIG_AT697F)
//Funtion to configure WPR2 in case there is shared memory area (only for AT697F target)
void SetupSharedMemoryArea(void) {
    xm_u32_t sharedMemoryAddress, sharedMemorySize;
    xm_u32_t tag, mask;

    //In case of using shared memory areas, only is allow to have one area
    if (xmcTab.hpv.noSharedMemoryAreas == 1) {
        sharedMemoryAddress = xmcSharedMemAreaTab[xmcTab.hpv.noSharedMemoryAreas-1].startAddr;

        sharedMemorySize = xmcSharedMemAreaTab[xmcTab.hpv.noSharedMemoryAreas-1].size;

        //Configuration of WPR2 with BP = 0
        //Make writable region from (sharedMemoryAddress) to (sharedMemoryAddress + sharedMemorySize)
        mask = ((~(sharedMemorySize-1)) & REG_MASK);
        tag = (sharedMemoryAddress & REG_MASK);
               
        StoreIoReg(LEON_MEMORY_WPR_BASE+LEON_WPR2_OFFSET, (mask>>REG_OFF) | tag);
    }
}
#endif

void EarlySetupArchCommon(void) {
    extern void EarlySetupCpu(void);
    /* There is at least one processor in the system */
    SET_NRCPUS(1);
    EarlySetupCpu();
}

void SetupArchCommon(void) {
}
