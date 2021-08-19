#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCT= nu
BINDIR=	$(DSTROOT)/usr/etc
CONFDIR= ${DSTROOT}/private/etc
SCRIPTDIR= ${DSTROOT}/private/etc/nulib
DSTDIRS= ${ETCDIR} ${CONFDIR} ${SCRIPTDIR}

CDEFS=	-DNeXT_MOD 
CFLAGS=	-O -g $(CDEFS) -ansi
IFLAGS= -s
LIBS= -lsys_s

SHELLFILES= nu1.sh nu2.sh nu3.sh nu4.sh
HFILES= ${PRODUCT}.h piduid.h
YFILES=
CFILES= ${PRODUCT}.c piduid.c
SFILES=

DERIVED=
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

all:	$(PRODUCT)

install: DSTROOT $(DSTDIRS) all symlinks
	install $(IFLAGS) $(PRODUCT).NEW $(BINDIR)/$(PRODUCT)
	install -c -m 555 $(SHELLFILES) $(SCRIPTDIR)
	install -c -m 444 nu.cf.real $(CONFDIR)/nu.cf

symlinks:
	@if [ ! -h ${DSTROOT}/etc ] ; \
	then \
		echo "ln -s private/etc ${DSTROOT}/etc" ; \
		ln -s private/etc ${DSTROOT}/etc ; \
	fi

#
# Building PRODUCT
#

$(PRODUCT).NEW:	$(OBJS) vers.o
	$(CC) $(CFLAGS) -o $@ $(OBJS) vers.o $(LIBS)

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
