#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCT= sendmail
BINDIR= $(DSTROOT)/usr/bin
LIBDIR=	$(DSTROOT)/usr/lib
DSTDIRS= $(LIBDIR)

CDEFS=	-DNeXT_MOD -D__STRICT_BSD__ -I../include -I/usr/include/arpa \
	-DVMUNIX -DMXDOMAIN -DYELLOWPAGES
CFLAGS=	-O -g -fwritable-strings $(CDEFS)
IFLAGS= -s -c -o root -m 4551
LIBS= -lsys_s -ldbm

HFILES= conf.h sendmail.h dbm.h mailstats.h trace.h
YFILES=
CFILES= conf.c deliver.c main.c parseaddr.c err.c alias.c savemail.c \
	sysexits.c util.c bmove.c arpadate.c version.c collect.c \
	macro.c headers.c readcf.c stab.c recipient.c stats.c daemon.c \
	usersmtp.c srvrsmtp.c queue.c clock.c trace.c envelope.c \
	convtime.c remote.c domain.c
SFILES=

DERIVED=
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

all:	$(PRODUCT)

install: DSTROOT $(DSTDIRS) all symlinks
	install $(IFLAGS) -l $(BINDIR)/newaliases -l $(BINDIR)/mailq $(PRODUCT).NEW $(LIBDIR)/$(PRODUCT)

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
