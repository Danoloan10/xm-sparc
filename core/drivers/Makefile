#SRCS := $(sort $(wildcard *.c))
#OBJS := $(patsubst %.c,%.o,$(wildcard *.c))
#LOBJS := $(OBJS)

all: objs mods

include ../../xmconfig
include $(XTRATUM_PATH)/config.mk
include $(XTRATUM_PATH)/core/rules.mk
-include dep.mk

srcs-y := kdevice.c

ifdef CONFIG_SPARCv8
srcs-$(CONFIG_DEV_UART) += leon_uart.c
endif

srcs-$(CONFIG_LICE) += lice_interface.c
srcs-$(CONFIG_DEV_FPGA_ITARFREE) += fpga_itarfree.c

srcs-$(CONFIG_SPW_RTC) += spw_irq.c sdp.c

srcs-$(CONFIG_DEV_MEMBLOCK) += memblock.c

srcs-$(CONFIG_DEV_MIL_STD) += mil-std.c 

srcs-$(CONFIG_GR712_WATCHDOG) += gr712_watchdog.c

SRCS := $(srcs-y) $(srcs-m)
OBJS := $(patsubst %.c,%.o,$(srcs-y))
LOBJS := $(OBJS)

OBJS-M := $(patsubst %.c,%.xo,$(srcs-m))

objs: $(OBJS)
	$(TARGET_LD) $(TARGET_LDFLAGS) -r -o drivers.o $(LOBJS)

mods: $(OBJS-M)
