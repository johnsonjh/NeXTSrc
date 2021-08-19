#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCTS= at atrm atq atrun
BINDIR=	$(DSTROOT)/usr/bin
LIBDIR=	$(DSTROOT)/usr/lib
SPOOLDIR= $(DSTROOT)/private/spool/at
PASTDIR= $(DSTROOT)/private/spool/at/past
SPOOLFILE= lasttimedone
DSTDIRS= $(BINDIR) $(LIBDIR)
DSTSPOOLDIRS= $(SPOOLDIR) $(PASTDIR)

CDEFS=	-DNeXT_MOD -D__STRICT_BSD__
CFLAGS=	-O -g -fwritable-strings $(CDEFS)
LDFLAGS= -object
IFLAGS= -s -o daemon -m 4755
LIBS= -lsys_s

HFILES=
YFILES=
CFILES= at.c atrm.c atq.c atrun.c
SFILES=

DERIVED=
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

all:	$(PRODUCTS)

install: DSTROOT ${DSTDIRS} ${DSTSPOOLDIRS} $(SPOOLDIR)/${SPOOLFILE} symlinks \
	all
	install -s atrun.NEW $(LIBDIR)/atrun
	install ${IFLAGS} atq.NEW $(BINDIR)/atq
	install ${IFLAGS} at.NEW $(BINDIR)/at
	install ${IFLAGS} atrm.NEW $(BINDIR)/atrm

symlinks:
	@if [ ! -h ${DSTROOT}/usr/spool ] ; \
	then \
		echo "ln -s ../private/spool ${DSTROOT}/usr/spool" ; \
		ln -s ../private/spool ${DSTROOT}/usr/spool ; \
	fi

#
# Building at
#
AT_OBJS= at.o

at.NEW:	${AT_OBJS}
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(AT_OBJS) $(LIBS)

#
# Building atq
#
ATQ_OBJS= atq.o

atq.NEW:	${ATQ_OBJS}
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(ATQ_OBJS) $(LIBS)

#
# Building atrm
#
ATRM_OBJS= atrm.o

atrm.NEW:	${ATRM_OBJS}
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(ATRM_OBJS) $(LIBS)

#
# Building atrun
#
ATRUN_OBJS= atrun.o

atrun.NEW:	${ATRUN_OBJS}
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(ATRUN_OBJS) $(LIBS)

#
# Standard targets
#
$(PRODUCTS):	Makedep $$@.NEW

clean:	ALWAYS
	-rm -f *.o *.NEW TAGS tags Makedep $(PRODUCTS) $(DERIVED) \
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

${DSTSPOOLDIRS}:
	mkdirs $@
	chown daemon.daemon $@
	chmod 755 $@

${SPOOLDIR}/${SPOOLFILE}: $(SPOOLDIR)
	echo 0 > $@
	chown daemon.daemon $@

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
