#SRCS := $(sort $(wildcard *.c))
#SRCS := console.c hm.c  mem.c  status.c  trace.c
#OBJS := $(patsubst %.c,%.o,$(wildcard *.c))
#LOBJS := $(OBJS)

all: objs mods

include ../../xmconfig
include $(XTRATUM_PATH)/config.mk
include $(XTRATUM_PATH)/core/rules.mk
-include dep.mk

srcs-y := console.c hm.c trace.c mem.c status.c

ifdef CONFIG_AMP_SUPPORT
srcs-y += commports_ex.c
else
srcs-y += commports.c
endif

SRCS := $(srcs-y) $(srcs-m)
OBJS := $(patsubst %.c,%.o,$(srcs-y))
LOBJS := $(OBJS)

OBJS-M := $(patsubst %.c,%.xo,$(srcs-m))

objs: $(OBJS)
	@$(TARGET_LD) $(TARGET_LDFLAGS) -r -o objects.o $(LOBJS)

mods: $(OBJS-M)
