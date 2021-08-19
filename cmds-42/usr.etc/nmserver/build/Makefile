#
# Mach Operating System
# Copyright (c) 1987 Carnegie-Mellon University
# All rights reserved.  The CMU software License Agreement specifies
# the terms and conditions for use and redistribution.
#
#
# $Source: /private/Net/p1/BSD/btaylor/LIBC/libc-2/etc/nmserver/build/RCS/Makefile,v $
#
# $Header: Makefile,v 1.1 88/09/30 15:21:18 gk Locked $
#
# Makefile for the network server and all associated programs.
#
# HISTORY
# 13-Apr-90  Morris Meyer (mmeyer) at NeXT
#	Added .d files to the clean list.
#
#  7-Apr-88  Mary Thompson (mrt) at Carnegie Mellon
#	Changed dependency rules to put dependencies into Make.dep
#	instead of the Makefile. Added -O switch to CLFAGS
#
# 25-Feb-88  Mary Thompson (mrt) at Carnegie Mellon
#	Modified to use DSTBASE and changed where to find camelot.o
#	cut down the number of programs in UTILPROG
#
# 17-Jan-88  Daniel Julin (dpj) at Carnegie-Mellon University
#	Added libmach.a in rule for linking nmtime.
#
# 14-Dec-87  Daniel Julin (dpj) at Carnegie-Mellon University
#	Updated for RPCMOD, TCP module, trint_test, nmtest, vmtptest, nmtime.
#
# 14-Oct-87  Robert Sansom (rds) at Carnegie Mellon University
#	Added nmxportstat - program to collate sets of port statistics.
#
#  6-Aug-87  Robert Sansom (rds) at Carnegie Mellon University
#	Added crypt_newdes.[co] and des.[co].
#
# 30-Jul-87  Daniel Julin (dpj) at Carnegie-Mellon University
#	Cleaned up a bit.
#
# 10-Jun-87  Daniel Julin (dpj) at Carnegie-Mellon University
#	Added vmtp1.{c,o}
#
# 13-Apr-87  Robert Sansom (rds) at Carnegie Mellon University
#	Munged many program names plus other things.
#
# 30-Mar-87  Daniel Julin (dpj) at Carnegie-Mellon University
#	Removed a couple of extraneous calls to 'lint'.
#
# 24-Mar-87  Daniel Julin (dpj) at Carnegie-Mellon University
#	Created.
#

#   Mach library stuff

DSTBASE=${DSTROOT}
INSTALL_DIR =	$(DSTBASE)/usr/etc
CONF_DIR = 	$(DSTBASE)/private/etc
DEP_FILE=	Makedep
I=	-d -c -s


#
# Programs to use
#
CC=	cc
# CC=	hc	# for RT
MIG=	mig

#
# Flags for running various programs
#

#
# Kernel include files.
#
KERNELINCS=	# -I$(DSTBASE)/include -I$(DSTBASE)/include/sys

INCLUDES=	-I. -I../server # -I../include $(KERNELINCS)
DEPENDS=	-MD
#CFLAGS=		-O -g -pg -DPROF $(DEPENDS) $(INCLUDES) -bsd
CFLAGS=		-O -g $(DEPENDS) $(INCLUDES) -bsd
MIGFLAGS=	$(DEPENDS) $(INCLUDES) -DMIG -DNMSERVER
#LDFLAGS= 	-pg -g
LDFLAGS= 	-sectorder __TEXT __text callf.order
LIBS=		-lcs -lsys_s

#
# Suffix for dependency files
#
MD_SUFFIX=d

#
# Directories where to find stuff
#
MIGSRC= /usr/include/servers
SRC=	../server
UTILS=	../utils

#
# Main program name
#
NM=		nmserver

#
# Things to process with mig
#
MIGFILES=	netname_user.c netname_server.c km_user.c km_server.c \
		logstat_user.c logstat_server.c
MIGDEFS=	$(MIGSRC)/netname.defs ../server/km.defs ../server/logstat.defs
MIGOBJS=	km_server.o netname_server.o logstat_server.o

