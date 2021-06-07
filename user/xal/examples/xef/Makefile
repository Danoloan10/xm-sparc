# XAL_PATH: path to the XTRATUM directory
XAL_PATH=../..

all: container.bin resident_sw
include $(XAL_PATH)/common/rules.mk

# XMLCF: path to the XML configuration file
XMLCF=xm_cf.$(ARCH).xml

# PARTITIONS: partition files (xef format) composing the example
PARTITIONS=partition.xef

partition1: partition1.o
	@$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(shell $(XPATHSTART) 1 xm_cf.$(ARCH).xml)

e_xef.o: partition1.xef cf.xef
	echo | $(TARGET_CC) -x c -O2 -c - -o e_xef.o
	$(TARGET_OBJCOPY) --add-section .xef=partition1.xef --set-section-flags .xef=load,alloc --add-section .xefCf=cf.xef --set-section-flags .xefCf=load,alloc e_xef.o e_xef.o

TARGET_LDFLAGS=-n -u start -u xmImageHdr -Tloader.lds -L../../../libxm/ -L../../lib --start-group `sparc-linux-gcc -print-libgcc-file-name -msoft-float` -lxm -lxal --end-group

cf.xef: custom_file.txt
	${XEF} -m -c -o $@ $^

partition: e_xef.o partition.o
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(shell $(XPATHSTART) 0 xm_cf.$(ARCH).xml)

PACK_ARGS=-h $(XMCORE):xm_cf.xef.xmc \
	-p 0:partition.xef

container.bin: $(PARTITIONS) xm_cf.xef.xmc
	$(XMPACK) check xm_cf.xef.xmc $(PACK_ARGS)
	$(XMPACK) build $(PACK_ARGS) $@
