#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCT= passwd
BINDIR=	$(DSTROOT)/bin
DSTDIRS= $(BINDIR)

CDEFS=
CFLAGS=	-O -g $(CDEFS)
LDFLAGS= -object
IFLAGS= -o root -m 4755 -s
LIBS= -lsys_s

HFILES=
YFILES=
CFILES= ${PRODUCT}.c getpwent.c
SFILES=

DERIVED=
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

.c.o:
	$(CC) -MD $(CFLAGS) $(COPTS) -c $<
	md -d -u Makedep $*.d

all:	$(PRODUCT)

#
# Need to get chsh and chfn working post-1.0.
#
install: DSTROOT $(DSTDIRS) all
	install ${IFLAGS} ${PRODUCT}.NEW $(BINDIR)/${PRODUCT}
	rm -f $(BINDIR)/chfn $(BINDIR)/chsh

#
# Building PRODUCT
#

$(PRODUCT).NEW:	$(OBJS) vers.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS) vers.o $(LIBS)

vers.c: $(DOVERS) $(OBJS)
	@rm -f $@
	vers_string -c $(PRODUCT) > $@

#
# Standard targets
#
$(PRODUCT):	$$@.NEW

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
