# XAL_PATH: path to the XAL directory
XAL_PATH=../..

# XMLCF: path to the XML configuration file
XMLCF=xm_cf.sparcv8.xml

# PARTITIONS: partition files (xef format) composing the example
PARTITIONS=partition0.xef partition1.xef partition2.xef partition3.xef partition4.xef partition5.xef partition6.xef

all: container.bin resident_sw
include $(XAL_PATH)/common/rules.mk

partition0: reader.o
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(call xpathstart,1,$(XMLCF))

partition1: counter.o
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(call xpathstart,2,$(XMLCF))

partition2: counter.o
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(call xpathstart,3,$(XMLCF))

partition3: counter.o
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(call xpathstart,4,$(XMLCF))

partition4: counter.o
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(call xpathstart,5,$(XMLCF))

partition5: counter.o
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(call xpathstart,6,$(XMLCF))

partition6: counter.o
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(call xpathstart,7,$(XMLCF))

PACK_ARGS=-h $(XMCORE):xm_cf.xef.xmc \
	-p 0:partition0.xef\
	-p 1:partition1.xef\
	-p 2:partition2.xef\
	-p 3:partition3.xef\
	-p 4:partition4.xef\
	-p 5:partition5.xef\
	-p 6:partition6.xef\

container.bin: $(PARTITIONS) xm_cf.xef.xmc
	$(XMPACK) build $(PACK_ARGS) $@
	@exec echo -en "> Done [container]\n"
