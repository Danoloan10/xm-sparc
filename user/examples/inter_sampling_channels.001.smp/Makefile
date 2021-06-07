all: resident_sw 
EXAMPLES_PATH = ..
include $(EXAMPLES_PATH)/common/rules.mk

-include dep.mk

#TARGET_LDFLAGS+=--emit-relocs
partition1: writer.o
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(call xpathstart,5,xm_cf.$(ARCH).xml)

partition2: reader.o
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(call xpathstart,8,xm_cf.$(ARCH).xml)


PACK_ARGS=-h $(XMCORE):xm_cf.xef.xmc \
        -p 5:partition1.xef \
        -p 8:partition2.xef \

PARTITIONS=partition1.xef partition2.xef

container.bin: $(PARTITIONS) xm_cf.xef.xmc
	$(XMPACK) check xm_cf.xef.xmc $(PACK_ARGS)
	$(XMPACK) build $(PACK_ARGS) $@

container.elf: container.bin
	./cont2elf ../../bootloaders/rsw/.config $< $@
