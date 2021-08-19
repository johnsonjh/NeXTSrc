#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCTS= Mail fmt
BINDIR=	$(DSTROOT)/usr/ucb
LIBDIR=	$(DSTROOT)/usr/lib
CONFDIR= $(DSTROOT)/private/etc
DSTDIRS= $(BINDIR) $(LIBDIR)

CDEFS=	-DSHLIB -D__STRICT_BSD__ -DVMUNIX -DV7 -DNeXT_NFS
CFLAGS=	-O -g -fwritable-strings $(CDEFS)
IFLAGS= -s
LIBS= -lsys_s

HFILES=	rcv.h configdefs.h def.h glob.h v7.local.h local.h
YFILES=
CFILES=	aux.c cmd1.c cmd2.c cmd3.c cmdtab.c collect.c config.c edit.c fio.c \
	getname.c head.c v7.local.c lex.c list.c main.c \
	names.c optim.c popen.c quit.c send.c strings.c temp.c tty.c vars.c \
	version.c fmt.c
SFILES=

DERIVED=
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

all:	$(PRODUCTS)

install: DSTROOT $(DSTDIRS) all symlinks
	install ${IFLAGS} Mail.NEW $(BINDIR)/Mail
	cd misc;  install -c -m 444 Mail.*help* $(LIBDIR)
	cd misc;  install -c -m 444 Mail.rc $(CONFDIR)
	rm -f $(BINDIR)/mail
	ln -s Mail $(BINDIR)/mail
	install ${IFLAGS} fmt.NEW $(BINDIR)/fmt

symlinks:
	@if [ ! -h ${DSTROOT}/etc ] ; \
	then \
		echo "ln -s private/etc ${DSTROOT}/etc" ; \
		ln -s private/etc ${DSTROOT}/etc ; \
	fi
	@if [ ! -h ${DSTROOT}/usr/spool ] ; \
	then \
		echo "ln -s ../private/spool ${DSTROOT}/usr/spool" ; \
		ln -s ../private/spool ${DSTROOT}/usr/spool ; \
	fi

#
# Building PRODUCT
#

$(PRODUCT).NEW:	$(OBJS) vers.o
	$(CC) $(CFLAGS) -o $@ $(OBJS) vers.o $(LIBS)

vers.c: $(DOVERS) $(OBJS)
	@rm -f $@
	vers_string -c $(PRODUCT) > $@

#
# Building Mail
#
MAIL_OBJS= aux.o cmd1.o cmd2.o cmd3.o cmdtab.o collect.o config.o edit.o \
	fio.o getname.o head.o v7.local.o lex.o list.o main.o names.o \
	optim.o popen.o quit.o send.o strings.o temp.o tty.o vars.o \
	version.o

Mail.NEW:	${MAIL_OBJS} Mail_vers.o
	$(CC) $(CFLAGS) -o $@ ${MAIL_OBJS} Mail_vers.o ${LIBS}

Mail_vers.c: ${DOVERS} ${MAIL_OBJS}
	@rm -f $@
	vers_string -c Mail > $@

Mail_vers.o: Mail_vers.c
	cc -c ${CFLAGS} Mail_vers.c

#
# Building fmt
#
FMT_OBJS= fmt.o head.o

fmt.NEW:	${FMT_OBJS} fmt_vers.o
	$(CC) $(CFLAGS) -o $@ ${FMT_OBJS} fmt_vers.o ${LIBS}

fmt_vers.c: ${DOVERS} ${FMT_OBJS}
	@rm -f $@
	vers_string -c fmt > $@

fmt_vers.o: fmt_vers.c
	cc -c ${CFLAGS} fmt_vers.c


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
