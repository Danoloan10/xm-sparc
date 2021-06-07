ifeq ($(EXAMPLES_PATH)/common/config.mk, $(wildcard $(EXAMPLES_PATH)/common/config.mk))
is_installed := 1
include $(EXAMPLES_PATH)/common/config.mk
else
XTRATUM_PATH=../../../
endif


# early detect of misconfiguration and missing variables
$(if $(XTRATUM_PATH),, \
	$(warning "The configuration variable XTRATUM_PATH is not set,") \
	$(error "check the \"common/mkconfig.dist\" file (see README)."))

$(if $(EXAMPLES_PATH),, \
	$(warning "The configuration variable EXAMPLES_PATH is not set,") \
	$(error "check the \"common/mkconfig.dist\" file (see README)."))

LIBXM=xm
LIBXEF=xef

include $(XTRATUM_PATH)/xmconfig
include $(XTRATUM_PATH)/version
include $(EXAMPLES_PATH)/$(ARCH)/rules_$(ARCH).mk

ifdef is_installed
include $(XTRATUM_PATH)/lib/rules.mk
XMCORE_BIN=$(XTRATUM_PATH)/lib/xm_core.bin
XMCORE=$(XTRATUM_PATH)/lib/xm_core.xef
export PATH := $(XTRATUM_PATH)/bin:$(PATH)
else
include $(XTRATUM_PATH)/user/rules.mk
XMCORE_BIN=$(XTRATUM_PATH)/core/xm_core.bin
XMCORE=$(XTRATUM_PATH)/core/xm_core.xef
export PATH := $(XTRATUM_PATH)/user/bin:$(PATH)
endif

XMPACK=xmpack
BUILD_XMC=xmcbuild
XEF=xmeformat build -c

COMMON_PATH = $(EXAMPLES_PATH)/common
TARGET_CFLAGS += -I$(EXAMPLES_PATH)/include
TARGET_CFLAGS += -fno-builtin


TARGET_LDFLAGS += -u start -u xmImageHdr -T$(EXAMPLES_PATH)/lib/loader.lds\
	-L$(EXAMPLES_PATH)/lib -L$(LIBXM_PATH) \
	--start-group $(LIBGCC) -l$(LIBXM) -lexamples --end-group

# function usage: $(call xpathstart,partitionid,xmlfile)
xpathstart = $(shell $(EXAMPLES_PATH)/bin/xpathstart $(1) $(2))

%.xef:  %
	$(XEF) $< -o $@

%.xef.xmc: %.bin.xmc
	$(XEF) -m $< -o $@

xm_cf.bin.xmc: xm_cf.$(ARCH).xml
	xmcparser -o $@ $^

xm_cf.c.xmc: xm_cf.$(ARCH).xml
	xmcparser -c -o $@ $^

resident_sw: container.bin
	rswbuild $^ $@

distclean: clean
	@$(RM) $(ARCH_PATH)/*.o $(ARCH_PATH)/*~ $(COMMON_PATH)/*.o $(COMMON_PATH)/*~
	@find -name "*.o" -exec rm '{}' \;
	@find -name "*~" -exec rm '{}' \;
	@find -name "dep.mk" -exec rm '{}' \;

clean:
	@$(RM) $(PARTITIONS) $(patsubst %.bin,%, $(PARTITIONS)) $(patsubst %.xef,%, $(PARTITIONS)) container.bin resident_sw xm_cf xm_cf.bin xm_cf.*.xmc
	@$(RM) *.o *~ *.*.xmc dep.mk
