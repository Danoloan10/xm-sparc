# XAL_PATH: path to the XTRATUM directory
XAL_PATH=../..

all: container.bin resident_sw
include $(XAL_PATH)/common/rules.mk

# XMLCF: path to the XML configuration file
XMLCF=xm_cf.$(ARCH).xml

# PARTITIONS: partition files (xef format) composing the example
PARTITIONS=partition0.xef partition1.xef partition2.xef

CSRCS := $(sort $(wildcard $(EXTRADIR)/*.c))
ASRCS := $(sort $(wildcard *.S $(EXTRADIR)/*.S))
OBJS := $(patsubst %.c,%.o, $(CSRCS)) $(patsubst %.S,%.o, $(ASRCS))

partition0: partition.o $(EXTRA_OBJS)
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(call xpathstart,0,xm_cf.sparcv8.xml)

partition1: partition.o $(EXTRA_OBJS)
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(call xpathstart,1,xm_cf.sparcv8.xml)

partition2: reader.o $(EXTRA_OBJS)
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(call xpathstart,2,xm_cf.sparcv8.xml)

PACK_ARGS=-h $(XMCORE):xm_cf.xef.xmc \
	-p 0:partition0.xef\
	-p 1:partition1.xef\
	-p 2:partition2.xef\

container.bin: $(PARTITIONS) xm_cf.xef.xmc
	$(XMPACK) build $(PACK_ARGS) $@
	@exec echo -en "> Done [container]\n"
