/*
 * $FILE: rules.mk.h
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

#ifndef _RULES_MK_ROOT_H_
#define _RULES_MK_ROOT_H_

char rulesMkRootFile[]="BUILD_XMC=$(XTRATUM_BIN_PATH)/xmcbuild\n\n"

"%%.bin:  %%\n"
        "\t$(OBJCOPY) -O binary $< $@\n\n"

"%%.bin.xmc: %%.c.xmc\n"
        "\t$(BUILD_XMC) $< $@\n\n"

"xm_cf.c.xmc: xm_cf.$(ARCH).xml\n"
        "\t$(XTRATUM_BIN_PATH)/xmcparser -o $@ $^\n"
        "\t$(XTRATUM_BIN_PATH)/memplot -f $^\n\n"

"xm_cf: xm_cf.xmc.o\n"
        "\t$(LD) $(LDFLAGS_ARCH) -o $@ $^ -e 0x0 -Tdata 0x0\n";

#endif
