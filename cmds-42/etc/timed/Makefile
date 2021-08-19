#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
# optional flags are: MEASURE TESTING DEBUG
#
PRODUCTS= timed timedc
BINDIR=	$(DSTROOT)/usr/etc
DSTDIRS= $(BINDIR)

CDEFS=	-DNeXT_MOD -D__STRICT_BSD__
CFLAGS=	-O -g -fwritable-strings $(CDEFS)
LDFLAGS= -object
TIMED_IFLAGS= -s
TIMEDC_IFLAGS= -s -o root -m 4511
LIBS= -lsys_s

HFILES=
YFILES=
CFILES= $(PRODUCT).c
SFILES=

DERIVED=
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

all:	$(PRODUCTS)

install: DSTROOT $(DSTDIRS) all
	install ${TIMED_IFLAGS} timed.NEW $(BINDIR)/timed
	install ${TIMEDC_IFLAGS} timedc.NEW $(BINDIR)/timedc

#
# Building timed
#
TIMED_OBJS= acksend.o candidate.o correct.o master.o networkdelta.o \
	readmsg.o slave.o timed.o byteorder.c measure.c cksum.o ocsum.o

timed.NEW:	${TIMED_OBJS} timed_vers.o
	$(CC) $(CFLAGS) -o $@ ${TIMED_OBJS} timed_vers.o ${LIBS}

timed_vers.c: ${DOVERS} ${TIMED_OBJS}
	@rm -f $@
	vers_string -c timed > $@

#
# Building timedc
#
TIMEDC_OBJS= cmds.o cmdtab.o timedc.o byteorder.o measure.o cksum.o ocsum.o

timedc.NEW:	${TIMEDC_OBJS} timedc_vers.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ ${TIMEDC_OBJS} timedc_vers.o ${LIBS}

timedc_vers.c: ${DOVERS} ${TIMEDC_OBJS}
	@rm -f $@
	vers_string -c timedc > $@

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
