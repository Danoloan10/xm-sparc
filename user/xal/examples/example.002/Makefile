# XAL_PATH: path to the XTRATUM directory
XAL_PATH=../..

all: container.bin resident_sw
include $(XAL_PATH)/common/rules.mk

# XMLCF: path to the XML configuration file
XMLCF=xm_cf.$(ARCH).xml

# PARTITIONS: partition files (xef format) composing the example
PARTITIONS=partition0.xef partition1.xef reader.xef

partition0: partition.o
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(shell $(XPATHSTART) 0 $(XMLCF))
	
partition1: partition.o
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(shell $(XPATHSTART) 1 $(XMLCF))
	
reader: reader.o
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(shell $(XPATHSTART) 2 $(XMLCF))

PACK_ARGS=-h $(XMCORE):xm_cf.xef.xmc \
	-p 0:partition0.xef \
	-p 1:partition1.xef \
	-p 2:reader.xef
	
container.bin: $(PARTITIONS) xm_cf.xef.xmc
	$(XMPACK) check xm_cf.xef.xmc $(PACK_ARGS)
	$(XMPACK) build $(PACK_ARGS) $@
