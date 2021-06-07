/*
 * $FILE: makefile.root.h
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

#ifndef _MAKEFILE_ROOT_H_
#define _MAKEFILE_ROOT_H_

char makefileRootFile[]="all: build_system\n"
"include config.mk\n"
"include $(XTRATUM_PATH)/xmconfig\n"
"include $(XTRATUM_PATH)/config.mk\n"
"include $(XTRATUM_LIB_PATH)/rules.mk\n"
"include rules.mk\n\n"

"PARTITIONS=%s\n\n"

"XTRATUM_CORE=$(XTRATUM_LIB_PATH)/xm_core.bin\n\n"

"XMPACK_BUILD_OPT=-h $(XTRATUM_CORE):xm_cf.bin.xmc %s\n\n"

"build_system: xm_cf.bin.xmc\n"
	"\t@echo \"Building system\"\n"
	"\t@for partition in $(PARTITIONS) ; do \\\n"
		"\t\techo $$partition ; \\\n"
		"\t\t$(MAKE) -s -C $$partition all ; \\\n"
                "\t\tcp $$partition/$$partition.bin ./ ; \\\n"
                "\t\tcp $$partition/$$partition.cfg ./ ; \\\n"
		"\t\tif [ \"$$?\" -ne 0 ]; then exit 1 ; fi \\\n"
	"\tdone\n"
    	"\t@$(XTRATUM_BIN_PATH)/xmpack build $(XMPACK_BUILD_OPT) container.bin\n"
	"\t@$(XTRATUM_BIN_PATH)/rswbuild container.bin resident_sw\n"
        "\t@echo \"Done\"\n\n"

"clean:\n"
	"\t@for partition in $(PARTITIONS) ; do \\\n"
		"\t\techo $$partition ; \\\n"
		"\t\t$(MAKE) -s -C $$partition clean ; \\\n"
		"\t\tif [ \"$$?\" -ne 0 ]; then exit 1 ; fi \\\n"
	"\tdone\n"
        "\t@rm -f *.o *~ *.xmc container.bin resident_sw\n";

#endif
