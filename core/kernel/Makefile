# Changelog:
# - [24/11/2015:SPR-241115-02] ${USR_RESET_CODE} added to compilation when 
#      JMP_USR_FUNC_COLD_RESET_SYSTEM is defined
all: objs

include ../../xmconfig
include $(XTRATUM_PATH)/config.mk
include $(XTRATUM_PATH)/core/rules.mk
-include dep.mk

SRCS_C-y = irqs.c ktimer.c objdir.c rsvmem.c setup.c hypercalls.c \
	kthread.c panic.c sched.c kwatchdog.c
#xef.c
SRCS_C-${CONFIG_JMP_USR_FUNC_COLD_RESET_SYSTEM} += ${USR_RESET_CODE}
SRCS = $(SRCS_C-y)
OBJS += $(patsubst %.c, %.o, $(SRCS_C-y))


objs: $(OBJS)
	@$(TARGET_LD) $(TARGET_LDFLAGS) -r -o kern.o $(OBJS)

#	$(CC) $(filter-out -pg,$(CFLAGS)) -c gprof.c
