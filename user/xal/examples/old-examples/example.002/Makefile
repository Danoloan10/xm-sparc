# XAL_PATH: path to the XTRATUM directory
XAL_PATH=../..

# XMLCF: path to the XML configuration file
XMLCF=xm_cf.sparcv8.xml

# PARTITIONS: partition files (xef format) composing the example
PARTITIONS=partition1.xef partition2.xef partition3.xef partition4.xef

all: container.bin resident_sw
include $(XAL_PATH)/common/rules.mk

partition1: timers.o
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(call xpathstart,1,$(XMLCF))

partition2: timers.o
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(call xpathstart,2,$(XMLCF))

partition3: counter.o
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(call xpathstart,3,$(XMLCF))

partition4: counter.o
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(call xpathstart,4,$(XMLCF))

PACK_ARGS=-h $(XMCORE):xm_cf.xef.xmc \
	-p 0:partition1.xef\
	-p 1:partition2.xef\
	-p 2:partition3.xef\
	-p 3:partition4.xef

container.bin: $(PARTITIONS) xm_cf.xef.xmc
	$(XMPACK) build $(PACK_ARGS) $@
	@exec echo -en "> Done [container]\n"
