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
LIBDIR= $(DSTDIR)/lib
LOCALLIBDIR= $(DSTDIR)/usr/local/lib
INDRFILE= ../../IND_LIST

SRCS=	crt0.s mon.c init_shlibs.c
OBJS=	crt0.o mcrt0.o gcrt0.o mon.o init_shlibs.o pscrt0.o
TAGSFILE=tags
INSTALL=install
CFLAGS= -MD -I../../../usr.include
IFLAGS=	-m 644 -S
MV=	mv -f

.c.o:
	$(CC) -MD $(CFLAGS) $(COPTS) -c $<
	md -d -u Makedep $*.d

all: $(OBJS)

indr: $(OBJS)
	@for i in $(OBJS); 						\
	do 								\
		indr -n $(INDRFILE) $$i x.o;				\
		mv x.o $$i;						\
	done
	

install: DSTROOT clean indr ${DSTROOT}${LIBDIR}
	${INSTALL} ${IFLAGS} crt0.o ${DSTROOT}${LIBDIR}
	${INSTALL} ${IFLAGS} gcrt0.o ${DSTROOT}${LIBDIR}
	${INSTALL} ${IFLAGS} pscrt0.o ${DSTROOT}${LOCALLIBDIR}

local: LCLROOT ${OBJS} ${LCLROOT}${LIBDIR}
	${INSTALL} ${IFLAGS} crt0.o ${LCLROOT}${LIBDIR}
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

${DSTROOT}${LOCALLIBDIR}:
	mkdirs $@

crt0.o:	crt0.s init_shlibs.o
	@rm -f x.s
	cc -MD -E ${DEFS} ${DFLAGS} -DCRT0 -DASSEMBLER crt0.s > x.s
	as -o x.o x.s
	ld -X -r -o crt0.o x.o init_shlibs.o
	rm -f x.s x.o
	md -d -u Makedep crt0.d

pscrt0.o:	crt0.s init_shlibs.o
	@rm -f x.s
	cc -E ${DEFS} ${DFLAGS} -DCRT0 -DPOSTSCRIPT -DASSEMBLER crt0.s > x.s
	as -o x.o x.s
	ld -X -r -o pscrt0.o x.o init_shlibs.o
	rm -f x.s x.o

moncrt0.o: crt0.s
	@rm -f x.s
	cc -E ${DEFS} ${DFLAGS} -DMCRT0 -DASSEMBLER crt0.s > x.s
	as -o x.o x.s
	ld -X -r -o moncrt0.o x.o
	rm -f x.s x.o

gmoncrt0.o: crt0.s
	@rm -f x.s
	cc -E ${DEFS} ${DFLAGS} -DGCRT0 -DASSEMBLER crt0.s > x.s
	as -o x.o x.s
	ld -X -r -o gmoncrt0.o x.o
	rm -f x.s x.o

gcrt0.o: gmoncrt0.o init_shlibs.o
	ld -X -r -o gcrt0.o gmoncrt0.o init_shlibs.o

mcrt0.o: moncrt0.o mon.o init_shlibs.o
	ld -X -r -o mcrt0.o moncrt0.o mon.o init_shlibs.o

mon.o:
	cc -MD -O -S ${DEFS} ${DFLAGS} mon.c
	ex - mon.s < mon.ex
	as -o mon.o mon.s
	ld -X -r -o x.o mon.o
	${MV} x.o mon.o
	rm -f x.o mon.s
	md -d -u Makedep mon.d

tags:	
	cwd=`pwd`; \
	for i in ${SRCS}; do \
		ctags -a -f ${TAGSFILE} $$cwd/$$i; \
	done

clean:
	rm -f ${OBJS} *.o core errs tags Makedep

-include Makedep

