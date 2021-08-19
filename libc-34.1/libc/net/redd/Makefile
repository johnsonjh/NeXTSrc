#
# Copyright (c) 1983 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	6.3 (Berkeley) 3/18/86
#

OBJS=	gethostnamadr.o sethostent.o

SRCS=	gethostnamadr.c sethostent.c

DEFS=
CFLAGS= -O -g ${DEFS}  -fwritable-strings -I../../../usr.include
TAGSFILE= tags
MV=	mv -f

all:	hostlib hostlib_p

shlib_ofiles:

.c.o:
	${CC} -p ${CFLAGS} -c $< -o profiled/$@
	${CC} ${CFLAGS} -c $<

hostlib hostlib_p: .depend_done ${OBJS} 
	@echo "building profiled hostlib"
	@rm -f hostlib_p
	@cd profiled; ar cru ../hostlib_p ${OBJS}
	@echo "building normal hostlib"
	@rm -f hostlib
	@ar cru hostlib ${OBJS}
	
tags:
	cwd=`pwd`; \
	for i in ${SRCS}; do \
		ctags -a -f ${TAGSFILE} $$cwd/$$i; \
	done

clean:
	rm -f *.o errs a.out core hostlib hostlib_p profiled/*.o \
		tags Makefile.bak
	rm -f Makedep .depend_done
	touch Makedep

depend .depend_done:
	rm -f Makedep .depend_done
	cc -M ${CFLAGS} ${SRCS} | awk ' { if ($$1 != prev) \
	    { if (rec != "") print rec; rec = $$0; prev = $$1; } \
	    else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
	    else rec = rec " " $$2 } } \
	    END { print rec } ' > Makedep
	-touch .depend_done

-include Makedep
