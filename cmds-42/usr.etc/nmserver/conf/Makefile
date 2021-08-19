#
# Mach Operating System
# Copyright (c) 1987 Carnegie-Mellon University
# All rights reserved.  The CMU software License Agreement specifies
# the terms and conditions for use and redistribution.
#
#
# $Source: /os/osdev/LIBS/libs-7/etc/nmserver/conf/RCS/Makefile,v $
#
# $Header: Makefile,v 1.1 88/09/30 17:44:16 osdev Exp $
#
# Makefile for the network server and all associated programs.
#
# HISTORY
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

DSTBASE=	$(DSTROOT)/usr/mach
INSTALL_DIR = $(DSTBASE)/etc
DEP_FILE=	Makedep
I=	-d -c


#
# Programs to use
#
CC=	cc
# CC=	hc	# for RT
LINT=	lint
MIG=	mig

#
# Flags for running various programs
#

#
# Kernel include files.
#
KERNELINCS=	-I$(DSTBASE)/include -I$(DSTBASE)/include/sys

#
# Checksum module. scksum.o only exists for the RT.
#
CKSUMLIB=
# CKSUMLIB=	../lib/scksum.o

#
# Define according to the status of the CAMELOT configuration parameter.
#
CAMLIB=
#CAMLIB=		$(DSTBASE)/src/etc/netmsgserver/camelot/camelot.o

# VMTPLIB = ../lib/libvmtp.a
VMTPLIB =


INCLUDES=	-I. -I../src -I../include -I../camelot $(KERNELINCS)
DEPENDS=	-MD
CFLAGS=		-O $(DEPENDS) $(INCLUDES) -bsd
MIGFLAGS=	$(DEPENDS)
LDFLAGS=
LINTFLAGS=	$(INCLUDES) -u $(THLIB)
#THLIB=		-lthreads
LIBS=		$(THLIB) $(VMTPLIB) $(CAMLIB) $(CKSUMLIB)

#
# Suffix for dependency files
#
MD_SUFFIX=d

#
# Directories where to find stuff
#
SRC=	../src
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
MIGDEFS=	$(SRC)/netname.defs $(SRC)/km.defs $(SRC)/logstat.defs
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

install:	default
	-@mach_install $I $(NM) $(INSTALL_DIR)/new_netmsgserver
	-@mach_install $I ../conf/NETMSG_CONFIG $(INSTALL_DIR)/NETMSG_CONFIG
	-@touch $(NM).install
#
# How to link all the programs
#
$(NM):	$(NM).o $(ALLOBJS)
	$(CC) $(LDFLAGS) -o $(NM) $(NM).o $(ALLOBJS) $(LIBS)

nmthreads:	$(NM).o $(ALLOBJS)
	$(CC) $(LDFLAGS) -o nmthreads $(NM).o $(ALLOBJS) -lthreads -lmach ../lib/kds_user.o

ptest: port_test.o $(ALLOBJS)
	$(CC) $(LDFLAGS) -o ptest port_test.o $(ALLOBJS) $(LIBS)

ttest: trans_test.o $(ALLOBJS)
	$(CC) $(LDFLAGS) -o ttest trans_test.o $(ALLOBJS) $(LIBS)

trint_test: trint_test.o $(ALLOBJS)
	$(CC) $(LDFLAGS) -o trint_test trint_test.o $(ALLOBJS) $(LIBS)

km_test: km_test.o km_user.o multperm.o ../lib/kds_server.o
	$(CC) $(LDFLAGS) -o km_test km_test.o km_user.o multperm.o ../lib/kds_server.o -lmach

nmcontrol: nmcontrol.o logstat_user.o ls_rec.o
	$(CC) $(CFLAGS) -o nmcontrol nmcontrol.o logstat_user.o ls_rec.o -lmach

nmxlog: nmxlog.o
	$(CC) $(CFLAGS) -o nmxlog nmxlog.o

nmxportstat: nmxportstat.o ls_rec.o
	$(CC) $(CFLAGS) -o nmxportstat nmxportstat.o

nmtest: nmtest.o
	$(CC) $(CFLAGS) -o nmtest nmtest.o -lthreads -lmach

vmtptest: vmtptest.o
	$(CC) $(CFLAGS) -o vmtptest vmtptest.o -lthreads -lmach ../lib/libvmtp.a

nmtime: nmtime.o
	$(CC) $(CFLAGS) -o nmtime nmtime.o -lmach

#
# Generic rules
#
$(OBJS): $(SRC)/$(@:.o=.c)
	$(CC) $(CFLAGS) -c $(SRC)/$*.c
	md -u $(DEP_FILE) -d $*.d

$(NM).o: $(SRC)/$(NM).c
	$(CC) $(CFLAGS) -c $(SRC)/$(NM).c
	md -u $(DEP_FILE) -d $*.d

$(NMTESTOBJS): $(SRC)/$(@:.o=.c)
	$(CC) $(CFLAGS) -c $(SRC)/$*.c
	md -u $(DEP_FILE) -d $*.d

$(UTILOBJS): $(UTILS)/$(@:.o=.c)
	$(CC) $(CFLAGS) -c $(UTILS)/$*.c
	md -u $(DEP_FILE) -d $*.d

$(MIGOBJS): $(@:.o=.c)
	$(CC) $(CFLAGS) -c $*.c
	md -u $(DEP_FILE) -d $*.d

#
# Rules for mig
#
netname_user.c netname_server.c netname.h: $(SRC)/netname.defs
	$(MIG) $(MIGFLAGS)  $(SRC)/netname.defs
	md -u $(DEP_FILE) -d netname.d
	cp netnameUser.c netname_user.c; rm -f netnameUser.c
	cp netnameServer.c netname_server.c; rm -f netnameServer.c

km_user.c km_server.c km.h: $(SRC)/km.defs
	$(MIG) $(MIGFLAGS) $(SRC)/km.defs
	md -u $(DEP_FILE) -d km.d
	cp kmUser.c km_user.c; rm -f kmUser.c
	cp kmServer.c km_server.c; rm -f kmServer.c

logstat_server.c logstat_user.c logstat.h: $(SRC)/logstat.defs
	$(MIG) $(MIGFLAGS) $(SRC)/logstat.defs
	md -u $(DEP_FILE) -d logstat.d
	cp logstatUser.c logstat_user.c; rm -f logstatUser.c
	cp logstatServer.c logstat_server.c; rm -f logstatServer.c

#
# Special dependencies
#
nmcontrol.o:	logstat.h netname.h
logstat.o:	logstat.h
km_user.o km_server: ../src/km.defs
netname_user.o netname_server.o:	../src/netname.defs
logstat_server.o logstat_user.o:	../src/logstat.defs

#
# Misc targets
#
lint:
	$(LINT) $(LINTFLAGS) $(CFILES)

tags:
	/usr/ucb/ctags ${CFILES}

clean:
	rm -f eddep tags *.o $(MIGFILES) $(NM) $(UTILPROG) $(TESTPROG) \
		errs linterrs makelinks *.log *LOG*


include $(DEP_FILE)
