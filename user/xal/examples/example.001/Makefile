# XAL_PATH: path to the XTRATUM directory
XAL_PATH=../..

all: container.bin resident_sw
include $(XAL_PATH)/common/rules.mk

# XMLCF: path to the XML configuration file
XMLCF=xm_cf.$(ARCH).xml

# PARTITIONS: partition files (xef format) composing the example
PARTITIONS=partition0.xef partition1.xef

partition0: partition0.o
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(shell $(XPATHSTART) 0 xm_cf.$(ARCH).xml)

partition1: partition1.o
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(shell $(XPATHSTART) 1 xm_cf.$(ARCH).xml)

PACK_ARGS=-h $(XMCORE):xm_cf.xef.xmc \
	-p 0:partition0.xef \
	-p 1:partition1.xef

container.bin: $(PARTITIONS) xm_cf.xef.xmc
	$(XMPACK) check xm_cf.xef.xmc $(PACK_ARGS)
	$(XMPACK) build $(PACK_ARGS) $@