#
# Normal object files for the network server, built form sources in $(SRC)
#
# For the non RPCMOD version, remove ipc_rpc.o, ipc_exception.o, ipc_msg.o,
# vmtp2.o, tcp.o and add ipc_in.o, ipc_out.o, vmtp1.o.
#
# For the non CAMELOT version, remove cam_stuff.o
#
OBJS=	dispatcher.o lock_queue.o mem.o nm_extra.o rwlock.o sbuf.o timer.o uid.o \
	crypt.o crypt_multperm.o crypt_newdes.o crypt_xor.o multperm.o \
	ipc.o ipc_block.o ipc_msg.o ipc_rpc.o ipc_exception.o ipc_swap.o \
	keyman.o km_procs.o km_utils.o \
	netname.o nn_handler.o nn_procs.o \
	portrec.o portcheck.o pc_handler.o pc_startup.o portsearch.o ps_auth.o \
	portops.o po_message.o po_notify.o po_utils.o \
	datagram.o deltat.o deltat_handler.o deltat_send.o deltat_utils.o \
	srr.o srr_handler.o srr_utils.o netipc.o network.o transport.o \
	nm_init.o logstat.o ls_rec.o vmtp2.o nm_enable.o crypt_des.o des.o \
	tcp.o cam_stuff.o

#
# Normal C files.
#
CFILES=	$(SRC)/dispatcher.c $(SRC)/lock_queue.c $(SRC)/mem.c $(SRC)/nm_extra.c \
	$(SRC)/rwlock.c $(SRC)/sbuf.c $(SRC)/timer.c $(SRC)/uid.c \
	$(SRC)/crypt.c $(SRC)/crypt_multperm.c $(SRC)/crypt_newdes.c \
	$(SRC)/crypt_xor.c $(SRC)/multperm.c \
	$(SRC)/ipc.c $(SRC)/ipc_block.c $(SRC)/ipc_msg.c $(SRC)/ipc_rpc.c \
	$(SRC)/src/ipc_exception.c $(SRC)/ipc_swap.c \
	$(SRC)/keyman.c $(SRC)/km_procs.c $(SRC)/km_utils.c \
	$(SRC)/netname.c $(SRC)/nn_handler.c $(SRC)/nn_procs.c \
	$(SRC)/portrec.c $(SRC)/portcheck.c $(SRC)/pc_handler.c \
	$(SRC)/pc_startup.c $(SRC)/portsearch.c $(SRC)/ps_auth.c \
	$(SRC)/portops.c $(SRC)/po_message.c $(SRC)/po_notify.c $(SRC)/po_utils.c \
	$(SRC)/datagram.c $(SRC)/deltat.c $(SRC)/deltat_handler.c \
	$(SRC)/deltat_send.c $(SRC)/deltat_utils.c \
	$(SRC)/srr.c $(SRC)/srr_handler.c $(SRC)/srr_utils.c $(SRC)/netipc.c \
	$(SRC)/network.c $(SRC)/transport.c \
	$(SRC)/nm_init.c $(SRC)/logstat.c $(SRC)/ls_rec.c \
	$(SRC)/vmtp2.c $(SRC)/nm_enable.c $(SRC)/crypt_des.c $(SRC)/des.c \
	$(SRC)/tcp.c $(SRC)/cam_stuff.c
#
# All object files for the network server.
#
ALLOBJS=	$(OBJS) $(MIGOBJS)

#
# Various sets of interesting programs
#
NMTEST=		ptest ttest trint_test
NMTESTOBJS=	port_test.o trans_test.o trint_test.o
NMTESTFILES=	$(SRC)/port_test.c $(SRC)/trans_test.c $(SRC)/trint_test.c
UTILPROG=	nmcontrol nmxlog nmtest nmtime
UTILOBJS=	nmcontrol.o nmxlog.o nmxportstat.o km_test.o nmtest.o \
		vmtptest.o nmtime.o
UTILFILES=	$(UTILS)/nmcontrol.c $(UTILS)/nmxlog.c $(UTILS)/nmxportstat.c \
		$(UTILS)/km_test.c $(UTILS)/nmtest.c $(UTILS)/vmtptest.c \
		$(UTILS)/nmtime.c


#
# Major program-generation targets
#
default:	$(NM)

utilprog:	$(UTILPROG)

all:		$(NM) $(NMTEST) $(UTILPROG) nmthreads

install:	DSTROOT default $(INSTALL_DIR) $(CONF_DIR)
	install $I $(NM) $(INSTALL_DIR)/nmserver
	ln -s ../../usr/etc/nmserver $(CONF_DIR)
	install -c -m 644 ../conf/NETMSG_CONFIG $(CONF_DIR)/NETMSG_CONFIG

