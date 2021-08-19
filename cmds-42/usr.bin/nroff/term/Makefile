#	Makefile	4.6	86/05/15
#
# Possible options:
#	make		compile source for terminal tables below
#	make <term>	compile table for a specific <term>
#	make install	move tables to ${TERMDIR}
#	make links	make links for ease of documentation
#	make clean	remove tab*.o files lying around

CFLAGS= -fwritable-strings

ALL=	tab37 tablpr tab300 tab300-12 tab302 tab302-12 tab382 tab382-12 \
	tab450 tab450-12 tab833 tab833-12 tabepson tabitoh tabitoh12 \
	tabnec tabnec12 tabnec-t tabnec25-t tabqume tabqume12 tabxerox \
	tabxerox12 tabx-ecs tabx-ecs12

TERMDIR=	${DSTROOT}/usr/lib/term

all:	${ALL}

${ALL}:	$$@.desc maketerm.c
	rm -f term.desc
	cp $@.desc term.desc
	cc -o maketerm maketerm.c
	./maketerm $@

install: ckdstroot ${TERMDIR} all
	install -m 644 ${ALL} ${TERMDIR}
	make DSTROOT=${DSTROOT} links
	install -c -m 644 README ${TERMDIR}/README

ckdstroot:
	@if [ -n "${DSTROOT}" ]; \
	then \
		exit 0; \
	else \
		echo Must define DSTROOT; \
		exit 1; \
	fi

${TERMDIR}:
	mkdirs $@

links:
	rm -f ${TERMDIR}/tabtn300; \
		ln ${TERMDIR}/tablpr ${TERMDIR}/tabtn300
	rm -f ${TERMDIR}/tabcrt; \
		ln ${TERMDIR}/tablpr ${TERMDIR}/tabcrt
	rm -f ${TERMDIR}/tab300s; \
		ln ${TERMDIR}/tab302 ${TERMDIR}/tab300s
	rm -f ${TERMDIR}/tab300s-12; \
		ln ${TERMDIR}/tab302-12 ${TERMDIR}/tab300s-12
	rm -f ${TERMDIR}/tabdtc; \
		ln ${TERMDIR}/tab302 ${TERMDIR}/tabdtc
	rm -f ${TERMDIR}/tabdtc12; \
		ln ${TERMDIR}/tab302-12 ${TERMDIR}/tabdtc12
	rm -f ${TERMDIR}/tabipsi; \
		ln ${TERMDIR}/tab450 ${TERMDIR}/tabipsi
	rm -f ${TERMDIR}/tabipsi12; \
		ln ${TERMDIR}/tab450-12 ${TERMDIR}/tabipsi12

clean:
	rm -f term.desc errs maketerm $(ALL)

depend:
