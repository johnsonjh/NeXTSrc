#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCT= nroff
BINDIR=	$(DSTROOT)/usr/bin
DSTDIRS= $(BINDIR)

CDEFS=	-DNeXT_MOD -D__STRICT_BSD__ -DNROFF -DVMUNIX -I../troff/
CFLAGS=	-O -g -fwritable-strings $(CDEFS)
IFLAGS= -s
LIBS= -lsys_s

HFILES=
YFILES=
#
# Take out malloc(), calloc(), realloc() and free() multiple 
# definitions from Makefile.  This is so awk can link with the shlib's.
#
#CFILES= n6.c n10.c ../troff/ni.c ../troff/nii.c ../troff/n[12345789].c \
#	../troff/hytab.c ../troff/ntab.c ../troff/suftab.c ../troff/malloc.c \
#	../troff/calloc.c
CFILES= n6.c n10.c ../troff/ni.c ../troff/nii.c ../troff/n[12345789].c \
	../troff/hytab.c ../troff/ntab.c ../troff/suftab.c
SFILES=

DERIVED=
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
NOBJS= n6.o n10.o
#
# Take out malloc(), calloc(), realloc() and free() multiple 
# definitions from Makefile.  This is so awk can link with the shlib's.
#
#TOBJS= ni.o nii.o n1.o n2.o n3.o n4.o n5.o n7.o n8.o n9.o \
#	hytab.o ntab.o suftab.o malloc.o calloc.o
#
TOBJS= ni.o nii.o n1.o n2.o n3.o n4.o n5.o n7.o n8.o n9.o \
	hytab.o ntab.o suftab.o

all:	${PRODUCT} term

install: DSTROOT $(DSTDIRS) all
	install $(IFLAGS) $(PRODUCT).NEW $(BINDIR)/$(PRODUCT)
	cd term; ${MAKE} DSTROOT=${DSTROOT} install

term:	ALWAYS
	cd term; ${MAKE}

#
# Building PRODUCT
#

$(TOBJS):	../troff/$$(@:.o=.c)
	$(CC) $(CFLAGS) -c ../troff/$(@:.o=.c)

$(PRODUCT).NEW:	$(TOBJS) $(NOBJS) vers.o
	$(CC) $(CFLAGS) -o $@ $(TOBJS) $(NOBJS) vers.o $(LIBS)

vers.c: $(DOVERS) $(TOBJS) $(NOBJS)
	@rm -f $@
	vers_string -c $(PRODUCT) > $@

#
# Standard targets
#
$(PRODUCT):	Makedep $$@.NEW

clean:	ALWAYS
	-rm -f *.o *.NEW TAGS tags vers.c Makedep $(PRODUCT) $(DERIVED) \
		$(GARBAGE)
	cd term; ${MAKE} clean

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
