#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCTS= dprog style1 style2 style3
BINDIR=	$(DSTROOT)/usr/bin
LIBDIR=	$(DSTROOT)/usr/lib
DSTDIRS= $(BINDIR) $(LIBDIR)

CDEFS=	-DNeXT_MOD -D__STRICT_BSD__ -DDICT=\"/usr/lib/dict.d\"
CFLAGS=	-O -g -fwritable-strings $(CDEFS)
IFLAGS= -s
LIBS= -lsys_s -ll

HFILES=	comp.h names.h style.h
CFILES= abbrev.c dict.c diction.c edict.c ehash.c nhash.c outp.c pscan.c ydict.c
SFILES=
YFILES=

DERIVED= part.c nwords.c end.c
GARBAGE= lex.yy.c *_vers.c

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

all:	$(PRODUCTS)

install: DSTROOT ${DSTDIRS} all
	install ${IFLAGS} style1.NEW $(LIBDIR)/style1
	install ${IFLAGS} style2.NEW $(LIBDIR)/style2
	install ${IFLAGS} style3.NEW $(LIBDIR)/style3
	install ${IFLAGS} dprog.NEW $(LIBDIR)/dprog
	install -c -xs -m 555 style.sh $(BINDIR)/style
	install -c -xs -m 555 diction.sh $(BINDIR)/diction
	install -c -xs -m 555 explain.sh $(BINDIR)/explain
	install -c -xs -m 555 dict.d $(LIBDIR)/dict.d
	install -c -xs -m 555 explain.d $(LIBDIR)/explain.d

#
# Building style3
#
STYLE3_OBJS= part.o pscan.o outp.o

style3.NEW:	${STYLE3_OBJS} style3_vers.o
	$(CC) $(CFLAGS) -o $@ ${STYLE3_OBJS} style3_vers.o ${LIBS}

style3_vers.c: ${DOVERS} ${STYLE3_OBJS}
	@rm -f $@
	vers_string -c style3 > $@

#
# Building style1
#
STYLE1_OBJS= nwords.o

style1.NEW:	${STYLE1_OBJS} style1_vers.o
	$(CC) $(CFLAGS) -o $@ ${STYLE1_OBJS} style1_vers.o ${LIBS}

style1_vers.c: ${DOVERS} ${STYLE1_OBJS}
	@rm -f $@
	vers_string -c style1 > $@

#
# Building style2
#
STYLE2_OBJS= end.o

style2.NEW:	${STYLE2_OBJS} style2_vers.o
	$(CC) $(CFLAGS) -o $@ ${STYLE2_OBJS} style2_vers.o ${LIBS}

style2_vers.c: ${DOVERS} ${STYLE2_OBJS}
	@rm -f $@
	vers_string -c style2 > $@

#
# Building dprog
#
DPROG_OBJS= diction.o

dprog.NEW:	${DPROG_OBJS} dprog_vers.o
	$(CC) $(CFLAGS) -o $@ ${DPROG_OBJS} dprog_vers.o ${LIBS}

dprog_vers.c: ${DOVERS} ${DPROG_OBJS}
	@rm -f $@
	vers_string -c dprog > $@


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
