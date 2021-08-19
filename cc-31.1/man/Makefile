OTHER_SRCS = Makefile
MAN1 = ar.1 atom.1 bison.1 file.1 gcc.1 gdb.1 gprof.1 ld.1 nm.1 otool.1 \
       ranlib.1 size.1 strings.1 strip.1 segedit.1 ebadexec.1
MAN3 = rld.3
MANL = cmpshlib.l indr.l mkshlib.l

MANDIR = /usr/local/doc/cc_proj/man

install: ${DSTROOT}${MANDIR} ${MAN1} ${MAN3} ${MANL}
	install -m 444 ${IFLAGS} ${MAN1} ${MAN3} ${MANL} ${DSTROOT}${MANDIR}

${DSTROOT}${MANDIR}:; mkdirs $@

depend:

clean:

all:
