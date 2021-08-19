#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#

PRODUCT= mt
BINDIR=	/bin

HFILES=
CFILES= ${PRODUCT}.c

DERIVED=
GARBAGE= tags errs vers.c

CFLAGS=	-O
IFLAGS= -s
LDFLAGS= -object
LIBS= -lsys_s

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

.c.o:
	$(CC) -MD $(CFLAGS) $(COPTS) -c $<
	md -d -u Makedep $*.d

all:	${PRODUCT}

clean:	ALWAYS
	-rm -f *.o *.NEW ${PRODUCT} ${DERIVED} ${GARBAGE} Makedep

install: DSTROOT ${DSTROOT}${BINDIR} all
	install ${IFLAGS} ${PRODUCT}.NEW ${DSTROOT}${BINDIR}/${PRODUCT}

local: LCLROOT ${LCLROOT}${BINDIR} all
	install ${IFLAGS} ${PRODUCT}.NEW ${LCLROOT}${BINDIR}/${PRODUCT}

tags:	ALWAYS
	ctags ${SRCS}

${PRODUCT}:	$$@.NEW

${PRODUCT}.NEW:	${OBJS} vers.o
	${CC} ${LDFLAGS} ${CFLAGS} -o $@ ${OBJS} vers.o ${LIBS}

vers.c: ${OBJS}
	@rm -f $@
	vers_string -c ${PRODUCT} > $@

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

ALWAYS:

-include Makedep
