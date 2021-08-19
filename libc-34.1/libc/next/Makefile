#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.5 (Berkeley) 9/5/85
#
# Machine dependent routines for the VAX are located here
#
COMPAT=	compat-4.1
#ALL=	ansi gen net stdio sys ${COMPAT} cs
ALL=	ansi gen net stdio sys ${COMPAT}
TAGSFILE=tags

all:	nextlib nextlib_p
	
nextlib nextlib_p: ${ALL}
	@for i in ${ALL}; \
	do \
		echo ================ make all for next/$$i ================; \
		(cd $$i; $(MAKE) ${MFLAGS} DEFS=${DEFS}); \
	done
	rm -rf tmp
	-mkdir tmp
	for i in ${ALL}; do (cd tmp; ar x ../$$i/$${i}lib); done
	rm -f nextlib
	ar cr nextlib `ls tmp/*.o | sort -t/ +1`
	rm -f tmp/*
	for i in ${ALL}; do (cd tmp; ar x ../$$i/$${i}lib_p); done
	rm -f nextlib_p
	ar cr nextlib_p `ls tmp/*.o | sort -t/ +1`
	rm -rf tmp

shlib_ofiles:
	@for i in ${ALL}; \
	do \
		echo ================= make $@ for next/$$i =================; \
		(cd $$i; $(MAKE) ${MFLAGS} DEFS=${DEFS} \
		 $@ SHLIBIDIR=${SHLIBIDIR}); \
	done

clean:
	@for i in ${ALL}; \
	do \
		echo ================= make $@ for next/$$i =================; \
		(cd $$i; $(MAKE) $@); \
	done
	rm -f nextlib nextlib_p tmp/*.o;
	-rmdir tmp

depend tags:
	@for i in ${ALL}; \
	do \
		echo ================= make $@ for next/$$i =================; \
		(cd $$i; $(MAKE) $@ ); \
	done

