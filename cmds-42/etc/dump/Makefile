#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCTS= dump rdump
BINDIR=	$(DSTROOT)/usr/etc
DSTDIRS= $(BINDIR)

CDEFS=	-DNeXT_MOD -DNeXT_NFS
CFLAGS=	-O -g -fwritable-strings -bsd $(CDEFS)
DUMP_IFLAGS= -s -m 2755 -g tty
RDUMP_IFLAGS= -s -m 6755 -g tty
LIBS= -lsys_s

HFILES=	dump.h
CFILES= dumpitime.c dumpmain.c dumpoptr.c dumprmt.c dumptape.c \
	dumptraverse.c unctime.c swent.c
SFILES=
YFILES=

DERIVED=
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

all:	$(PRODUCTS)

install: DSTROOT ${DSTDIRS} all
	install ${DUMP_IFLAGS} dump.NEW $(BINDIR)/dump
	install ${RDUMP_IFLAGS} rdump.NEW $(BINDIR)/rdump

#
# Building dump
#
DUMP_OBJS= dumpitime.o dumpmain.o dumpoptr.o dumptape.o dumptraverse.o \
	unctime.o disktab.o swent.o

dump.NEW:	${DUMP_OBJS} dump_vers.o
	$(CC) $(CFLAGS) -o $@ ${DUMP_OBJS} dump_vers.o ${LIBS}

dump_vers.c: ${DOVERS} ${DUMP_OBJS}
	@rm -f $@
	vers_string -c dump > $@

#
# Building rdump
#
RDUMP_OBJS= dumpitime.o dumprmain.o dumpoptr.o dumprmt.o dumprtape.o \
	dumptraverse.o unctime.o disktab.o swent.o

rdump.NEW:	${RDUMP_OBJS} rdump_vers.o
	$(CC) $(CFLAGS) -o $@ ${RDUMP_OBJS} rdump_vers.o ${LIBS}

rdump_vers.c: ${DOVERS} ${RDUMP_OBJS}
	@rm -f $@
	vers_string -c rdump > $@

dumprmain.o: dumpmain.c
	$(CC) $(CFLAGS) -DRDUMP -c dumpmain.c -o dumprmain.o

dumprtape.o: dumptape.c
	$(CC) $(CFLAGS) -DRDUMP -c dumptape.c -o dumprtape.o
	

#
# Standard targets
#
$(PRODUCTS):	Makedep $$@.NEW

clean:	ALWAYS
	-rm -f *.o *.NEW TAGS tags *_vers.c Makedep $(PRODUCTS) $(DERIVED) \
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
