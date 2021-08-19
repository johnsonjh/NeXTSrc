#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCTS= restore.old rrestore.old
BINDIR=	$(DSTROOT)/usr/etc
DSTDIRS= $(BINDIR)

CDEFS=	-DNeXT_MOD -DNeXT_NFS -D__STRICT_BSD__
CFLAGS=	-O -g -fwritable-strings $(CDEFS)
IFLAGS= -s
LIBS= -lsys_s

HFILES=	restore.h
CFILES=	main.c interactive.c restore.c dirs.c symtab.c tape.c utilities.c
SFILES=
YFILES=

DERIVED=
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES) ../dump.old/dumprmt.c
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

all:	$(PRODUCTS)

install: DSTROOT $(DSTDIRS) all
	install ${IFLAGS} restore.old.NEW $(BINDIR)/restore.old
	install ${IFLAGS} rrestore.old.NEW $(BINDIR)/rrestore.old

#
# Building restore
#
RESTORE_OBJS= main.o interactive.o restore.o dirs.o symtab.o utilities.o \
	tape.o

restore.old.NEW:	${RESTORE_OBJS} restore_vers.o
	$(CC) $(CFLAGS) -o $@ ${RESTORE_OBJS} restore_vers.o ${LIBS}

restore_vers.c: ${DOVERS} ${RESTORE_OBJS}
	@rm -f $@
	vers_string -c restore > $@

#
# Building rrestore
#
RRESTORE_OBJS= main.o interactive.o restore.o dirs.o symtab.o \
	utilities.o rtape.o dumprmt.o

rtape.o: tape.c
	cp tape.c rtape.c
	$(CC) $(CFLAGS) -c -DRRESTORE rtape.c
	rm -f rtape.c

dumprmt.o: ../dump/dumprmt.c
	$(CC) $(CFLAGS) -c ../dump/dumprmt.c

rrestore.old.NEW:	${RRESTORE_OBJS} rrestore_vers.o
	$(CC) $(CFLAGS) -o $@ ${RRESTORE_OBJS} rrestore_vers.o ${LIBS}

rrestore_vers.c: ${DOVERS} ${RRESTORE_OBJS}
	@rm -f $@
	vers_string -c rrestore > $@

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
