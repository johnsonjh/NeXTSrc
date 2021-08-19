# $Source: /usr/users/louie/ntp/RCS/Makefile,v $ $Revision: 3.4.1.7 $ $Date: 89/05/18 12:43:32 $
#
# $Log:	Makefile,v $
# Revision 3.4.1.7  89/05/18  12:43:32  louie
# Add preliminary support of NeXT machine, new floating point bug work-around
# and reference clock support.
# 
# Revision 3.4.1.6  89/05/03  15:08:06  louie
# In the Makefile, remove references to the readclock.c module.
# 
# Revision 3.4.1.5  89/04/10  15:54:19  louie
# Add dependency for ntpd.o on ntp.h so things sorta work even without doing a
# "make depend".  Trash "ci" rule.
# 
# Revision 3.4.1.4  89/04/07  18:04:14  louie
# Add definition of NOSWAP for Ultrix systems.
# 
# Revision 3.4.1.3  89/03/29  12:21:27  louie
# Don't bother to create the Version file for the distribution any longer.
# Define SUN_FLT_BUG rather than just 'sun' to get the a fix for a floating
# point problem in ntpsubs; apparantly the DECSTATION 3100 also has the
# same problem.
# 
# Revision 3.4.1.2  89/03/22  18:25:14  louie
# patch3: Use new and improved RCS headers.
# 
# Revision 3.4.1.1  89/03/20  00:01:24  louie
# patch1: Add LIBS macro to Makefile directives which link ntp, ntpdc, ntpd and
# patch1: test programs.
# patch1:  
# patch1: Add symbolic version identifier to Makefile install targets for
# patch1: proper directory name.
# 
# Revision 3.4  89/03/17  18:36:37  louie
# Latest test release.
# 
# Revision 3.3.1.1  89/03/17  18:21:17  louie
# Install new binaries only if they differ.
# 
# Revision 3.3  89/03/15  14:19:08  louie
# New baseline for next release.
# 
# Revision 3.2.1.2  89/03/15  13:42:36  louie
# Updated distribution directory.
# 
# Revision 3.2.1.1  89/03/10  11:23:21  louie
# Repair a bunch of typos in the Makefile
# 
# Revision 3.2  89/03/07  18:19:37  louie
# New version of UNIX NTP daemon and software based on the 6 March 1989 
# draft of the new NTP specification.  
# 
# Revision 3.1.1.1  89/02/15  09:09:47  louie
# Applied fixes from bug reports.  Makefile now runs the ntest program to
# verify that certain arithmetic operations are done correctly on the
# host that the code is being compiled on.
# 
# 
#
DESTDIR=
INCPATH=
LDFLAGS=
BINDIR=/usr/local/etc
LINKDIR=/etc
LIBS=

VERS=3.4

#CC=gcc -g -W -Wall
# Modified by ltucker to cc
CC=cc -g -W

INSTALL= install -c

#
# FEATURES include:
#	DEBUG	  - include DEBUG code
#	BROADCAST_NTP - experimental support for broadcast NTP
#	XADJTIME2 - experimental support for second-order clock adjustment
#		    system call.
#	SETTICKADJ - attempt to modify kernel's `tickadj' variable at run time.
#	REFCLOCK  - define if you have a reference clock attached to your
#		    machine.  (untested by UMD)
#	PSTI - define along with REFCLOCK if you have a PSTI clock attached
#		that you'd like to use a a reference clock.
#	XTAL=0 - for line freq clock, or
#	XTAL=1 	 for crystal controlled clock (default)
#	LOG_NTP=foo - to change the syslog facility.  You could specify
#		    something like -DLOG_NTP=LOG_LOCAL3 to log into the
#		    LOG_LOCAL3 syslog facility
#	NOSWAP - allow use of plock() to prevent swapping
#

#FEATURES= -DBROADCAST_NTP -DSETTICKADJ -DDEBUG
#FEATURES= -DSETTICKADJ -DDEBUG -DREFCLOCK -DPSTI
FEATURES= -DSETTICKADJ -DDEBUG -DREFCLOCK

# for 4.3 BSD
DEFINES=

# for Sun
#DEFINES= -DSUN_FLT_BUG

