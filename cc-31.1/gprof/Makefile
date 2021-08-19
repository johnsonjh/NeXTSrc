#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
CFLAGS=	-DNeXT_MOD -I../include -g -O
IFLAGS= -s -g kmem -m 2555
LIBS=

PRODUCT= gprof
PROJECT= cmds

BINDIR=	/usr/ucb
LIBDIR= /usr/lib

DSTDIRS= ${DSTROOT}${BINDIR} ${DSTROOT}${LIBDIR}
LCLDIRS= ${LCLROOT}${BINDIR} ${LCLROOT}${LIBDIR}

HFILES=	gprof.h next.h mlist.h
YFILES=
CFILES= gprof.c arcs.c dfn.c lookup.c calls.c hertz.c printgprof.c \
	printlist.c mach_o.c mlist.c getobjcseg.c scatter.c
SFILES=

SRCS= ${HFILES} ${YFILES} ${CFILES} ${SFILES}
OBJS= ${CFILES:.c=.o} ${YFILES:.y=.o} ${SFILES:.s=.o}

FILES=	${SRCS} Makefile Makedep gprof.flat gprof.callg notes
INTERMEDIATES=
DERIVED=
GARBAGE= TAGS tags vers.c

all:	${PRODUCT}

#
# Building PRODUCT
#

${PRODUCT}.NEW:	${OBJS} vers.o
	$(CC) $(CFLAGS) -o $@ ${OBJS} vers.o ${LIBS} -lsys_s

vers.c: ${DOVERS} ${OBJS}
	@rm -f $@
	vers_string -c ${PRODUCT} > $@


#
# Standard targets
#
${PRODUCT}:	.depend_done $$@.NEW

clean:	ALWAYS
	-rm -f *.o *.NEW
	-rm -f ${PRODUCT} ${DERIVED} ${INTERMEDIATES} ${GARBAGE}
	-rm -f Makedep .depend_done
	touch Makedep

install: DSTROOT ${DSTDIRS} all
	install ${IFLAGS} ${PRODUCT}.NEW ${DSTROOT}${BINDIR}/${PRODUCT}
	install -c -m 444 gprof.flat ${DSTROOT}${LIBDIR}
	install -c -m 444 gprof.callg ${DSTROOT}${LIBDIR}

local: LCLROOT ${LCLDIRS} all
	install ${IFLAGS} ${PRODUCT}.NEW ${LCLROOT}${BINDIR}/${PRODUCT}
	install -c -m 444 gprof.flat ${LCLROOT}${LIBDIR}
	install -c -m 444 gprof.callg ${LCLROOT}${LIBDIR}

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

depend .depend_done: ${DERIVED}
	rm -f Makedep
	${CC} -M -I../include ${CFILES} ${SFILES} ${DERIVED} | \
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

-include Makedep
