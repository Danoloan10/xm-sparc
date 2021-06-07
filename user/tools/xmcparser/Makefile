TOOLS=xmcparser
#xmproject
#.PHONY: $(TOOLS)
all: $(TOOLS)

INSTALL=$(TOOLS)

include ../../../xmconfig

include $(XTRATUM_PATH)/config.mk
include $(XTRATUM_PATH)/user/rules.mk
include $(XTRATUM_PATH)/user/install.mk
LIBXM_PATH=$(XTRATUM_PATH)/user/libxm

-include dephost.mk

HOST_CFLAGS += `xml2-config --cflags` -D_GNU_SOURCE -D$(ARCH) -I./ --include xm_inc/config.h --include xm_inc/arch/arch_types.h --include $(XTRATUM_PATH)/user/bootloaders/rsw/include/autoconf.h
HOST_LDFLAGS += `xml2-config --libs` -L$(LIBXM_PATH) -lxmhost

xmc.xsd: xmc.xsd.in
	@$(TARGET_CC) $(TARGET_ASFLAGS) -x assembler-with-cpp -E -P $< -o $@.bl
	@sed "/^$$/d" $@.bl > $@

#LEX=flex
#YACC=bison

#lex.yy.c: xmc.l xmc.tab.h
#	$(LEX) xmc.l

#xmc.tab.h xmc.tab.c: xmc.y
#	$(YACC) -d xmc.y

SRCS=main.c process_xml.c xmc.c cgen.c hm.c conv.c checks.c xmcbuild.c $(ARCH).c
#cgen.c hm.c datatab.c dataproc.c estring.c $(ARCH).c genproject.c
OBJS=main.host.o process_xml.host.o xmc.host.o cgen.host.o hm.host.o conv.host.o checks.host.o xmcbuild.host.o $(ARCH).host.o rsvmem.host.o

xmcparser: xmc.xsd  $(OBJS) sxsd.host.o exsd.host.o xmc.xsd
	@exec echo "" | $(HOST_CC) -x c -c - -o tmp.host.o
	@$(HOST_OBJCOPY) --add-section .rodata.xsd=xmc.xsd tmp.host.o
	$(HOST_CC) -o xmcparser $(HOST_CFLAGS) $(OBJS) sxsd.host.o tmp.host.o exsd.host.o  $(HOST_LDFLAGS)

OBJS_PROJECT=main_xmproject.host.o genproject.host.o process_xml.host.o xmc.host.o hm.host.o conv.host.o checks.host.o $(ARCH).host.o

xmproject: $(OBJS_PROJECT) sxsd.host.o exsd.host.o xmc.xsd \
	prj-skel/boot.sparcv8.S.h  \
	prj-skel/config.mk.h  \
	prj-skel/lds.h  \
	prj-skel/main.c.h  \
	prj-skel/makefile.partition.h  \
	prj-skel/makefile.root.h  \
	prj-skel/rules.mk.h  \
	prj-skel/traps.c.h \
	prj-skel/partition.cfg.h
	$(HOST_CC) -o xmproject $(HOST_CFLAGS) $(OBJS_PROJECT) sxsd.host.o tmp.host.o exsd.host.o $(HOST_LDFLAGS)

clean:
	@$(RM) -f *.o *~ *.tab.c *.tab.h *.yy.c $(TOOLS) dephost.mk xmcparser.3 xmcparser.tex xmc.xsd

DATE:=$(shell LANG=C date "+%B %Y")

xmcparser.3: xmcparser.pod
	pod2man  --section=3  --center="XtratuM tools" --release=XtratuM-$(XM_VERSION).$(XM_SUBVERSION).$(XM_REVISION)  --date="${DATE}" $< $@

xmcparser.tex: xmcparser.pod
	pod2latex -h1level 3  $< $@
