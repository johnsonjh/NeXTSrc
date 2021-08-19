#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCTS= implog implogd
BINDIR=	$(DSTROOT)/usr/etc
DSTDIRS= $(BINDIR)

CDEFS=	-DNeXT_MOD -D__STRICT_BSD__
CFLAGS=	-O -g -fwritable-strings $(CDEFS)
IFLAGS= -s
LIBS= -lsys_s

HFILES=
CFILES= implog.c implogd.c
SFILES=
YFILES=

DERIVED=
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

all:	$(PRODUCTS)

install: DSTROOT $(DSTDIRS) all
	install ${IFLAGS} implog.NEW $(BINDIR)/implog
	install ${IFLAGS} implogd.NEW $(BINDIR)/implogd

#
# Building implog
#
IMPLOG_OBJS= implog.o

implog.NEW:	${IMPLOG_OBJS} implog_vers.o
	$(CC) $(CFLAGS) -o $@ ${IMPLOG_OBJS} implog_vers.o ${LIBS}

implog_vers.c: ${DOVERS} ${IMPLOG_OBJS}
	@rm -f $@
	vers_string -c implog > $@

#
# Building implogd
#
IMPLOGD_OBJS= implogd.o

implogd.NEW:	${IMPLOGD_OBJS} implogd_vers.o
	$(CC) $(CFLAGS) -o $@ ${IMPLOGD_OBJS} implogd_vers.o ${LIBS}

implogd_vers.c: ${DOVERS} ${IMPLOGD_OBJS}
	@rm -f $@
	vers_string -c implogd > $@

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
