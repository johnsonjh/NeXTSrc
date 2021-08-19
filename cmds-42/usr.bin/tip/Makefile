#
# Copyright (c) 1983 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.2 (Berkeley) 4/3/86
#
# make file for intermachine communications package
#
# Files are:
#	/etc/remote		remote host description file
#	/etc/phones		phone number file, owned by ${OWNER} and
#				  mode 6??
#	${ADMDIR}/aculog	ACU accounting file, owned by ${OWNER} and
#				  mode 6?? {if ACULOG defined}
# Presently supports:
#	BIZCOMP
#	DEC DF02-AC, DF03-AC
#	DEC DN-11/Able Quadracall
#	HAYES and Hayes emulators
#	USR COURIER (2400 baud)
#	VENTEL 212+
#	VADIC 831 RS232 adaptor
#	VADIC 3451
# (drivers are located in aculib.a)
#
# Configuration defines:
#	DF02, DF03, DN11	ACU's supported
#	  BIZ1031, BIZ1022, VENTEL, V831, V3451, HAYES, COURIER
#	ACULOG			turn on tip logging of ACU use
#	PRISTINE		no phone #'s put in ACU log file
#	CONNECT			worthless command
#	DEFBR			default baud rate to make connection at
#	DEFFS			default frame size for FTP buffering of
#				writes on local side
#	BUFSIZ			buffer sizing from stdio, must be fed
#				explicitly to remcap.c if not 1024
#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCT= tip
BINDIR=	$(DSTROOT)/usr/bin
ADMDIR=$(DSTROOT)/private/adm
CONFDIR= $(DSTROOT)/private/etc
DSTDIRS= $(BINDIR) $(ADMDIR) $(CONFDIR)

# Doing file "takes" is real-time -- we need cycles
HIGHPRI= -DHIGHPRI=-20
# If HIGHPRI is defined, owner must be root to allow setpriority
#OWNER=	uucp
OWNER=	root
GROUP=	daemon

CDEFS=	-DNeXT_MOD -D__STRICT_BSD__ -DDEFBR=1200 -DDEFFS=BUFSIZ -DACULOG -DPRISTINE -DBIG_ENDIAN -DV831 -DVENTEL -DHAYES -DCOURIER ${HIGHPRI}
CFLAGS=	-O -g -fwritable-strings $(CDEFS)
IFLAGS= -s
LIBS= aculib/aculib.a

HFILES=
YFILES=
CFILES= acu.c cmds.c cmdtab.c cu.c hunt.c log.c partab.c remote.c \
	tip.c tipout.c value.c vars.c acutab.c remcap.c uucplock.c
SFILES=

DERIVED=
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

all:	$(PRODUCT)

install: DSTROOT $(DSTDIRS) all symlinks
	install -s -m 4711 -o ${OWNER} -g ${GROUP} tip.NEW ${BINDIR}/tip
	rm -f ${BINDIR}/cu
	ln ${BINDIR}/tip ${BINDIR}/cu
	rm -f ${ADMDIR}/aculog
	touch -f ${ADMDIR}/aculog
	chown ${OWNER} ${ADMDIR}/aculog
	chmod 600 ${ADMDIR}/aculog
	install -c -m 644 remote-file $(CONFDIR)/remote

symlinks:
	@if [ ! -h ${DSTROOT}/usr/adm ] ; \
	then \
		echo "ln -s ../private/adm ${DSTROOT}/usr/adm" ; \
		ln -s ../private/adm ${DSTROOT}/usr/adm ; \
	fi
	@if [ ! -h ${DSTROOT}/etc ] ; \
	then \
		echo "ln -s private/etc ${DSTROOT}/etc" ; \
		ln -s private/etc ${DSTROOT}/etc ; \
	fi

#
# Building PRODUCT
#
acutab.o remote.o log.o: Makefile

$(PRODUCT).NEW:	$(OBJS) $(LIBS) vers.o
	$(CC) $(CFLAGS) -o $@ $(OBJS) vers.o $(LIBS) -lsys_s

vers.c: $(DOVERS) $(OBJS)
	@rm -f $@
	vers_string -c $(PRODUCT) > $@

aculib/aculib.a: ALWAYS
	cd aculib; ${MAKE}

#
# Standard targets
#
$(PRODUCT):	Makedep $$@.NEW

clean:	ALWAYS
	-rm -f *.o *.NEW TAGS tags vers.c Makedep $(PRODUCT) $(DERIVED) \
		$(GARBAGE)
	cd aculib; ${MAKE} clean

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