# for Ultrix 2.0/2.2
# don't forget to fix the broken definition of inet_addr in netdb.h
# it should be declared as a u_long not a in_addr  (the doc is wrong also)
# VAX_COMPILER_FLT_BUG is defined for pcc which doesn't know how to 
# convert an unsigned long into a float/double
#DEFINES= -DVAX_COMPILER_FLT_BUG -DNOSWAP

#
# for a NeXT system, define these pre-processor symbols.
DEFINES=-DSUN_FLT_BUG -DGENERIC_UNS_BUG 

CFLAGS= -O ${DEFINES} ${FEATURES} ${INCPATH}
#
# Header files
#
HDRS=	ntp.h patchlevel.h

# Source files
#
NTPDSRC= ntpd.c ntpsubs.c ntp_proto.c ntp_sock.c ntp_adjust.c read_local.c \
	read_psti.c
SRCS=	ntp.c ntpdc.c ${NTPDSRC}

# Object files
#
NTPDOBJ= ntpd.o ntpsubs.o ntp_proto.o ntp_sock.o ntp_adjust.o read_local.o \
	read_psti.o
OBJS=	ntp.o ntpdc.o ${NTPDOBJ}


DIST= README Makefile man ${SRCS} ${HDRS} ntp.conf test.c extract.pl stat.pl
PROGS=	ntp ntpd ntpdc ntest

all:	${PROGS}
	@./ntest

ntp:	ntp.o ntpsubs.o
	${CC} ${LDFLAGS} -o ntp ntp.o ntpsubs.o ${LIBS}

ntpd:	${NTPDOBJ}
	${CC} ${LDFLAGS} -o ntpd ${NTPDOBJ} ${LIBS}

ntpdc: ntpdc.o
	${CC} ${LDFLAGS} -o ntpdc ntpdc.o ${LIBS}

ntest: test.o ntpsubs.o
	${CC} ${LDFLAGS} -o ntest test.o ntpsubs.o ${LIBS}

sock_test: ntp_sock.c
	${CC} ${LDFLAGS} -DTEST -o sock_test ntp_sock.c ${LIBS}

${OBJS}:	ntp.h Makefile
ntpd.o:	patchlevel.h

install: ${DESTDIR}${BINDIR}/ntpd ${DESTDIR}${BINDIR}/ntp ${DESTDIR}${BINDIR}/ntpdc ntest
	@./ntest
#
# If you don't want a symlink to the daemon, comment out the next line
#	make ${MFLAGS} DESTDIR=${DESTDIR} install-link

${DESTDIR}${BINDIR}/ntpd:	ntpd
	${INSTALL}  ntpd ${DESTDIR}${BINDIR}/ntpd

${DESTDIR}${BINDIR}/ntp:	ntp
	${INSTALL}  ntp ${DESTDIR}${BINDIR}/ntp

${DESTDIR}${BINDIR}/ntpdc:	ntpdc
	${INSTALL}  ntpdc ${DESTDIR}${BINDIR}/ntpdc

install-man:
	cd man; make ${MFLAGS} DESTDIR=${DESTDIR} install

install-link:
	rm -f ${BINDIR}/${LINKDIR}/ntpd
	ln -s ${BINDIR}/ntpd ${DESTDIR}/${LINKDIR}/ntpd

print:
	enscript -2r -p - Makefile ${HDRS} ${SRCS} | qpr -q lps40

clean:
	@rm -f *.o *~ core ${PROGS} ntp.tar ntest sock_test
	cd man; make ${MFLAGS} DESTDIR=${DESTDIR} clean

lint:
	lint -bac ${DEFINES} ${NTPDSRC}

dist:	ntp.tar.Z
	mv ntp.tar.Z /usr/ftp/pub/ntp.${VERS}/ntp.tar.Z

test-dist:	ntp.tar.Z
	mv ntp.tar.Z /usr/ftp/pub/ntp.${VERS}/ntp-test.tar.Z

ntp.tar.Z:	${DIST}
	rm -f ntp.tar ntp.tar.Z
	tar cf ntp.tar ${DIST}
	compress  ntp.tar


depend:
	mkdep $(CFLAGS) $(SRCS)

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
# IF YOU PUT ANYTHING HERE IT WILL GO AWAY
