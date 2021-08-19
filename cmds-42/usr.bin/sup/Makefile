#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCTS= sup supnamesrv supfilesrv supscan
BINDIR=	$(DSTROOT)/usr/bin
ETCDIR=	$(DSTROOT)/usr/etc
DSTDIRS= $(BINDIR)

CDEFS=	-DNeXT_MOD -D__STRICT_BSD__ -DDEFDIR=\"/usr\" -DDEFUSER=\"anon\"
IFLAGS= -s
CFLAGS=	-O -g -fwritable-strings $(CDEFS)
IFLAGS= -s
LIBS= -lcs -lsys_s

HFILES=	sup.h supmsg.h
CFILES= msgcrypt.c msgdeny.c msggoaway.c msglist.c msglogin.c msgname.c \
	msgneed.c msgrefuse.c msgsetup.c msgsignon.c msgxfer.c scan.c \
	scm.c scmio.c stree.c sup.c supfilesrv.c supnamesrv.c supscan.c \
	supvers.c
SFILES=
YFILES=

DERIVED=
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

all:	$(PRODUCTS)

install: DSTROOT $(DSTDIRS) all
	install ${IFLAGS} sup.NEW $(BINDIR)/sup
	install ${IFLAGS} supfilesrv.NEW $(ETCDIR)/supfilesrv
	install ${IFLAGS} supnameserv.NEW $(ETCDIR)/supnameserv
	install ${IFLAGS} supscan.NEW $(BINDIR)/supscan

#
# Building sup
#
SUP_OBJS= sup.o supvers.o scm.o scmio.o stree.o msggoaway.o msgsignon.o \
	msgname.o msgsetup.o msgcrypt.o msglogin.o msgrefuse.o msglist.o \
	msgneed.o msgdeny.o msgxfer.o

sup.NEW:	${SUP_OBJS} sup_vers.o
	$(CC) $(CFLAGS) -o $@ ${SUP_OBJS} sup_vers.o ${LIBS}

sup_vers.c: ${DOVERS} ${SUP_OBJS}
	@rm -f $@
	vers_string -c sup > $@

#
# Building supnameserv
#
SUPNAMESERV_OBJS= supnameserv.o scm.o scmio.o stree.o msggoaway.o \
	msgsignon.o msgname.o

supnameserv.NEW:	${SUPNAMESERV_OBJS} supnameserv_vers.o
	$(CC) $(CFLAGS) -o $@ ${SUPNAMESERV_OBJS} supnameserv_vers.o ${LIBS}

supnameserv_vers.c: ${DOVERS} ${SUPNAMESERV_OBJS}
	@rm -f $@
	vers_string -c supnameserv > $@

#
# Building supfilesrv
#
SUPFILESRV_OBJS= supfilesrv.o scm.o scmio.o stree.o scan.o msggoaway.o \
	msgsignon.o msgsetup.o msgcrypt.o msglogin.o msgrefuse.o msglist.o \
	msgneed.o msgdeny.o msgxfer.o

supfilesrv.NEW:	${SUPFILESRV_OBJS} supfilesrv_vers.o
	$(CC) $(CFLAGS) -o $@ ${SUPFILESRV_OBJS} supfilesrv_vers.o ${LIBS}

supfilesrv_vers.c: ${DOVERS} ${SUPFILESRV_OBJS}
	@rm -f $@
	vers_string -c supfilesrv > $@

#
# Building supscan
#
SUPSCAN_OBJS= supscan.o stree.o scan.o

supscan.NEW:	${SUPSCAN_OBJS} supscan_vers.o
	$(CC) $(CFLAGS) -o $@ ${SUPSCAN_OBJS} supscan_vers.o ${LIBS}

supscan_vers.c: ${DOVERS} ${SUPSCAN_OBJS}
	@rm -f $@
	vers_string -c supscan > $@


#
# Standard targets
#
$(PRODUCTS):	Makedep $$@.NEW

clean:	ALWAYS
	-rm -f *.o *.NEW TAGS tags vers.c Makedep $(PRODUCTS) $(DERIVED) \
		$(GARBAGE)

DSTROOT:
	@if [ -n "$($@)" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

$(DSTDIRS):
	mkdirs $@

depend Makedep: $(DERIVED)
	rm -f Makedep
	$(CC) -M $(CDEFS) $(CFILES) $(SFILES) $(DERIVED) | \
	    awk '{ if ($$1 != prev) { if (rec != "") print rec; \
		rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		else rec = rec " " $$2 } } \
		END { print rec }' >Makedep

tags:	ALWAYS
	etags $(SRCS)
	ctags $(SRCS)

ALWAYS:

-include Makedep
