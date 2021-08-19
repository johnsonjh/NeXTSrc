#
# Generic top-level makefile for multi-file commands
# (c) NeXT, Inc.  1987
#
#
# Ownerships {see note at the top of lpd.c}
# ROOT		a user that can access any and all files
# DAEMON	someone special
# SPGRP		the group id of the spooling programs
#

ROOT = root
SPGRP = wheel

# Use this set of flags for release
SEGFLAGS =
LDFLAGS = 
CFLAGS = -g -O -DNeXT_MOD -DNeXT_NFS
IFLAGS= -s -o ${ROOT} -g ${SPGRP} -m 755

# Use this set of flags for development

LIBS = -lsys_s
BINDIR=	/usr/bin

PRODUCTS= dread dwrite dremove
PROJECT= def

SRCDIR= /MyDisk/Home/me/Programming/${PROJECT}_proj/$(PROJECT)

DSTDIRS= ${DSTROOT}${BINDIR}

LCLDIRS= ${LCLROOT}${BINDIR}

# Source files.
# All these files will be installed with "make installsrc"
HFILES = defaults.h open.h
CFILES = dread.c dremove.c dwrite.c open.c util.c defaults.c
MFILES = 
PSWFILES = 
YFILES=
SRCS=	${HFILES} ${YFILES} ${CFILES} ${SFILES} $(MFILES) $(PSWFILES)

FILES=	${SRCS} Makefile
INTERMEDIATES=
DERIVED=
GARBAGE= TAGS tags *_vers.c

.psw.o:	
	pswrap < $*.psw > $*.c
	$(CC) -MD $(CFLAGS) -c -o $@ $*.c
	md -d -u Makedep $*.d

.c.o:
	$(CC) -MD $(CFLAGS) -c -o $@ $<
	md -d -u Makedep $*.d

all:	${PRODUCTS}

#
# Building Preferences
#
DREAD_CFILES = dread.c open.c defaults.c util.c
DREAD_OBJS = ${DREAD_CFILES:.c=.o} ${MFILES:.m=.o} $(PSWFILES:.psw=.o)
dread: ${DREAD_OBJS}
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ ${DREAD_OBJS} ${LIBS}

DREMOVE_CFILES = dremove.c defaults.c util.c
DREMOVE_OBJS = ${DREMOVE_CFILES:.c=.o} ${MFILES:.m=.o} $(PSWFILES:.psw=.o)
dremove: ${DREMOVE_OBJS}
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ ${DREMOVE_OBJS} ${LIBS}

DWRITE_CFILES = dwrite.c defaults.c util.c
DWRITE_OBJS = ${DWRITE_CFILES:.c=.o} ${MFILES:.m=.o} $(PSWFILES:.psw=.o)
dwrite: ${DWRITE_OBJS}
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ ${DWRITE_OBJS} ${LIBS}

#
# Standard targets
#
${PRODUCTS}:	$(OBJS)

clean:	ALWAYS
	-rm -f *.o
	-rm -f *~
	-rm -f ${PRODUCTS} ${DERIVED} ${INTERMEDIATES} ${GARBAGE}
	-rm -f Makedep .depend_done

install: DSTROOT ${DSTDIRS} all
	mkdirs $(DSTROOT)$(BINDIR)
	install ${IFLAGS} dread ${DSTROOT}${BINDIR}
	install ${IFLAGS} dremove ${DSTROOT}${BINDIR}
	install ${IFLAGS} dwrite ${DSTROOT}${BINDIR}

installsrc: SRCROOT
	-rm -rf ${SRCROOT}
	mkdirs $(SRCROOT)
	cp ${FILES} ${SRCROOT}
	touch $(SRCROOT)/Makedep

DSTROOT LCLROOT SRCROOT:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

${DSTDIRS} ${LCLDIRS}:
	mkdirs $@

tags:	ALWAYS
	etags ${SRCS}
	ctags ${SRCS}

ALWAYS:

-include Makedep

