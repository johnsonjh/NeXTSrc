# Note that OTHER_SRCES do NOT get installed into /usr/include
OTHER_SRCS = notes a.out.h symseg.h sys/exec.h
FILES = ldsyms.h reloc.h rld.h
INCDIR = /usr/include

install: ${DSTROOT}${INCDIR} ${FILES}
	install -m 444 ${IFLAGS} ${FILES} ${DSTROOT}${INCDIR}

local: LCLROOT ${LCLROOT}${INCDIR} ${FILES}
	install -m 444 ${IFLAGS} ${FILES} ${LCLROOT}${INCDIR}

${LCLROOT}${INCDIR} ${DSTROOT}${INCDIR}:; mkdirs $@

depend:

clean:

all:
