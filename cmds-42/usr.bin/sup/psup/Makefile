#
######################################################################
# HISTORY
# 05-Jan-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
#	Added stree.o to supnamesrv files.
#
# 29-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
#	Added msg* files.
#
######################################################################

DSTDIR= /bootdisk/NeXT/Unix
BINDIR=	${DSTROOT}${DSTDIR}/usr/bin
ETCDIR=	${DSTROOT}${DSTDIR}/usr/etc

CFLAGS= -DDEFDIR=\"/usr/cs\" -DDEFUSER=\"anon\" -fwritable-strings -D__STRICT_BSD__
LINT=	lint -n -u -DCMU -DCMUCS

MBOTH=	msggoaway.o msgsignon.o
MNAME=	msgname.o
MFILE=	msgsetup.o msgcrypt.o msglogin.o msgrefuse.o msglist.o msgneed.o msgdeny.o msgxfer.o
SUPM=	${MBOTH} ${MNAME} ${MFILE}
SUPNM=	${MBOTH} ${MNAME}
SUPFM=	${MBOTH} ${MFILE}
SUPS=	scm.o scmio.o stree.o
ALL=	sup supnamesrv supfilesrv supscan
LALL=	sup.lint supnamesrv.lint supfilesrv.lint supscan.lint

all:	${ALL}
	@echo "\`sup' system is up to date."

sup:	sup.o supvers.o ${SUPS} ${SUPM}
	${CC} ${CFLAGS} -o sup sup.o supvers.o ${SUPS} ${SUPM}

supnamesrv: supnamesrv.o ${SUPS} ${SUPNM}
	${CC} ${CFLAGS} -o supnamesrv supnamesrv.o ${SUPS} ${SUPNM}

supfilesrv: supfilesrv.o ${SUPS} scan.o ${SUPFM}
	${CC} ${CFLAGS} -o supfilesrv supfilesrv.o ${SUPS} scan.o ${SUPFM}

supscan: supscan.o stree.o scan.o
	${CC} ${CFLAGS} -o supscan supscan.o stree.o scan.o

install: ${ALL}
	install -s sup ${BINDIR}
	install -s supnamesrv ${ETCDIR}
	install -s supfilesrv ${ETCDIR}
	install -s supscan ${BINDIR}

lint:	${LALL}

sup.lint:
	${LINT} ${CFLAGS} sup.c scm.c scmio.c stree.c

supnamesrv.lint:
	${LINT} ${CFLAGS} supnamesrv.c scm.c scmio.c stree.c

supfilesrv.lint:
	${LINT} ${CFLAGS} supfilesrv.c scm.c scmio.c stree.c scan.c

supscan.lint:
	${LINT} ${CFLAGS} supscan.c stree.c scan.c

${SUPM}: sup.h supmsg.h
scm.o:	sup.h
scmio.o: sup.h supmsg.h
stree.o: sup.h
scan.o:	sup.h
sup.o:	sup.h supmsg.h
supnamesrv.o: sup.h supmsg.h
supfilesrv.o: sup.h supmsg.h
supscan.o: sup.h

clean:
	rm -f *.o ${ALL}

depend:
	@echo No depend target, make depend ignored
