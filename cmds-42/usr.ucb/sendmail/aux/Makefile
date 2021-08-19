#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCTS= mconnect mailstats praliases vacation
BINDIR=	$(DSTROOT)/usr/ucb
ETCDIR=	$(DSTROOT)/usr/etc
DSTDIRS= $(BINDIR) $(ETCDIR)

CDEFS=	-DNeXT_MOD -D__STRICT_BSD__ -I../include -I../src -DVMUNIX -DNeXT_NFS
CFLAGS=	-O -g -fwritable-strings $(CDEFS)
IFLAGS= -s -m 755
LIBS= -lsys_s

HFILES=
YFILES=
CFILES= mconnect.c mailstats.c praliases.c vacation.c
SFILES=

DERIVED=
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

all:	$(PRODUCTS)

install: DSTROOT $(DSTDIRS) all
	install ${IFLAGS} mconnect.NEW $(ETCDIR)/mconnect
	install ${IFLAGS} mailstats.NEW $(ETCDIR)/mailstats
	install ${IFLAGS} praliases.NEW $(BINDIR)/praliases
	install ${IFLAGS} vacation.NEW $(BINDIR)/vacation

#
# Building mconnect
#
MCONNECT_OBJS= mconnect.o

mconnect.NEW:	${MCONNECT_OBJS} mconnect_vers.o
	$(CC) $(CFLAGS) -o $@ ${MCONNECT_OBJS} mconnect_vers.o ${LIBS}

mconnect_vers.c: ${DOVERS} ${MCONNECT_OBJS}
	@rm -f $@
	vers_string -c mconnect > $@

#
# Building mailstats
#
MAILSTATS_OBJS= mailstats.o

mailstats.NEW:	${MAILSTATS_OBJS} mailstats_vers.o
	$(CC) $(CFLAGS) -o $@ ${MAILSTATS_OBJS} mailstats_vers.o ${LIBS}

mailstats_vers.c: ${DOVERS} ${MAILSTATS_OBJS}
	@rm -f $@
	vers_string -c mailstats > $@

#
# Building praliases
#
PRALIASES_OBJS= praliases.o

praliases.NEW:	${PRALIASES_OBJS} praliases_vers.o
	$(CC) $(CFLAGS) -o $@ ${PRALIASES_OBJS} praliases_vers.o ${LIBS} -ldbm

praliases_vers.c: ${DOVERS} ${PRALIASES_OBJS}
	@rm -f $@
	vers_string -c praliases > $@

#
# Building vacation
#
VACATION_OBJS= vacation.o convtime.o

convtime.o: ../src/convtime.c
	${CC} ${CFLAGS} -c ../src/convtime.c

vacation.NEW:	${VACATION_OBJS} vacation_vers.o
	$(CC) $(CFLAGS) -o $@ ${VACATION_OBJS} vacation_vers.o ${LIBS} -ldbm

vacation_vers.c: ${DOVERS} ${VACATION_OBJS}
	@rm -f $@
	vers_string -c vacation > $@


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
