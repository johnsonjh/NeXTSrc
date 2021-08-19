MANDIR=${DSTROOT}/NextLibrary/Documentation/Unix/ManPages
MANDIRS=${MANDIR}/man1 ${MANDIR}/man5

IFLAGS=-c -d

all clean depend tags:

install:	DSTROOT ${MANDIRS}
		install $(IFLAGS) *.1* ${MANDIR}/man1
		install $(IFLAGS) *.5* ${MANDIR}/man5

print:
		$(TROFF) -man *.1* *.5*

${MANDIRS}:
	mkdirs $@

DSTROOT:
	@if [ -n "$@" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi
