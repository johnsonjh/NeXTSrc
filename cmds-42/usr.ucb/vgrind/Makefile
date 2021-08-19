#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCTS= vfontedpr retest vgrind
BINDIR=	$(DSTROOT)/usr/ucb
LIBDIR=	$(DSTROOT)/usr/lib
MACDIR= $(DSTROOT)/usr/lib/tmac
DSTDIRS= $(BINDIR) $(LIBDIR) $(MACDIR)

CDEFS=	-DNeXT_MOD -D__STRICT_BSD__
CFLAGS=	-O -g -fwritable-strings $(CDEFS)
IFLAGS= -s
LIBS= -lsys_s

HFILES=
CFILES= regexp.c retest.c vfontedpr.c vgrindefs.c
SFILES=
YFILES=

DERIVED=
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

all:	$(PRODUCTS)

install: DSTROOT $(DSTDIRS) all
	install ${IFLAGS} vfontedpr.NEW $(LIBDIR)/vfontedpr
	install -c -m 755 vgrind.NEW $(BINDIR)/vgrind
	install -c vgrindefs.src $(LIBDIR)/vgrindefs
	install -c tmac.vgrind $(MACDIR)/tmac.vgrind

#
# Building vfontedpr
#
VFONTEDPR_OBJS= vfontedpr.o vgrindefs.o regexp.o

vfontedpr.NEW:	${VFONTEDPR_OBJS} vfontedpr_vers.o
	$(CC) $(CFLAGS) -o $@ ${VFONTEDPR_OBJS} vfontedpr_vers.o ${LIBS}

vfontedpr_vers.c: ${DOVERS} ${VFONTEDPR_OBJS}
	@rm -f $@
	vers_string -c vfontedpr > $@

#
# Building retest
#
RETEST_OBJS= retest.o regexp.c

retest.NEW:	${RETEST_OBJS} retest_vers.o
	$(CC) $(CFLAGS) -o $@ ${RETEST_OBJS} retest_vers.o ${LIBS}

retest_vers.c: ${DOVERS} ${RETEST_OBJS}
	@rm -f $@
	vers_string -c retest > $@

#
# Building vgrind
#
vgrind.NEW: vgrind.sh ${DOVERS}
	sed -e "s/#PROGRAM.*/#`vers_string vgrind`/" \
	<vgrind.sh >vgrind.NEW


#
# Standard targets
#
$(PRODUCTS):	Makedep $$@.NEW

clean:	ALWAYS
	-rm -f *.o *.NEW TAGS tags vers.c Makedep $(PRODUCTS) $(DERIVED) \
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
