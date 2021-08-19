#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
CFLAGS=	-O -DNeXT_MOD -fwritable-strings -D__STRICT_BSD__
IFLAGS= -s
LIBS=

PRODUCT= makekey

DSTDIR=
BINDIR=	$(DSTDIR)/usr/lib

HFILES=
YFILES=
CFILES= ${PRODUCT}.c
SFILES=

SRCS= ${HFILES} ${YFILES} ${CFILES} ${SFILES}
OBJS= ${CFILES:.c=.o} ${YFILES:.y=.o} ${SFILES:.s=.o}

FILES=	${SRCS} Makefile Makedep
INTERMEDIATES=
DERIVED=
GARBAGE= TAGS tags vers.c

all:	${PRODUCT}

#
# Building PRODUCT
#

${PRODUCT}.NEW:	${OBJS} vers.o
	$(CC) $(CFLAGS) -o $@ ${OBJS} vers.o ${LIBS}

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

install: DSTROOT ${DSTROOT}${BINDIR} all
	install ${IFLAGS} ${PRODUCT}.NEW ${DSTROOT}${BINDIR}/${PRODUCT}

local: LCLROOT ${LCLROOT}${BINDIR} all
	install ${IFLAGS} ${PRODUCT}.NEW ${LCLROOT}${BINDIR}/${PRODUCT}

DSTROOT LCLROOT:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

${DSTROOT}${BINDIR} ${LCLROOT}${BINDIR}:
	mkdirs $@

depend .depend_done: ${DERIVED}
	rm -f Makedep .depend_done
	${CC} -M ${CFILES} ${SFILES} ${DERIVED} | \
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
