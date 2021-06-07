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

#ifndef GR712_WATCHDOG_H_
#define GR712_WATCHDOG_H_

/*
 * The GR712-RC board after a reset initializes the Timer 4 (Watchdog) to
 * 0xFFFF, where the scaler is initialized to 0xFFFF (~55 secs). Where the scaler is set to
 * 1KHZ the initial value of the HW watchdog would be exhausted in less than a
 * second, so it is neccesary to reload the Watchdog with a bigger value
 */

#define WD_INITIAL_VALUE 0x3FFFFFF /* ~60 sec at 80Mhz processor */

#endif /* GR712_WATCHDOG_H_ */
