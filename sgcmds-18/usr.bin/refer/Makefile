#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCTS= mkey inv hunt refer addbib lookbib sortbib
BINDIR=	$(DSTROOT)/usr/bin
LIBDIR=	$(DSTROOT)/usr/lib/refer
MACDIR= $(DSTROOT)/usr/lib/tmac
DSTDIRS= $(BINDIR) $(LIBDIR) $(MACDIR)

CDEFS=	-DNeXT_MOD -D__STRICT_BSD__
CFLAGS=	-O -g -fwritable-strings $(CDEFS)
IFLAGS= -s
LIBS= -lsys_s

HFILES=	refer..c what..c
YFILES=
CFILES=	addbib.c annobib.c deliv1.c deliv2.c flagger.c glue1.c glue2.c \
	glue3.c glue4.c glue5.c hunt1.c hunt2.c hunt3.c hunt5.c hunt6.c \
	hunt7.c hunt8.c hunt9.c inv1.c inv2.c inv3.c inv5.v inv6.c \
	kaiser.c lookbib.c mkey1.c mkey2.c mkey3.c refer0.c refer1.c \
	refer2.c refer3.c refer4.c refer5.c refer6.c refer7.c refer8.c \
	shell.c sortbib.c thash.c tick.c types.c what1.c what2.c what3.c \
	what4.c
SFILES=

DERIVED=
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

all:	$(PRODUCTS)

install: DSTROOT $(DSTDIRS) all
	install ${IFLAGS} mkey.NEW $(LIBDIR)/mkey
	install ${IFLAGS} inv.NEW $(LIBDIR)/inv
	install ${IFLAGS} hunt.NEW $(LIBDIR)/hunt
	install ${IFLAGS} refer.NEW $(BINDIR)/refer
	install ${IFLAGS} addbib.NEW $(BINDIR)/addbib
	install ${IFLAGS} sortbib.NEW $(BINDIR)/sortbib
	install -c roffbib.sh $(BINDIR)/roffbib
	install -c indxbib.sh $(BINDIR)/indxbib
	install ${IFLAGS} lookbib.NEW $(BINDIR)/lookbib
	install -c tmac.bib $(MACDIR)/tmac.bib

#
# Building mkey
#
MKEY_OBJS= mkey1.o mkey2.o mkey3.o deliv2.o

mkey.NEW:	${MKEY_OBJS} mkey_vers.o
	$(CC) $(CFLAGS) -o $@ ${MKEY_OBJS} mkey_vers.o ${LIBS}

mkey_vers.c: ${DOVERS} ${MKEY_OBJS}
	@rm -f $@
	vers_string -c mkey > $@

#
# Building inv
#
INV_OBJS= inv1.o inv2.o inv3.o inv5.o inv6.o deliv2.o

inv.NEW:	${INV_OBJS} inv_vers.o
	$(CC) $(CFLAGS) -o $@ ${INV_OBJS} inv_vers.o ${LIBS}

inv_vers.c: ${DOVERS} ${INV_OBJS}
	@rm -f $@
	vers_string -c inv > $@

#
# Building hunt
#
HUNT_OBJS= hunt1.o hunt2.o hunt3.o hunt5.o hunt6.o hunt7.o hunt8.o \
	hunt9.o glue4.o glue5.o refer3.o shell.o deliv2.o tick.o

hunt.NEW:	${HUNT_OBJS} hunt_vers.o
	$(CC) $(CFLAGS) -o $@ ${HUNT_OBJS} hunt_vers.o ${LIBS}

hunt_vers.c: ${DOVERS} ${HUNT_OBJS}
	@rm -f $@
	vers_string -c hunt > $@

#
# Building refer
#
REFER_OBJS= refer0.o refer1.o refer2.o refer4.o refer5.o refer6.o \
	refer7.o refer8.o glue1.o glue2.o glue3.o glue4.o glue5.o mkey3.o \
	 hunt2.o hunt3.o hunt5.o hunt6.o hunt7.o hunt8.o hunt9.o \
	deliv2.o shell.o

refer.NEW:	${REFER_OBJS} refer_vers.o
	$(CC) $(CFLAGS) -o $@ ${REFER_OBJS} refer_vers.o ${LIBS}

refer_vers.c: ${DOVERS} ${REFER_OBJS}
	@rm -f $@
	vers_string -c refer > $@

#
# Building addbib
#
ADDBIB_OBJS= addbib.o

addbib.NEW:	${ADDBIB_OBJS} addbib_vers.o
	$(CC) $(CFLAGS) -o $@ ${ADDBIB_OBJS} addbib_vers.o ${LIBS}

addbib_vers.c: ${DOVERS} ${ADDBIB_OBJS}
	@rm -f $@
	vers_string -c addbib > $@

#
# Building lookbib
#
LOOKBIB_OBJS= lookbib.o

lookbib.NEW:	${LOOKBIB_OBJS} lookbib_vers.o
	$(CC) $(CFLAGS) -o $@ ${LOOKBIB_OBJS} lookbib_vers.o ${LIBS}

lookbib_vers.c: ${DOVERS} ${LOOKBIB_OBJS}
	@rm -f $@
	vers_string -c lookbib > $@

#
# Building sortbib
#
SORTBIB_OBJS= sortbib.o

sortbib.NEW:	${SORTBIB_OBJS} sortbib_vers.o
	$(CC) $(CFLAGS) -o $@ ${SORTBIB_OBJS} sortbib_vers.o ${LIBS}

sortbib_vers.c: ${DOVERS} ${SORTBIB_OBJS}
	@rm -f $@
	vers_string -c sortbib > $@

#
# Building whatabout
#
WHATABOUT_OBJS= what1.o what2.o what3.o what4.o shell.o mkey3.o

whatabout.NEW:	${WHATABOUT_OBJS} whatabout_vers.o
	$(CC) $(CFLAGS) -o $@ ${WHATABOUT_OBJS} whatabout_vers.o ${LIBS}

whatabout_vers.c: ${DOVERS} ${WHATABOUT_OBJS}
	@rm -f $@
	vers_string -c whatabout > $@

#
# Building refpart
#
REFPART_OBJS= refer0.o refer1.o refer2.o refer3.o refer4.o refer5.o \
	refer6.o refer7.o refer8.o deliv2.o glue4.o

refpart.NEW:	${REFPART_OBJS} refpart_vers.o
	$(CC) $(CFLAGS) -o $@ ${REFPART_OBJS} refpart_vers.o ${LIBS}

refpart_vers.c: ${DOVERS} ${REFPART_OBJS}
	@rm -f $@
	vers_string -c refpart > $@

#
# Building deliv
#
DELIV_OBJS= deliv1.o deliv2.o

deliv.NEW:	${DELIV_OBJS} deliv_vers.o
	$(CC) $(CFLAGS) -o $@ ${DELIV_OBJS} deliv_vers.o ${LIBS}

deliv_vers.c: ${DOVERS} ${DELIV_OBJS}
	@rm -f $@
	vers_string -c deliv > $@

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
