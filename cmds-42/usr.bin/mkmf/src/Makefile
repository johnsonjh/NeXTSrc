#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1989
#
PRODUCT= mkmf
BINDIR=	$(DSTROOT)/usr/bin
DSTDIRS= $(BINDIR)

CDEFS=	-DNeXT_MOD -DV4BSD
CFLAGS=	-O -g -fwritable-strings $(CDEFS)
IFLAGS= -s
LIBS= -lsys_s

HFILES= Mkmf.h defaultrul.h defaultsfx.h dlist.h getarg.h hash.h macro.h \
	null.h path.h rule.h sccsid.h slist.h spms.h suffix.h system.h   \
	target.h yesno.h
CFILES= Mkmf.c badopt.c buildlist.c depend.c dlist.c editmf.c getcwp.c \
	getproject.c hash.c iolin.c macro.c misc.c mustfopen.c         \
	optpath.c pathcat.c pathhead.c pperror.c rule.c	slappend.c     \
	slinit.c slsort.c strpcpy.c strsav.c suffix.c usage.c warn.c   \
	warn2.c	warns.c
SFILES=

DERIVED= 
GARBAGE= 

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

all:	$(PRODUCT)

install: DSTROOT $(DSTDIRS) all
	install $(IFLAGS) $(PRODUCT).NEW $(BINDIR)/$(PRODUCT)

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
