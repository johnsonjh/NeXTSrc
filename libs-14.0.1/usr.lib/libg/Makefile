#
#	@(#)Makefile	5.1	(Berkeley)	85/07/12
#
LIBDIR=$(DSTROOT)/usr/lib

all: libg.a

libg.a:	dbxxx.s
	as dbxxx.s -o libg.a

install: DSTROOT libg.a ${LIBDIR}
	install -c -m 444 libg.a ${LIBDIR}

DSTROOT:
	@if [ -n "$($@)" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

${LIBDIR}:
	mkdirs $@

clean:
	rm -f libg.a

depend tags:
