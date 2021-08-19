#
# Generic top-level makefile for shell scripts
# (c) NeXT, Inc.  1987
#
PRODUCT= which
BINDIR=	$(DSTROOT)/usr/ucb
DSTDIRS= $(BINDIR)

IFLAGS= -xc -m 555

all:	$(PRODUCT).NEW

$(PRODUCT).NEW:	$(PRODUCT).csh $(DOVERS)
	sed -e "s/#PROGRAM.*/#`vers_string $(PRODUCT)`/" \
	    <$(PRODUCT).csh >$(PRODUCT).NEW

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
