# XAL_PATH: path to the XTRATUM directory
XAL_PATH=../..

all: gen_xmlcf container.bin resident_sw
include $(XAL_PATH)/common/rules.mk

# XMLCF: path to the XML configuration file
XMLCF=xm_cf.$(ARCH).xml

XMLCF_CFLAGS = $(patsubst  --include xm_inc/arch/arch_types.h,, $(TARGET_CFLAGS))

#  PARTITIONS: partition files (xef format) composing the example
PARTITIONS=partition0.xef partition1.xef partition2.xef

gen_xmlcf:
	rm -rf $(XMLCF) # Eliminate old XM_CF
	gcc $(XMLCF_CFLAGS) -x assembler-with-cpp -E -P xm_cf.sparcv8.xml.base > $(XMLCF) # preprocesing the XM_CF 
	sed -i '/^$$/d' $(XMLCF) # Eliminate blank lines from XM_CF

partition0: partition0.o
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(shell $(XPATHSTART) 0 $(XMLCF))
	
partition1: partition1.o
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(shell $(XPATHSTART) 1 $(XMLCF))
	
partition2: partition2.o
	$(TARGET_LD) -o $@ $^ $(TARGET_LDFLAGS) -Ttext=$(shell $(XPATHSTART) 2 $(XMLCF))

PACK_ARGS=-h $(XMCORE):xm_cf.xef.xmc \
	-p 0:partition0.xef \
	-p 1:partition1.xef \
	-p 2:partition2.xef
	
container.bin: $(PARTITIONS) xm_cf.xef.xmc
	$(XMPACK) check xm_cf.xef.xmc $(PACK_ARGS)
	$(XMPACK) build $(PACK_ARGS) $@
