#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
DIR= tn3270
PRODUCTS= tn3270 mset
BINDIR=	$(DSTROOT)/usr/ucb
CONFDIR= $(DSTROOT)/private/etc
DSTDIRS= $(BINDIR) $(CONFDIR)
SUBDIRS= api ascii ctlr general sys telnet

CDEFS=	-DNeXT_MOD -D__STRICT_BSD__
CFLAGS=	-O -g -fwritable-strings $(CDEFS)
IFLAGS= -s
LIBS= -lcurses -ltermlib

HFILES=	
M4FILES= 
CFILES= 
SFILES=
YFILES=

DERIVED=
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

all:	subdirs tn3270.NEW mset.NEW map3270

subdirs: ALWAYS
	@echo '#################' make all for $(DIR) '###################'
	@for i in ${SUBDIRS}; \
	do \
		echo ================= make all for $$i =================; \
		(cd $$i; ${MAKE} all || exit 1); \
	done
	@echo '#################' make all for $(DIR) done '################'

clean:
	@echo '#################' make $@ for $(DIR) '###################'
	@for i in ${SUBDIRS}; \
	do \
		echo ================= make $@ for $$i =================; \
		(cd $$i; ${MAKE} $@ || exit 1); \
	done
	@echo '#################' make $@ for $(DIR) done '################'
	rm -f *.NEW

install: DSTROOT $(DSTDIRS) all
	install ${IFLAGS} tn3270.NEW $(BINDIR)/tn3270
	install ${IFLAGS} mset.NEW $(BINDIR)/mset
	rm -f $(CONFDIR)/map3270
	install -c -m 444 map3270 $(CONFDIR)/map3270

#
# Building tn3270
#
API_OBJS = api/apilib.o api/api_bsd.o api/api_exch.o api/astosc.o	\
	api/asc_ebc.o api/dctype.o api/disp_asc.o api/ebc_disp.o

ASCII_OBJS = ascii/map3270.o ascii/termin.o

CTLR_OBJS = ctlr/api.o ctlr/inbound.o ctlr/oia.o ctlr/options.o		\
	ctlr/outbound.o

GENERAL_OBJS = general/globals.o general/genbsubs.o

SYS_OBJS = sys/system.o sys/termout.o

TELNET_OBJS = telnet/telnet.o telnet/commands.o telnet/main.o 		\
	telnet/network.o telnet/ring.o telnet/sys_bsd.o 		\
	telnet/terminal.o telnet/tn3270.o telnet/utilities.o

TN3270_OBJS= ${API_OBJS} ${ASCII_OBJS} ${CTLR_OBJS} ${GENERAL_OBJS}	\
	${SYS_OBJS} ${TELNET_OBJS}

tn3270.NEW:	all ${SUBDIRS} tn3270_vers.o
	$(CC) $(CFLAGS) -o $@ ${TN3270_OBJS} tn3270_vers.o ${LIBS}

link:
	$(CC) $(CFLAGS) -o $@ ${TN3270_OBJS} tn3270_vers.o ${LIBS}

tn3270_vers.c: ${DOVERS} ${SUBDIRS}
	@rm -f $@
	vers_string -c tn3270 > $@

#
# Building mset
#
MSET_OBJS= ascii/mset.o ascii/map3270.o api/astosc.o

mset.NEW:	${MSET_OBJS} mset_vers.o
	$(CC) $(CFLAGS) -o $@ ${MSET_OBJS} mset_vers.o ${LIBS}

mset_vers.c: ${DOVERS} ${MSET_OBJS}
	@rm -f $@
	vers_string -c mset > $@

#
# Standard targets
#
$(PRODUCTS):	$$@.NEW


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


tags:	ALWAYS
	etags $(SRCS)
	ctags $(SRCS)

ALWAYS:


