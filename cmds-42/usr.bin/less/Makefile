#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
# If you add or delete functions, remake funcs.h by doing:
#	make newfuncs
# This depends on the coding convention of function headers looking like:
#	" \t public <function-type> \n <function-name> ( ... ) "
#
PRODUCTS= less lesskey
BINDIR=	$(DSTROOT)/usr/bin
HELPDIR= $(DSTROOT)/usr/lib
DSTDIRS= $(BINDIR) $(HELPDIR)
HELPFILE = /usr/lib/less.help


CDEFS=	-D__STRICT_BSD__
CFLAGS=	-O -g -fwritable-strings $(CDEFS)
IFLAGS= -s
LIBS = -lsys_s -ltermcap

HFILES= less.h position.h
YFILES=
CFILES= main.c option.c prim.c ch.c position.c input.c linenum.c \
	screen.c prompt.c line.c signal.c os.c help.c ttyin.c command.c \
	output.c decode.c tags.c version.c
SFILES=

DERIVED= funcs.h
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

all:	$(PRODUCTS)

install: DSTROOT $(DSTDIRS) all
	install $(IFLAGS) less.NEW $(BINDIR)/less
	install $(IFLAGS) lesskey.NEW $(BINDIR)/lesskey
	install -m 444 -c less.help $(HELPDIR)/less.help

#
# Building PRODUCTS
#

less.NEW:  $(OBJS) vers.o
	$(CC) $(CFLAGS) -o $@ $(OBJS) vers.o $(LIBS)

lesskey.NEW:  lesskey.o vers.o
	$(CC) $(CFLAGS) -o $@ $(OBJS) vers.o $(LIBS)

help.o:
	$(CC) $(CFLAGS) -c -DHELPFILE=\"$(HELPFILE)\" help.c

vers.c: $(DOVERS) $(OBJS)
	@rm -f $@
	vers_string -c $(PRODUCT) > $@

$(OBJS):	funcs.h

funcs.h:
	awk -f mkfuncs.awk $(CFILES) >funcs.h

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
