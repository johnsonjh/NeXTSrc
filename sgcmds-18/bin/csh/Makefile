#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCT= csh
RCFILES= cshrc.std login.std logout.std
BINDIR=	$(DSTROOT)/bin
CONFDIR= $(DSTROOT)/private/etc
DSTDIRS= $(BINDIR) $(CONFDIR)

CDEFS=	-DNeXT_MOD -DTELL -DVFORK -DCMUCS -D__STRICT_BSD__
CFLAGS=	-g -O -fwritable-strings $(CDEFS)
IFLAGS= -s
LIBS=   -lsys_s -ltermcap
XSTR= xstr

HFILES=	c.h libc.h sh.char.h sh.dir.h sh.h sh.local.h sh.proc.h
YFILES=
CFILES=	dosetpath.c fold.c getbool.c prstab.c salloc.c \
	setpath.c sh.c sh.char.c sh.dir.c sh.dol.c sh.err.c sh.exec.c \
	sh.exp.c sh.file.c sh.func.c sh.glob.c sh.hist.c sh.init.c sh.lex.c \
	sh.misc.c sh.parse.c sh.print.c sh.proc.c sh.sem.c sh.set.c \
	sh.time.c skipto.c stablk.c stlmatch.c tenex.c printf.c
SFILES=

DERIVED=
GARBAGE= x.c xs.c strings

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

all:	$(PRODUCT)

install: DSTROOT $(DSTDIRS) all symlinks
	install $(IFLAGS) $(PRODUCT).NEW $(BINDIR)/$(PRODUCT)
	install -c -m 644 $(RCFILES) $(CONFDIR)

symlinks:
	@if [ ! -h ${DSTROOT}/etc ] ; \
	then \
		echo "ln -s private/etc ${DSTROOT}/etc" ; \
		ln -s private/etc ${DSTROOT}/etc ; \
	fi
	@if [ ! -h ${DSTROOT}/tmp ] ; \
	then \
		echo "ln -s private/tmp ${DSTROOT}/tmp" ; \
		ln -s private/tmp ${DSTROOT}/tmp ; \
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
