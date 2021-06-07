/*
 * $FILE: config.mk.h
 *
 * Generates the project skeleton
 *
 * $VERSION$
 *
 * Authors: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _CONFIG_MK_ROOT_H_
#define _CONFIG_MK_ROOT_H_

char configMkRootFile[]="XTRATUM_PATH=%s\n"
    "XTRATUM_LIB_PATH=$(XTRATUM_PATH)/lib\n"
    "XTRATUM_BIN_PATH=$(XTRATUM_PATH)/user/bin\n";

#endif
