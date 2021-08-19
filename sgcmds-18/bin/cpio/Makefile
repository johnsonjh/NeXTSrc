#
# Generic top-level makefile for simple commands
# Copyright (c) 1990 Mike DeMoney.  All rights reserved.
#
CDEFS= -DNeXT_MOD -D__STRICT_BSD__
CFLAGS=	-O -fwritable-strings $(CDEFS)
IFLAGS= -s
LIBS=

#
# The name of this program as installed
#
PRODUCT= cpio

#
# Where it gets installed
#
DSTDIR= ${DSTROOT}/usr/local
BINDIR=	${DSTDIR}/bin

#
# The directories that must exist for install to work
#
DSTDIRS= ${BINDIR}

#
# The list of files that make-up this command.  Used both
# for compilation and tags.
#
HFILES=
YFILES=
CFILES= ${PRODUCT}.c
SFILES=

SRCS= ${HFILES} ${YFILES} ${CFILES} ${SFILES}
OBJS= ${CFILES:.c=.o} ${YFILES:.y=.o} ${SFILES:.s=.o}

#
# DERIVED is the list of files that must be made before a make depend
# is done.  Typically the C file corresponding to yacc source goes here.
#
DERIVED=
GARBAGE= TAGS tags vers.c

#
# Standard targets
#
all:	${PRODUCT}

clean:	ALWAYS
	-rm -f *.o *.NEW
	-rm -f ${PRODUCT} ${DERIVED} ${GARBAGE}
	-rm -f Makedep .depend_done

install: DSTROOT ${BINDIR} all
	install ${IFLAGS} ${PRODUCT}.NEW ${BINDIR}/${PRODUCT}

#
# Building PRODUCT
#
${PRODUCT}.NEW:	${OBJS} vers.o
	$(CC) $(CFLAGS) -o $@ ${OBJS} vers.o ${LIBS}

vers.c: ${DOVERS} ${OBJS}
	@rm -f $@
	vers_string -c ${PRODUCT} > $@

${PRODUCT}:	.depend_done $$@.NEW

DSTROOT:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

${DSTDIRS}:
	mkdirs $@

depend .depend_done: ${DERIVED}
	rm -f Makedep
	${CC} -M ${CDEFS} ${CFILES} ${SFILES} ${DERIVED} | \
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
