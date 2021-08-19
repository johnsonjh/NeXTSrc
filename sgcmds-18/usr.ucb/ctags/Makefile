#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCTS= ctags ptags ctree cfunc
BINDIR=	$(DSTROOT)/usr/ucb
DSTDIRS= $(BINDIR)

CDEFS=	-DNeXT_MOD -D__STRICT_BSD__
CFLAGS=	-O -g -fwritable-strings $(CDEFS)
IFLAGS= -s
LIBS= -lsys_s

HFILES= tags.h
YFILES=
CFILES= ctags.c ctree.c findtag.c hash.c ptags.c tags.c
SFILES=

DERIVED=
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

all:	$(PRODUCTS)

install: DSTROOT $(DSTDIRS) all
	install $(IFLAGS) ctags.NEW $(BINDIR)/ctags
	install $(IFLAGS) ptags.NEW $(BINDIR)/ptags
	install $(IFLAGS) ctree.NEW $(BINDIR)/ctree
	install -xs -c -m 755 cfunc.NEW $(BINDIR)/cfunc

#
# Building ctags
#
CTAGS_OBJS= ctags.o

ctags.NEW:	$(CTAGS_OBJS) ctags_vers.o
	$(CC) $(CFLAGS) -o $@ $(CTAGS_OBJS) ctags_vers.o $(LIBS)

ctags_vers.c: $(DOVERS) $(CTAGS_OBJS)
	@rm -f $@
	vers_string -c ctags > $@

#
# Building ptags
#
PTAGS_OBJS= ptags.o tags.o

ptags.NEW:	$(PTAGS_OBJS) ptags_vers.o
	$(CC) $(CFLAGS) -o $@ $(PTAGS_OBJS) ptags_vers.o $(LIBS)

ptags_vers.c: $(DOVERS) $(PTAGS_OBJS)
	@rm -f $@
	vers_string -c ptags > $@

#
# Building ctree
#
CTREE_OBJS= ctree.o tags.o hash.o findtag.o

ctree.NEW:	$(CTREE_OBJS) ctree_vers.o
	$(CC) $(CFLAGS) -o $@ $(CTREE_OBJS) ctree_vers.o $(LIBS)

ctree_vers.c: $(DOVERS) $(CTREE_OBJS)
	@rm -f $@
	vers_string -c ctree > $@

#
# Building cfunc
#
cfunc.NEW:	cfunc.sh $(DOVERS)
	sed -e "s/#PROGRAM.*/#`vers_string cfunc`/" \
	    <cfunc.sh >cfunc.NEW

#
# Standard targets
#
$(PRODUCTS):	Makedep $$@.NEW

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
