#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
#CFLAGS = -bsd -g -O -I../include
CFLAGS = -g -O -Wall -I../include
IFLAGS = -s

PRODUCT = mkshlib
LOCBINDIR = /usr/local/bin

HFILES = mkshlib.h errors.h
MKCFILES = mkshlib.c parse_spec.c target.c host.c errors.c
CMPCFILES = cmpshlib.c parse_spec.c errors.c misc.c
CFILES = ${MKCFILES} ${CMPCFILES}

SRCS =  ${MKCFILES} ${CMPCFILES} ${HFILES}
OTHER_SRCS = notes
MKOBJS = ${MKCFILES:.c=.o}
CMPOBJS = ${CMPCFILES:.c=.o}

FILES = ${SRCS} Makefile Makedep
INTERMEDIATES =
DERIVED =
GARBAGE = TAGS tags vers.c

all:	${PRODUCT}

#
# Building PRODUCT
#

mkshlib.NEW: ${MKOBJS} vers.o
	$(CC) $(CFLAGS) -o $@ ${MKOBJS} vers.o

cmpshlib.NEW:  ${CMPOBJS} vers.o
	$(CC) $(CFLAGS) -o $@ ${CMPOBJS} vers.o

vers.c: ${DOVERS} ${OBJS}
	@rm -f $@
	vers_string -c ${PRODUCT} > $@


#
# Standard targets
#
${PRODUCT}:	.depend_done $$@.NEW cmpshlib.NEW

clean:	ALWAYS
	-rm -f *.o *.NEW
	-rm -f ${PRODUCT} ${DERIVED} ${INTERMEDIATES} ${GARBAGE}
	-rm -f Makedep .depend_done
	touch Makedep

install: ${DSTROOT}${LOCBINDIR} all
	install ${IFLAGS} -m 555 mkshlib.NEW ${DSTROOT}${LOCBINDIR}/mkshlib
	install ${IFLAGS} -m 555 cmpshlib.NEW ${DSTROOT}${LOCBINDIR}/cmpshlib

local: ${LCLROOT}${LOCBINDIR} all
	install ${IFLAGS} -m 555 mkshlib.NEW ${LCLROOT}${LOCBINDIR}/mkshlib
	install ${IFLAGS} -m 555 cmpshlib.NEW ${LCLROOT}${LOCBINDIR}/cmpshlib


DSTROOT LCLROOT SRCROOT:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

${DSTROOT}${LOCBINDIR} ${LCLROOT}${LOCBINDIR}:
	mkdirs $@

depend .depend_done: ${DERIVED}
	rm -f Makedep
	${CC} ${CFLAGS} -M ${CFILES} ${SFILES} ${DERIVED} | \
	    awk '{ if ($$1 != prev) { if (rec != "") print rec; \
		rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		else rec = rec " " $$2 } } \
		END { print rec }' >Makedep
	-touch .depend_done


tags:	ALWAYS
	etags ${SRCS}
	ctags ${SRCS}

ALWAYS:

include Makedep
