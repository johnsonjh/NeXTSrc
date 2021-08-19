#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCTS= find bigram code
BINDIR=	$(DSTROOT)/usr/bin
LIBDIR=	$(DSTROOT)/usr/lib/find
DSTDIRS= $(BINDIR) $(LIBDIR)

CDEFS=	-DNeXT_MOD -D__STRICT_BSD__
CFLAGS=	-O -g -fwritable-strings $(CDEFS)
IFLAGS= -s
LIBS= -lsys_s

HFILES=
YFILES=
CFILES= find.c bigram.c code.c
SFILES=

DERIVED=
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

all:	$(PRODUCTS)

install: DSTROOT $(DSTDIRS) all
	install ${IFLAGS} find.NEW $(BINDIR)/find
	install ${IFLAGS} code.NEW $(LIBDIR)/code
	install ${IFLAGS} bigram.NEW $(LIBDIR)/bigram
	install -c -m 755 updatedb.csh $(LIBDIR)/updatedb

#
# Building find
#
FIND_OBJS= find.o

find.NEW:	${FIND_OBJS} find_vers.o
	$(CC) $(CFLAGS) -o $@ ${FIND_OBJS} find_vers.o ${LIBS}

find_vers.c: ${DOVERS} ${FIND_OBJS}
	@rm -f $@
	vers_string -c find > $@

#
# Building bigram
#
BIGRAM_OBJS= bigram.o

bigram.NEW:	${BIGRAM_OBJS} bigram_vers.o
	$(CC) $(CFLAGS) -o $@ ${BIGRAM_OBJS} bigram_vers.o ${LIBS}

bigram_vers.c: ${DOVERS} ${BIGRAM_OBJS}
	@rm -f $@
	vers_string -c bigram > $@

#
# Building code
#
CODE_OBJS= code.o

code.NEW:	${CODE_OBJS} code_vers.o
	$(CC) $(CFLAGS) -o $@ ${CODE_OBJS} code_vers.o ${LIBS}

code_vers.c: ${DOVERS} ${CODE_OBJS}
	@rm -f $@
	vers_string -c code > $@

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
