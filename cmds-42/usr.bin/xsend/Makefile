#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCTS= enroll xget xsend
BINDIR=	$(DSTROOT)/usr/bin
DSTDIRS= $(BINDIR)

CDEFS=	-DNeXT_MOD -D__STRICT_BSD__
CFLAGS=	-O -g -fwritable-strings $(CDEFS)
IFLAGS= -s
LIBS= -lsys_s -lmp

HFILES=	xmail.h
CFILES= enroll.c lib.c xget.c xsend.c
SFILES=
YFILES=

DERIVED=
GARBAGE= *_vers.c

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

all:	$(PRODUCTS)

install: DSTROOT $(DSTDIRS) all
	install ${IFLAGS} enroll.NEW $(BINDIR)/enroll
	install ${IFLAGS} xget.NEW $(BINDIR)/xget
	install ${IFLAGS} xsend.NEW $(BINDIR)/xsend

#
# Building enroll
#
ENROLL_OBJS= enroll.o lib.o

enroll.NEW:	${ENROLL_OBJS} enroll_vers.o
	$(CC) $(CFLAGS) -o $@ ${ENROLL_OBJS} enroll_vers.o ${LIBS}

enroll_vers.c: ${DOVERS} ${ENROLL_OBJS}
	@rm -f $@
	vers_string -c enroll > $@

#
# Building xsend
#
XSEND_OBJS= xsend.o lib.o

xsend.NEW:	${XSEND_OBJS} xsend_vers.o
	$(CC) $(CFLAGS) -o $@ ${XSEND_OBJS} xsend_vers.o ${LIBS}

xsend_vers.c: ${DOVERS} ${XSEND_OBJS}
	@rm -f $@
	vers_string -c xsend > $@

#
# Building xget
#
XGET_OBJS= xget.o lib.o

xget.NEW:	${XGET_OBJS} xget_vers.o
	$(CC) $(CFLAGS) -o $@ ${XGET_OBJS} xget_vers.o ${LIBS}

xget_vers.c: ${DOVERS} ${XGET_OBJS}
	@rm -f $@
	vers_string -c xget > $@

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
