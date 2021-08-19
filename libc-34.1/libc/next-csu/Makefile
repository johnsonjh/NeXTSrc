#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.4 (Berkeley) 9/5/85
#
#	crt0	Normal C run time startoff
#	mcrt0	C run time start off for profiling, ``prof'' conventions
#	gcrt0	C run time start off for profiling, ``gprof'' conventions
#
LIBDIR= /lib

SRCS=	crt0.s mon.c gmon.c init_shlibs.c
OBJS=	crt0.o mcrt0.o gcrt0.o mon.o gmon.o init_shlibs.o
TAGSFILE=tags
INSTALL=install
CFLAGS= -I../../usr.include
IFLAGS=	-m 644 -S
MV=	mv -f

all: ${OBJS}

install: DSTROOT ${OBJS} ${DSTROOT}${LIBDIR}
	${INSTALL} ${IFLAGS} crt0.o ${DSTROOT}${LIBDIR}
	${INSTALL} ${IFLAGS} mcrt0.o ${DSTROOT}${LIBDIR}
	${INSTALL} ${IFLAGS} gcrt0.o ${DSTROOT}${LIBDIR}

local: LCLROOT ${OBJS} ${LCLROOT}${LIBDIR}
	${INSTALL} ${IFLAGS} crt0.o ${LCLROOT}${LIBDIR}
	${INSTALL} ${IFLAGS} mcrt0.o ${LCLROOT}${LIBDIR}
	${INSTALL} ${IFLAGS} gcrt0.o ${LCLROOT}${LIBDIR}

DSTROOT LCLROOT:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

${DSTROOT}${LIBDIR}:
	mkdirs $@

clean:
	rm -f ${OBJS} *.o core errs tags Makefile.bak
	rm -f Makedep .depend_done

