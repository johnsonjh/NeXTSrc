OTHER_SRCS = Makefile
NOTES = CProgramming.rtf

NOTESDIR = /usr/local/RelNotes

install: ${DSTROOT}${NOTESDIR} ${NOTES}
	install -m 444 ${IFLAGS} ${NOTES} ${DSTROOT}${NOTESDIR}

${DSTROOT}${NOTESDIR}:; mkdirs $@

depend:

clean:

all:
