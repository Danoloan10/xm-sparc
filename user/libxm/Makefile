# Changelog
# - [02/09/2015:SPR-120515-03] Removed common/objdesc.c from SRCS
all: libs

include ../../xmconfig
include $(XTRATUM_PATH)/config.mk
include $(XTRATUM_PATH)/user/rules.mk

-include dep.mk
-include dephost.mk

HOST_CFLAGS += --include xm_inc/config.h --include xm_inc/arch/arch_types.h

SRCS = common/comm.c common/hypercalls.c common/init.c common/trace.c common/hm.c common/hypervisor.c common/status.c $(ARCH)/hypercalls.c $(ARCH)/init.c common/xef.c common/lzss.c common/md5.c common/container.c

OBJS = $(patsubst %.c,%.o, $(filter %.c, $(SRCS))) $(patsubst %.S,%.o, $(filter %.S, $(SRCS)))

OBJS = $(patsubst %.c,%.o, $(filter %.c, $(SRCS))) $(patsubst %.S,%.o, $(filter %.S, $(SRCS)))

HOST_SRCS = common/md5.c common/lzss.c common/xef.c
HOST_OBJS = $(patsubst %.c,%.host.o, $(HOST_SRCS))

LIBXM=libxm.a libxmhost.a

libs: $(LIBXM)

libxm.a: $(OBJS)
	@$(TARGET_AR) -s -c -r libxm.a $(OBJS)

libxmhost.a: $(HOST_OBJS)
	@$(HOST_AR) -s -c -r libxmhost.a $(HOST_OBJS)

distclean: clean

clean:
	@$(RM) -f *.o *.a *.o *.host.o *~ include/*~ include/arch/*~ $(ARCH)/*~ $(ARCH)/*.o common/*.o common/*~ dep.mk dephost.mk
