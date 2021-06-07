all: libs

include ../../xmconfig
include $(XTRATUM_PATH)/config.mk
include $(XTRATUM_PATH)/user/rules.mk

EXAMPLES_PATH	= $(XTRATUM_PATH)/user/examples
COMMON_PATH	= $(EXAMPLES_PATH)/common
ARCH_PATH	= $(EXAMPLES_PATH)/$(ARCH)

TARGET_CFLAGS += -I$(EXAMPLES_PATH)/include -I$(ARCH_PATH) 

TARG=lib/libexamples.a
OBJS=\
	$(patsubst %.c,%.o, $(wildcard $(COMMON_PATH)/*.c))\
	$(patsubst %.S,%.o, $(wildcard $(ARCH_PATH)/*.S))\

libs: $(TARG)
$(TARG): $(OBJS)
	@$(TARGET_AR) cr $@ $^
	@cp $(ARCH)/loader.lds lib

clean:
	@$(RM) $(TARG)
	@$(RM) $(ARCH_PATH)/*.o $(COMMON_PATH)/*.o
	@$(RM) lib/loader.lds
distclean: clean
	@find -name "*.o" -exec rm '{}' \;
	@find -name "*~" -exec rm '{}' \;
	@find -name "dep.mk" -exec rm '{}' \;
