#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCT=su

#
# Standard 4.3 version of su
#
PRODUCT_NOWHEEL= su.nowheel

#
# NeXT version of su
#
PRODUCT_WHEEL= su.wheel

#
# Default su used in release
#
PRODUCT_DEFAULT= su.nowheel

BINDIR=	$(DSTROOT)/bin
DSTDIRS= $(BINDIR)

CDEFS=
CFLAGS=	-O -g $(CDEFS)

IFLAGS= -o root -g wheel -m 755 -s
LDFLAGS= -object
LIBS= -lsys_s

HFILES=
YFILES=
CFILES= $(PRODUCT).c
SFILES=

DERIVED=
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

.c.o:
	$(CC) -MD $(CFLAGS) $(COPTS) -c $<
	md -d -u Makedep $*.d

all:	$(PRODUCT_WHEEL) $(PRODUCT_NOWHEEL)

install: DSTROOT $(DSTDIRS) all
	install $(IFLAGS) $(PRODUCT_NOWHEEL).NEW $(BINDIR)/$(PRODUCT_NOWHEEL)
	install $(IFLAGS) $(PRODUCT_WHEEL).NEW $(BINDIR)/$(PRODUCT_WHEEL)
	cd $(BINDIR); rm -f $(PRODUCT); ln -s $(PRODUCT_DEFAULT) $(PRODUCT);
	chmod u+s $(BINDIR)/$(PRODUCT_DEFAULT)

#
# Building PRODUCT
#

$(PRODUCT_WHEEL).NEW:	$(PRODUCT_WHEEL).o vers.o
	$(CC) ${LDFLAGS} $(CFLAGS) -o $@ $(PRODUCT_WHEEL).o vers.o $(LIBS)

$(PRODUCT_NOWHEEL).NEW:	$(PRODUCT_NOWHEEL).o vers.o
	$(CC) ${LDFLAGS} $(CFLAGS) -o $@ $(PRODUCT_NOWHEEL).o vers.o $(LIBS)

vers.c: $(DOVERS) $(PRODUCT_WHEEL).o $(PRODUCT_NOWHEEL).o
	@rm -f $@
	vers_string -c $(PRODUCT) > $@

#
# Standard targets
#

$(PRODUCT_WHEEL):	$(PRODUCT_WHEEL).NEW

$(PRODUCT_NOWHEEL):	$(PRODUCT_NOWHEEL).NEW

$(PRODUCT_WHEEL).o: $(PRODUCT).c
	$(CC) $(CFLAGS) -DWHEEL -c $(PRODUCT).c -o $@

$(PRODUCT_NOWHEEL).o: $(PRODUCT).c
	$(CC) $(CFLAGS) -c $(PRODUCT).c -o $@

clean:	ALWAYS
	-rm -f *.o *.NEW TAGS tags vers.c Makedep $(PRODUCT) $(DERIVED) \
		$(GARBAGE)

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

tags:	ALWAYS
	etags $(SRCS)
	ctags $(SRCS)

ALWAYS:

-include Makedep
