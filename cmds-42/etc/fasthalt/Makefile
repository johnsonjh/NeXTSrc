#
# Generic top-level makefile for shell scripts
# (c) NeXT, Inc.  1987
#
PRODUCT= fasthalt
BINDIR=	$(DSTROOT)/usr/etc
DSTDIRS= $(BINDIR)

IFLAGS= -xs -m 555

all:	$(PRODUCT).NEW

$(PRODUCT).NEW:	$(PRODUCT).sh $(DOVERS)
	sed -e "s/#PROGRAM.*/#`vers_string $(PRODUCT)`/" \
	    <$(PRODUCT).sh >$(PRODUCT).NEW

clean:	ALWAYS
	-rm -f *.NEW

install: DSTROOT $(DSTDIRS) all
	install $(IFLAGS) $(PRODUCT).NEW $(BINDIR)/$(PRODUCT)

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
