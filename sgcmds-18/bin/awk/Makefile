#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCT= awk
BINDIR=	$(DSTROOT)/bin
DSTDIRS= $(BINDIR)

CDEFS=	-DNeXT_MOD -D__STRICT_BSD__
CFLAGS=	-O -g -fwritable-strings $(CDEFS)
YFLAGS=	-d
IFLAGS= -s
LIBS= -lsys_s -lm

HFILES=
YFILES=	awk.g.y
LFILES=	awk.lx.l
#
# Take out malloc(), calloc(), realloc() and free() multiple 
# definitions from Makefile.  This is so awk can link with the shlib's.
#
#CFILES= b.c lib.c main.c parse.c freeze.c run.c token.c tran.c \
#        malloc.c calloc.c
#CFILES= b.c lib.c main.c parse.c freeze.c run.c token.c tran.c
CFILES= b.c lib.c main.c parse.c freeze.c run.c token.c tran.c
FILES=

DERIVED= y.tab.h proctab.c
GARBAGE= token.c.bak temp

SRCS= ${HFILES} ${YFILES} ${LFILES} ${CFILES} ${SFILES} proc.c
OBJS= $(LFILES:.l=.o) $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o) proctab.o

all:	$(PRODUCT)

install: DSTROOT $(DSTDIRS) all
	install $(IFLAGS) $(PRODUCT).NEW $(BINDIR)/$(PRODUCT)

#
# Building PRODUCT
#

y.tab.h:	awk.g.o

awk.h:	y.tab.h
	-cmp -s y.tab.h awk.h || cp y.tab.h awk.h

token.c:	awk.h
	rm -f temp
	mv token.c token.c.bak
	cp token.c.bak token.c
	ex - <tokenscript
	rm -f temp

proctab.c:	proc
	./proc >proctab.c

proc:	proc.o token.o
	cc -o proc proc.o token.o


$(PRODUCT).NEW:	$(OBJS) vers.o
	$(CC) $(CFLAGS) -o $@ $(OBJS) vers.o $(LIBS)

malloc.o:
	$(CC) -DNeXT_MOD -O -g -fwritable-strings -c malloc.c


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