$(INSTALL_DIR) $(CONF_DIR):
	mkdirs $@

DSTROOT:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

#
# How to link all the programs
#
$(NM):	$(NM).o $(ALLOBJS)
	cc $(LDFLAGS) -o $(NM) $(NM).o $(ALLOBJS) $(LIBS)

nmthreads:	$(NM).o $(ALLOBJS)
	$(LD) $(LDFLAGS) -o nmthreads /lib/crt0.o $(NM).o $(ALLOBJS) ../lib/kds_user.o $(LIBS)

ptest: port_test.o $(ALLOBJS)
	$(LD) $(LDFLAGS) -o ptest /lib/crt0.o port_test.o $(ALLOBJS) $(LIBS)

ttest: trans_test.o $(ALLOBJS)
	$(LD) $(LDFLAGS) -o ttest /lib/crt0.o trans_test.o $(ALLOBJS) $(LIBS)

trint_test: trint_test.o $(ALLOBJS)
	$(LD) $(LDFLAGS) -o trint_test /lib/crt0.o trint_test.o $(ALLOBJS) $(LIBS)

km_test: km_test.o km_user.o multperm.o ../lib/kds_server.o
	$(LD) $(LDFLAGS) -o km_test /lib/crt0.o km_test.o km_user.o multperm.o ../lib/kds_server.o $(LIBS)

nmcontrol: nmcontrol.o logstat_user.o ls_rec.o
	$(LD) $(LDFLAGS) -o nmcontrol /lib/crt0.o nmcontrol.o logstat_user.o ls_rec.o $(LIBS)

nmxlog: nmxlog.o
	$(LD) $(LDFLAGS) -o nmxlog /lib/crt0.o nmxlog.o $(LIBS)

nmxportstat: nmxportstat.o ls_rec.o
	$(LD) $(LDFLAGS) -o nmxportstat /lib/crt0.o nmxportstat.o $(LIBS)

nmtest: nmtest.o
	$(LD) $(LDFLAGS) -o nmtest /lib/crt0.o nmtest.o $(LIBS)

vmtptest: vmtptest.o
	$(LD) $(LDFLAGS) -o vmtptest /lib/crt0.o vmtptest.o ../lib/libvmtp.a $(LIBS)

nmtime: nmtime.o
	$(LD) $(LDFLAGS) -o nmtime /lib/crt0.o nmtime.o $(LIBS)

#
# Generic rules
#
$(OBJS): $(SRC)/$$(@:.o=.c)
	$(CC) $(CFLAGS) -c $(SRC)/$*.c
	md -u $(DEP_FILE) -d $*.d

$(NM).o: $(SRC)/$(NM).c
	$(CC) $(CFLAGS) -c $(SRC)/$(NM).c
	md -u $(DEP_FILE) -d $*.d

$(NMTESTOBJS): $(SRC)/$$(@:.o=.c)
	$(CC) $(CFLAGS) -c $(SRC)/$*.c
	md -u $(DEP_FILE) -d $*.d

$(UTILOBJS): $(UTILS)/$$(@:.o=.c)
	$(CC) $(CFLAGS) -c $(UTILS)/$*.c
	md -u $(DEP_FILE) -d $*.d

$(MIGOBJS): $$(@:.o=.c)
	$(CC) $(CFLAGS) -c $*.c
	md -u $(DEP_FILE) -d $*.d

#
# Rules for mig
#
netname_user.c netname_server.c netname.h: $(MIGSRC)/netname.defs
	$(MIG) $(MIGFLAGS)  $(MIGSRC)/netname.defs \
		-server netname_server.c \
		-user netname_user.c

km_user.c km_server.c km.h: ../server/km.defs
	$(MIG) $(MIGFLAGS) ../server/km.defs \
		-server km_server.c \
		-user km_user.c

logstat_server.c logstat_user.c logstat.h: ../server/logstat.defs
	$(MIG) $(MIGFLAGS) ../server/logstat.defs \
		-server logstat_server.c \
		-user logstat_user.c

#
# Misc targets
#
tags:
	/usr/ucb/ctags ${CFILES}

clean:
	rm -f eddep tags *.o *.d $(MIGFILES) $(NM) $(UTILPROG) $(TESTPROG) \
		Makedep errs linterrs makelinks *.log *LOG*


-include $(DEP_FILE)





