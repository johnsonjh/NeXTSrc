#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCT= neqn
BINDIR=	$(DSTROOT)/usr/bin
DSTDIRS= $(BINDIR)

CDEFS=	-DNeXT_MOD -D__STRICT_BSD__ -DNEQN -I. -I../eqn/
CFLAGS=	-O -g -fwritable-strings $(CDEFS)
YFLAGS= -d
IFLAGS= -s
LIBS= -lsys_s

HFILES=
YFILES= ../eqn/e.y
CFILES= ../eqn/diacrit.c ../eqn/eqnbox.c ../eqn/font.c ../eqn/fromto.c \
	../eqn/funny.c ../eqn/glob.c ../eqn/integral.c ../eqn/io.c \
	../eqn/lex.c ../eqn/lookup.c ../eqn/mark.c ../eqn/matrix.c \
	../eqn/move.c ../eqn/over.c ../eqn/paren.c ../eqn/pile.c \
	../eqn/shift.c ../eqn/size.c ../eqn/sqrt.c ../eqn/text.c
SFILES=

DERIVED= e.c
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= diacrit.o eqnbox.o font.o fromto.o funny.o glob.o integral.o \
	io.o lex.o lookup.o mark.o matrix.o move.o over.o paren.o \
	pile.o shift.o size.o sqrt.o text.o

all:	$(PRODUCT)

install: DSTROOT $(DSTDIRS) all
	install $(IFLAGS) $(PRODUCT).NEW $(BINDIR)/$(PRODUCT)

#
# Building PRODUCT
#

e.c:		../eqn/e.y
	yacc $(YFLAGS) ../eqn/e.y
	mv y.tab.c e.c

$(OBJS):	../eqn/$$(@:.o=.c)
	$(CC) $(CFLAGS) -c ../eqn/$(@:.o=.c)

$(PRODUCT).NEW:	$(OBJS) e.o vers.o
	$(CC) $(CFLAGS) -o $@ $(OBJS) e.o vers.o $(LIBS)

vers.c: $(DOVERS) $(OBJS)
	@rm -f $@
	vers_string -c $(PRODUCT) > $@

#
# Standard targets
#
$(PRODUCT):	Makedep $$@.NEW

clean:	ALWAYS
	-rm -f *.o *.NEW TAGS tags vers.c Makedep $(PRODUCT) $(DERIVED) \
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
