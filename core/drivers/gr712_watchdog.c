/**
 * @file gr712_watchdog.c
 *
 * GR712-RC Watchdog driver
 * The driver implements the XtratuM Watchdog Interface
 *
 * @author Ángel Esquinas (aesquinas@fentiss.com)
 *
 * @copyright
 * Copyright 2016 Fent Innovative Software Solutions (FENTISS). 
 * All rights reserved. Read LICENSE.txt file for the license terms.
 * 
 * Changelog:
 * 
 * [dd/mm/yyyy]     [author]
 * [brief description]
 *
 * 29/03/2016            Ángel Esquinas Fernandez
 * Initial version
 */

#include <stdc.h>
#include <arch/arch_types.h>
#include <arch/leon.h>
#include <kdevice.h>
#include <kwatchdog.h>
#include <autoconf.h>
#include <drivers/gr712_watchdog.h>


static xm_s32_t WatchdogReload(xmTime_t value)
{
    static xmTime_t lastValue = 0;
    static xm_u32_t lastHwValue = 0;

    /* When the value is equal that last value, then use the previous calculated
     * values. It is a pseudo-cache.
     */

    if (value != lastValue)
    {
        /* Calculate the value of the timer based in microseconds */
        if (value > 0xFFFFFFFFUL)
        {
            value = 0xFFFFFFFF;
        }
        lastValue = value;

        /* The timer is set to 1Mhz so the value in us could be used without
         * changes
         */
        lastHwValue = value;
    }

    StoreIoReg(LEON_TIMER4_BASE, lastHwValue); /* Counter value register */

    return 0;
}


static hwWatchdogIfc_t gr712watchdog = {
    .HwWatchdogReload = WatchdogReload,
};


xm_s32_t SetupGR712Wd(void)
{
    /* Increase the HW Watchdog value to ~60 sec at 80Mhz
     * The value should be incremented before the LEON SCALER is set to 1KHZ.
     */
    eprintf("Initializing GR712 Watchdog\n");
    StoreIoReg(LEON_TIMER4_BASE, WD_INITIAL_VALUE);
    StoreIoReg(LEON_TIMER4_BASE + 0x8, (1 << 3) | (1 <<0 ));
    KWatchdogRegisterHw(&gr712watchdog);

    return 0;
}

REGISTER_KDEV_SETUP(SetupGR712Wd);
