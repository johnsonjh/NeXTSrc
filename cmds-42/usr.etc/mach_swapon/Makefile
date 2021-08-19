#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCT= mach_swapon
BINDIR=	$(DSTROOT)/usr/etc
ETCDIR= $(DSTROOT)/private/etc
DSTDIRS= $(BINDIR)

CDEFS=
CFLAGS=	-O -g $(CDEFS)
#CDEFS= -I/os/king/MK/mk
#CFLAGS=	-g $(CDEFS)
LDFLAGS= -object
IFLAGS= -s
LIBS= -lsys_s

HFILES= swent.h
YFILES=
CFILES= ${PRODUCT}.c swent.c
SFILES= 

DERIVED=
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

#
# Special rules until the new mach_swapon syscall makes it into the
# libraries.
#
.s.o:
	cc -E $*.s > $*.as
	as -o $*.o $*.as
	rm -f $*.as

all:	$(PRODUCT) swaptab

install: DSTROOT $(DSTDIRS) all
	install $(IFLAGS) $(PRODUCT).NEW $(BINDIR)/$(PRODUCT)
	install -c swaptab $(ETCDIR)/swaptab

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
$(PRODUCT):	Makedep $$@.NEW

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

depend Makedep: $(DERIVED)
	rm -f Makedep
	$(CC) -M $(CDEFS) $(CFILES) $(SFILES) $(DERIVED) | \
	    awk '{ if ($$1 != prev) { if (rec != "") print rec; \
		rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		else rec = rec " " $$2 } } \
		END { print rec }' >Makedep

tags:	ALWAYS
	etags $(SRCS)
	ctags $(SRCS)

ALWAYS:

-include Makedep
