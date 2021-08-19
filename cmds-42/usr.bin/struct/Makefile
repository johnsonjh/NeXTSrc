#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCTS= structure beautify
BINDIR=	$(DSTROOT)/usr/bin
LIBDIR=	$(DSTROOT)/usr/lib/struct
DSTDIRS= $(BINDIR) $(LIBDIR)

CDEFS=	-DNeXT_MOD -D__STRICT_BSD__
CFLAGS=	-O -g -fwritable-strings $(CDEFS)
YFLAGS= -d
IFLAGS= -s
LIBS= -lsys_s
BEAUTY_LIBS= -lsys_s -lln

HFILES=	1.defs.h 1.incl.h 2.def.h 3.def.h 4.def.h b.h def.h
CFILES= 0.alloc.c 0.args.c 0.def.c 0.extr.c 0.graph.c 0.list.c \
	0.parts.c 0.string.c 1.finish.c 1.form.c 1.fort.c 1.hash.c \
	1.init.c 1.line.c 1.main.c 1.node.c 1.recog.c 1.tables.c \
	2.dfs.c 2.dom.c 2.head.c 2.inarc.c 2.main.c 2.tree.c \
	3.branch.c 3.flow.c 3.loop.c 3.main.c 3.reach.c 3.then.c \
	4.brace.c 4.form.c 4.main.c 4.out.c bdef.c main.c tree.c
SFILES=
YFILES=	beauty.y
LFILES=	lextab.l

DERIVED= beauty.c lextab.c
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(LFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(LFILES:.l=.o) $(SFILES:.s=.o)

all:	$(PRODUCTS)

install: DSTROOT $(DSTDIRS) all
	install ${IFLAGS} structure.NEW $(LIBDIR)/structure
	install ${IFLAGS} beautify.NEW $(LIBDIR)/beautify
	install -c struct $(BINDIR)/struct

#
# Building structure
#
STRUCTURE_OBJS=	main.o 0.alloc.o 0.args.o 0.def.o 0.extr.o 0.graph.o \
	0.list.o 0.parts.o 0.string.o 1.finish.o 1.form.o 1.fort.o \
	1.hash.o 1.init.o 1.line.o 1.main.o 1.node.o 1.recog.o \
	1.tables.o 2.dfs.o 2.dom.o 2.head.o 2.inarc.o 2.main.o \
	2.tree.o 3.branch.o 3.flow.o 3.loop.o 3.main.o 3.reach.o \
	3.then.o 4.brace.o 4.form.o 4.main.o 4.out.o

structure.NEW:	${STRUCTURE_OBJS} structure_vers.o
	$(CC) $(CFLAGS) -o $@ ${STRUCTURE_OBJS} structure_vers.o ${LIBS}

structure_vers.c: ${DOVERS} ${STRUCTURE_OBJS}
	@rm -f $@
	vers_string -c structure > $@

#
# Building beautify
#
BEAUTIFY_OBJS= beauty.o tree.o lextab.o bdef.o

beauty.y: b.h

y.tab.h: beauty.y

beautify.NEW:	${BEAUTIFY_OBJS} beautify_vers.o
	$(CC) $(CFLAGS) -o $@ ${BEAUTIFY_OBJS} beautify_vers.o ${BEAUTY_LIBS}

beautify_vers.c: ${DOVERS} ${BEAUTIFY_OBJS}
	@rm -f $@
	vers_string -c beautify > $@

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
