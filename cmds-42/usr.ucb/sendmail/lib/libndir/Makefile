# @(#)Makefile	4.4 (Berkeley) 10/13/82
DSTDIR= /bootdisk/NeXT/Unix
LIBDIR=${DSTROOT}${DSTDIR}/usr/lib
INCDIR=${DSTROOT}${DSTDIR}/usr/include
MAN3DIR=${DSTROOT}${DSTDIR}/usr/man/man3

CFLAGS=	-O -g -I. -fwritable-strings -D__STRICT_BSD__

OBJS=	closedir.o opendir.o readdir.o seekdir.o telldir.o

.c.o:
	${CC} ${CFLAGS} -c $*.c
	-ld -x -r $*.o
	mv a.out $*.o

libndir: ${OBJS}
	ar cru libndir ${OBJS}
	ranlib libndir

install: ckdstroot libndir
	cp dir.h ${INCDIR}/dir.h
	cp libndir ${LIBDIR}/libndir.a
	ranlib ${LIBDIR}/libndir.a
	cp directory.3s ${MAN3DIR}/directory.3s

ckdstroot:
	@if [ -n "${DSTROOT}" ]; \
	then \
		exit 0; \
	else \
		echo Must define DSTROOT; \
		exit 1; \
	fi

clean:
	rm -f libndir ${OBJS}
