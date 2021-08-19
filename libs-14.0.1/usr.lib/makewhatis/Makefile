#
# Generic top-level makefile for shell scripts
# (c) NeXT, Inc.  1987
#
IFLAGS= -xs -m 555

PRODUCT= makewhatis

DSTDIR=
BINDIR=	$(DSTDIR)/usr/lib

FILES=	makewhatis.sh Makefile Makedep
GARBAGE=

all:	${PRODUCT}.NEW

${PRODUCT}.NEW:	${PRODUCT}.sh ${DOVERS}
	sed -e "s/#PROGRAM.*/#`vers_string ${PRODUCT}`/" \
	    <${PRODUCT}.sh >${PRODUCT}.NEW

depend tags:

clean:	ALWAYS
	-rm -f ${GARBAGE} *.NEW

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

ALWAYS:
