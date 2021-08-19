#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCTS= spell spellin spellout spell.xx hlist hlista hlistb hstop
BINDIR=	$(DSTROOT)/usr/bin
LIBDIR=	$(DSTROOT)/usr/lib
DICTDIR= $(DSTROOT)/usr/dict
DSTDIRS= $(BINDIR) $(LIBDIR) $(DICTDIR)

CDEFS=	-DNeXT_MOD -D__STRICT_BSD__ -Dunix
CFLAGS=	-O -g -fwritable-strings $(CDEFS)
LDFLAGS= -object
IFLAGS= -s
LIBS= -lsys_s

HFILES=	spell.h
CFILES= spell.c spellin.c spellout.c
SFILES=
YFILES=

DERIVED=
GARBAGE= *_vers.c hlist hlista hlistb hstop lhlist lhlista lhlistb

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

all:	$(PRODUCTS)

install: DSTROOT $(DSTDIRS) all
	install ${IFLAGS} spell.NEW ${LIBDIR}/spell
	install ${IFLAGS} spellout.NEW ${BINDIR}/spellout
	install ${IFLAGS} spellin.NEW ${BINDIR}/spellin
	install -c -m 644 words ${DICTDIR}/words
	install -c -m 644 hlist.NEW ${DICTDIR}/hlist
	install -c -m 644 hlista.NEW ${DICTDIR}/hlista
	install -c -m 644 hlistb.NEW ${DICTDIR}/hlistb
	install -c -m 644 hstop.NEW ${DICTDIR}/hstop
	install -c -m 755 -xs spell.xx.NEW ${BINDIR}/spell


#
# Building spell
#
SPELL_OBJS= spell.o

spell.NEW:	${SPELL_OBJS} spell_vers.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ ${SPELL_OBJS} spell_vers.o ${LIBS}

spell_vers.c: ${DOVERS} ${SPELL_OBJS}
	@rm -f $@
	vers_string -c spell > $@

#
# Building spellin
#
SPELLIN_OBJS= spellin.o

spellin.NEW:	${SPELLIN_OBJS} spellin_vers.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ ${SPELLIN_OBJS} spellin_vers.o ${LIBS}

spellin_vers.c: ${DOVERS} ${SPELLIN_OBJS}
	@rm -f $@
	vers_string -c spellin > $@

#
# Building spellout
#
SPELLOUT_OBJS= spellout.o

spellout.NEW:	${SPELLOUT_OBJS} spellout_vers.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ ${SPELLOUT_OBJS} spellout_vers.o ${LIBS}

spellout_vers.c: ${DOVERS} ${SPELLOUT_OBJS}
	@rm -f $@
	vers_string -c spellout > $@

#
# Building hlist
#
hlist.NEW: words spellin.NEW
	@rm -f hlist hlist.NEW
	./spellin.NEW <words >hlist.NEW

#
# Building local lhlist
#
lhlist.NEW: words spellin.NEW
	@rm -f lhlist lhlist.NEW
	./spellin.NEW <words >lhlist.NEW

#
# Building hlista
#
hlista.NEW: american localwords hlist.NEW spellin.NEW
	@rm -f hlista hlista.NEW
	(cat american localwords)|./spellin.NEW hlist.NEW >hlista.NEW

#
# Building local lhlista
#
lhlista.NEW: american localwords lhlist.NEW spellin.NEW
	@rm -f lhlista lhlista.NEW
	(cat american localwords)|./spellin.NEW lhlist.NEW >lhlista.NEW

#
# Building hlistb
#
hlistb.NEW: british localwords hlist.NEW spellin.NEW
	@rm -f hlistb hlistb.NEW
	(cat british localwords)|./spellin.NEW hlist.NEW >hlistb.NEW

#
# Building local lhlistb
#
lhlistb.NEW: british localwords lhlist.NEW spellin.NEW
	@rm -f lhlistb lhlistb.NEW
	(cat british localwords)|./spellin.NEW lhlist.NEW >lhlistb.NEW

#
# Building hstop
#
hstop.NEW: stop spellin.NEW
	@rm -f hstop hstop.NEW
	./spellin.NEW <stop >hstop.NEW

#
# Building spell.xx
#
spell.xx.NEW:	spell.sh $(DOVERS)
	sed -e "s/#PROGRAM.*/#`vers_string $(PRODUCT)`/" \
	    <spell.sh >spell.xx.NEW

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
