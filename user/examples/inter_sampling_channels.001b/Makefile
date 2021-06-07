all: resident_sw 
EXAMPLES_PATH = ..
include $(EXAMPLES_PATH)/common/rules.mk

-include dep.mk

#TARGET_LDFLAGS+=--emit-relocs
partition0: dummy.o
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(call xpathstart,1,xm_cf.$(ARCH).xml)

partition1: reader.o
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(call xpathstart,2,xm_cf.$(ARCH).xml)


PACK_ARGS=-h $(XMCORE):xm_cf.xef.xmc \
        -p 0:partition0.xef \
        -p 1:partition1.xef \

PARTITIONS=partition0.xef partition1.xef

container.bin: $(PARTITIONS) xm_cf.xef.xmc
	$(XMPACK) check xm_cf.xef.xmc $(PACK_ARGS)
	$(XMPACK) build $(PACK_ARGS) $@

container.elf: container.bin
	./cont2elf ../../bootloaders/rsw/.config $< $@
