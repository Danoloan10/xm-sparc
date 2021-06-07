# XAL_PATH: path to the XTRATUM directory
XAL_PATH=../..

# XMLCF: path to the XML configuration file
XMLCF=xm_cf.sparcv8.xml

# PARTITIONS: partition files (xef format) composing the example
PARTITIONS=partition0.xef partition1.xef

all: container.bin resident_sw
include $(XAL_PATH)/common/rules.mk

# duplicated required to run hello_world
# but should be in ../common/rules.mk
#xpathstart = $(shell $(XPATHSTART) $(1) $(2))

partition0: hello.o
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(call xpathstart,0,xm_cf.$(ARCH).xml)

partition1: hello.o
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(call xpathstart,1,xm_cf.$(ARCH).xml)

PACK_ARGS=-h $(XMCORE):xm_cf.xef.xmc \
	-p 0:partition0.xef \
	-p 1:partition1.xef

container.bin: $(PARTITIONS) xm_cf.xef.xmc
	$(XMPACK) check xm_cf.xef.xmc $(PACK_ARGS)
	$(XMPACK) build $(PACK_ARGS) $@
