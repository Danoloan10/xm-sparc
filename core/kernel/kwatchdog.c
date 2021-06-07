/*
 * kwatchdog.c
 *
 * $FILE: hypercalls.h
 *
 * Processor-related hypercalls definition
 *
 * Author: Angel Esquinas <aesquinas@fentiss.com>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 * [16/03/2016]     Ángel Esquinas (aesquinas@fentiss.com)
 * CP-250216-01: Initial version
 *
 */

#include<kwatchdog.h>
#include<stdc.h>
#include<ktimer.h>
#include<xmconf.h>

#define true 1
#define false 0


static struct {
    xmTime_t hwReload;
    xmTime_t initValue;
} wdConfig;

/*
 * XXX: The XtratuM Watchdog is only valid in monocore systems.
 *
 * Only one XtratuM Watchdog is defined for whole system. The XM WD is based in
 * the XM hardware timer.
 */

static hwWatchdogIfc_t *hwWatchdog = NULL; /* HW watchdog interface */
static kTimer_t wdTimer;                /* Timer for XM Watchdog */

/* The XtratuM Wachdog is deactivate initially. So it is expired */
static xm_u32_t kWdIsExpired = true;


static void KWatchdogTimerHandler(kTimer_t *wd, void *args)
{
    kWdIsExpired = true;
}

static void GetKWatchdogConfInfo(void)
{
    wdConfig.hwReload = (xmTime_t) xmcTab.hpv.watchdog.hwWdReloadValue;
    wdConfig.initValue = (xmTime_t) xmcTab.hpv.watchdog.wdInitialValue;
}


/*
 * Return if the XtratuM Watchdog timer has expired.
 */
xm_u32_t IsKWatchdogExpired(void)
{
    return kWdIsExpired;
}


/*
 * Reload the KWatchDog.
 * This function is called from the XM hypercall that reload the XM Watchdog.
 */
xm_u32_t KWatchdogSetBudget(const xmTime_t value)
{
    xmTime_t absTime = 0;

    if (value == 0)
    {
        kWdIsExpired = true;
        DisarmKTimer(&wdTimer);
    }
    else
    {
        absTime = GetSysClockUsec();
        kWdIsExpired = false;
        ArmKTimer(&wdTimer, absTime + value, 0);
    }
    return 0;
}


/*
 * KWatchdog Reload hardware watchdog if watchdog associated.
 */
xm_u32_t KWatchdogReloadHw()
{
    xm_u32_t res = 0;
    if(hwWatchdog != NULL && hwWatchdog->HwWatchdogReload != NULL)
    {
        /* TODO: Includes the reload value */
        hwWatchdog->HwWatchdogReload(wdConfig.hwReload);
    }

    return res;
}


xm_u32_t KWatchdogRegisterHw(hwWatchdogIfc_t* wd)
{
    xm_u32_t ret = -1;

    if (wd != NULL)
    {
        hwWatchdog = wd;
        ret = 0;
    }
    return ret;
}


/*
 * Initialize KWatchdog
 */
xm_u32_t KWatchdogInit(void)
{
    KWatchdogSetBudget(wdConfig.initValue);
    return 0;
}


/*
 * Setup KWatchdog
 */
xm_u32_t KWatchdogSetup(void)
{
    GetKWatchdogConfInfo();
    InitKTimer(0, &wdTimer, KWatchdogTimerHandler, NULL, NULL);

    return 0;
}
