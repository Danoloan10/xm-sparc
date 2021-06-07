/*
 * kwatchdog.h
 *
 *  Created on: 16 de mar. de 2016
 *      Author: lithosdev
 */

#ifndef XM_KWATCHDOG_H_
#define XM_KWATCHDOG_H_

#include<arch/arch_types.h>

/*
 * Interface declaration.
 * This interface add a level.
 * A hardware watchdog driver must implement this interface to allow to use the
 * watchdog as the HW watchdog used by the XtratuM watchdog. The interface (or
 * device) must be registered to use the hw watchdog using the
 * KWatchdogRegisterHw function.
 *
 */
typedef struct
{
    /* Reload the hw watchdog with the ´time´ microseconds. */
    xm_s32_t (* HwWatchdogReload)(xmTime_t time);
} hwWatchdogIfc_t;

/*
 * Set the XtratuM Wathdog with a bugdet of ´value´ microseconds.
 * Activate the XtratuM Watchdog if not activated.
 */
extern xm_u32_t KWatchdogSetBudget(const xmTime_t value);

/*
 * Return if the XtratuM Watchdog timer has expired.
 */
extern xm_u32_t IsKWatchdogExpired(void);

/*
 * Register a Hardware Watchdog to be used as HW watchdog for XtratuM watchdog
 */
extern xm_u32_t KWatchdogRegisterHw(hwWatchdogIfc_t *wd);

/*
 * The hardware watchdog is reloaded with the configured value.
 * The configured value is set in the configuration file, this value does not
 * change in run-time.
 */
extern xm_u32_t KWatchdogReloadHw();

/*
 * Initialize KWatchdog with the initial value configured
 */
extern xm_u32_t KWatchdogInit(void);

/*
 * Setup the the XtratuM Watchdog.
 * The Watchdog information is getted from the XtratuM configuration
 */
extern xm_u32_t KWatchdogSetup(void);

#endif /* XM_KWATCHDOG_H_ */
