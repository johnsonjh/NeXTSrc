#
# Generic top-level makefile for shell scripts
# (c) NeXT, Inc.  1987
#
PRODUCTS= ypinit ypxfr_1perday ypxfr_2perday ypxfr_1perhour make.script
BINDIR=	$(DSTROOT)/private/etc/yp
DSTDIRS= $(BINDIR)

IFLAGS= -xc -m 555

all:	$(PRODUCTS)

$(PRODUCTS):	$$@.NEW

ypinit.NEW:	ypinit.sh $(DOVERS)
	sed -e "s/#PROGRAM.*/#`vers_string ypinit`/" \
	    <ypinit.sh >ypinit.NEW

ypxfr_1perday.NEW:	ypxfr_1perday.sh $(DOVERS)
	sed -e "s/#PROGRAM.*/#`vers_string ypxfr_1perday`/" \
	    <ypxfr_1perday.sh >ypxfr_1perday.NEW

ypxfr_2perday.NEW:	ypxfr_2perday.sh $(DOVERS)
	sed -e "s/#PROGRAM.*/#`vers_string ypxfr_2perday`/" \
	    <ypxfr_2perday.sh >ypxfr_2perday.NEW

ypxfr_1perhour.NEW:	ypxfr_1perhour.sh $(DOVERS)
	sed -e "s/#PROGRAM.*/#`vers_string ypxfr_1perhour`/" \
	    <ypxfr_1perhour.sh >ypxfr_1perhour.NEW

make.script.NEW:	make.script.Make $(DOVERS)
	sed -e "s/#PROGRAM.*/#`vers_string make.script`/" \
	    <make.script.Make >make.script.NEW

clean:	ALWAYS
	-rm -f *.NEW

install: DSTROOT $(DSTDIRS) all
	install $(IFLAGS) ypinit.NEW $(BINDIR)/ypinit
	install $(IFLAGS) ypxfr_1perday.NEW $(BINDIR)/ypxfr_1perday
	install $(IFLAGS) ypxfr_2perday.NEW $(BINDIR)/ypxfr_2perday
	install $(IFLAGS) ypxfr_1perhour.NEW $(BINDIR)/ypxfr_1perhour
	install $(IFLAGS) make.script.NEW $(BINDIR)/Makefile

DSTROOT:
	@if [ -n "$($@)" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

$(DSTDIRS):
	mkdirs $@

depend tags ALWAYS:
