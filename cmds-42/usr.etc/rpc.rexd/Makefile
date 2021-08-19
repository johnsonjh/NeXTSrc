#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCTS= under where on rpc.rexd
ETCDIR=	$(DSTROOT)/usr/etc
BINDIR= $(DSTROOT)/usr/bin
DSTDIRS= $(ETCDIR) $(BINDIR)

CDEFS=	-DNeXT_MOD -D__STRICT_BSD__ -I/usr/include/rpcsvc
CFLAGS=	-O -g -fwritable-strings $(CDEFS)
LDFLAGS= -object
IFLAGS= -s
LIBS= -lsys_s -lrpcsvc

HFILES=
YFILES=
CFILES= mount_nfs.c on.c rexd.c under.c unix_login.c where.c where_main.c
SFILES=

DERIVED=
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

all:	$(PRODUCTS)

install: DSTROOT $(DSTDIRS) all
	install ${IFLAGS} rpc.rexd.NEW $(ETCDIR)/rpc.rexd
	install ${IFLAGS} on.NEW $(BINDIR)/on

#
# Building rexd
#
REXD_OBJS= rexd.o mount_nfs.o unix_login.o

rpc.rexd.NEW:	${REXD_OBJS} rexd_vers.o
	$(CC) $(CFLAGS) -o $@ ${REXD_OBJS} rexd_vers.o ${LIBS}

rexd_vers.c: ${DOVERS} ${REXD_OBJS}
	@rm -f $@
	vers_string -c rexd > $@

#
# Building on
#
ON_OBJS= on.o where.o

on.NEW:	${ON_OBJS} on_vers.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ ${ON_OBJS} on_vers.o ${LIBS} -ltermcap

on_vers.c: ${DOVERS} ${ON_OBJS}
	@rm -f $@
	vers_string -c on > $@

#
# Building under
#
UNDER_OBJS= under.o mount_nfs.o

under.NEW:	${UNDER_OBJS} under_vers.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ ${UNDER_OBJS} under_vers.o ${LIBS}

under_vers.c: ${DOVERS} ${UNDER_OBJS}
	@rm -f $@
	vers_string -c under > $@

#
# Building where
#
WHERE_OBJS= where_main.o where.o

where.NEW:	${WHERE_OBJS} where_vers.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ ${WHERE_OBJS} where_vers.o ${LIBS}

where_vers.c: ${DOVERS} ${WHERE_OBJS}
	@rm -f $@
	vers_string -c where > $@


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
