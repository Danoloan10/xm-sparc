/*
 * $FILE: makefile.partition.h
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

#ifndef _MAKEFILE_PARTITION_H_
#define _MAKEFILE_PARTITION_H_

char makefilePartitionFile[]="all: %s.bin\n"
"include ../config.mk\n\n"
"include ../rules.mk\n"
"include $(XTRATUM_PATH)/xmconfig\n"
"include $(XTRATUM_PATH)/config.mk\n"
"include $(XTRATUM_LIB_PATH)/rules.mk\n\n"

"-include dep.mk\n\n"

"SRCS := $(sort $(wildcard *.c) $(wildcard *.S))\n"
"OBJS := $(patsubst %%.c,%%.o, $(SRCS))\n"
"OBJS := $(patsubst %%.S,%%.o, $(OBJS))\n\n"

"%s: $(OBJS)\n"
    "\t$(LD) -o $@ $^ $(LDFLAGS) -Tpartition.lds  -L$(LIBXM_PATH) --start-group $(LIBGCC) -lxm --end-group\n\n"

"clean:\n"
	"\t$(RM) -f *.o *~\n";

#endif
