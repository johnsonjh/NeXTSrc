#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCTS= learn lrntee lcount
BINDIR=	$(DSTROOT)/usr/bin
LIBDIR=	$(DSTROOT)/usr/lib/learn
LLIBDIR= ${LIBDIR}/bin
DSTDIRS= $(BINDIR) $(LIBDIR) $(LLIBDIR)

CDEFS=	-DNeXT_MOD -D__STRICT_BSD__ -DBSD4_2
CFLAGS=	-O -g -fwritable-strings $(CDEFS)
IFLAGS= -s
LIBS= -lsys_s

HFILES=	lrnref.h
CFILES= copy.c dounit.c getlesson.c lcount.c learn.c list.c lrntee.c \
	makpipe.c maktee.c mem.c mysys.c selsub.c selunit.c start.c tee.c \
	whatnow.c wrapup.c
SFILES=
YFILES=

DERIVED=
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

all:	$(PRODUCTS)

install: DSTROOT $(DSTDIRS) all
	install -c ${IFLAGS} learn.NEW $(BINDIR)/learn
	install ${IFLAGS} learn.NEW $(LLIBDIR)/learn
	install ${IFLAGS} lrntee.NEW $(LLIBDIR)/lrntee
	install ${IFLAGS} lcount.NEW $(LLIBDIR)/lcount

#
# Building learn
#
LEARN_OBJS= copy.o dounit.o getlesson.o learn.o list.o mem.o makpipe.o \
	maktee.o mysys.o selsub.o selunit.o start.o whatnow.o wrapup.o

learn.NEW:	${LEARN_OBJS} learn_vers.o
	$(CC) $(CFLAGS) -o $@ ${LEARN_OBJS} learn_vers.o ${LIBS}

learn_vers.c: ${DOVERS} ${LEARN_OBJS}
	@rm -f $@
	vers_string -c learn > $@

#
# Building lrntee
#
LRNTEE_OBJS= lrntee.o

lrntee.NEW:	${LRNTEE_OBJS} lrntee_vers.o
	$(CC) $(CFLAGS) -o $@ ${LRNTEE_OBJS} lrntee_vers.o ${LIBS}

lrntee_vers.c: ${DOVERS} ${LRNTEE_OBJS}
	@rm -f $@
	vers_string -c lrntee > $@

#
# Building lcount
#
LCOUNT_OBJS= lcount.o

lcount.NEW:	${LCOUNT_OBJS} lcount_vers.o
	$(CC) $(CFLAGS) -o $@ ${LCOUNT_OBJS} lcount_vers.o ${LIBS}

lcount_vers.c: ${DOVERS} ${LCOUNT_OBJS}
	@rm -f $@
	vers_string -c lcount > $@

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
