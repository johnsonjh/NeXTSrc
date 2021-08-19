#
# Copyright (c) 1985 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	6.2 (Berkeley) 5/28/85
#
#
#  Makefile for -man macros
#

SRCS=	tmac.an tmac.an.new tmac.an6n tmac.an6t
DSTDIR=
TMACDIR=${DSTROOT}${DSTDIR}/usr/lib/tmac

all:	${SRCS}

install:ckdstroot ${SRCS}
	for i in ${SRCS}; do (install -c $$i ${TMACDIR}/$$i); done

ckdstroot:
	@if [ -n "${DSTROOT}" ]; \
	then \
		exit 0; \
	else \
		echo Must define DSTROOT; \
		exit 1; \
	fi

${TMACDIR}:
	mkdirs $@

clean depend:
	@echo No $@ target, make $@ ignored
