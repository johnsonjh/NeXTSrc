#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
#	Makefile -- for Bourne shell (BRL VAX 4.2BSD version)
#
PRODUCT= sh
RCFILES= profile.std
BINDIR=	$(DSTROOT)/bin
CONFDIR= $(DSTROOT)/private/etc
DSTDIRS= $(BINDIR) $(CONFDIR)

# Kernel now defines BSD as "43" so we can't use -DBSD here
# We now use "BERKELEY"
CDEFS=	-DNeXT_MOD -D__STRICT_BSD__ -DNICE -DNICEVAL=4 -DJOBS -DBERKELEY \
	-DBRL -DTILDE_SUB -DMACH
CFLAGS=	-O -g -fwritable-strings $(CDEFS)
IFLAGS= -s
#
# Do not, under any circumstances link the Bourne Shell with the
# libsys shlib, unless the shlib is *FAR* enough away from the stack
# area of the Shell. This is because of its bogus way of doing memory
# allocation.
#
LIBS= -lc_static

HFILES= brkincr.h defs.h mac.h name.h sym.h ctype.h hash.h mode.h stak.h \
	timeout.h
YFILES=
CFILES= setbrk.c blok.c stak.c cmd.c fault.c main.c word.c string.c \
	name.c args.c xec.c service.c error.c io.c print.c macro.c expand.c \
	ctype.c msg.c test.c defs.c hash.c hashserv.c pwd.c func.c \
	jobs.c homedir.c echo.c ulimit.c malloc.c calloc.c
SFILES=

DERIVED=
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

all:	$(PRODUCT)

install: DSTROOT $(DSTDIRS) all symlinks
	install ${IFLAGS} -l $(BINDIR)/jsh ${PRODUCT}.NEW $(BINDIR)/${PRODUCT}
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
	$(LD) -lcrt0.o -o $@ $(OBJS) vers.o $(LIBS)

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
