all: resident_sw 
EXAMPLES_PATH = ..
include $(EXAMPLES_PATH)/common/rules.mk

-include dep.mk

SRCS := $(sort $(wildcard *.c)) 				   #@ \label{general-start}
ASRCS := $(sort $(wildcard *.S))
OBJS := $(patsubst %.c,%.o, $(SRCS)) $(patsubst %.S,%.o, $(ASRCS)) #@ \label{general-end}

#TARGET_LDFLAGS+=--emit-relocs
partition1: hello.o #$(OBJS)
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(call xpathstart,0,xm_cf.$(ARCH).xml)

partition2: hello.o #$(OBJS)
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(call xpathstart,1,xm_cf.$(ARCH).xml)

#partition3: $(OBJS)
#	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(call xpathstart,3,xm_cf.$(ARCH).xml)

#partition4: $(OBJS)
#	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(call xpathstart,4,xm_cf.$(ARCH).xml)

PACK_ARGS=-h $(XMCORE):xm_cf.xef.xmc \
        -p 0:partition1.xef \
	-p 1:partition2.xef

PARTITIONS=partition1.xef partition2.xef
#partition3.xef partition4.xef
container.bin: $(PARTITIONS) xm_cf.xef.xmc
	$(XMPACK) check xm_cf.xef.xmc $(PACK_ARGS)
	$(XMPACK) build $(PACK_ARGS) $@

container.elf: container.bin
	./cont2elf ../../bootloaders/rsw/.config $< $@
