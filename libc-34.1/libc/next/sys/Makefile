#
# Generic Makefile for libraries
# (c) NeXT, Inc.  1989
#
LIBNAME= syslib
BUILDS= STD PROF

CDEFS=	
CFLAGS=	$(CDEFS) -I../../../../usr.include
IFLAGS= -r -m 444
LIBS=

HFILES=	
YFILES=
CFILES=	sigvec.c sigcatch.c errno.c gettimeofday.c
SFILES= accept.s access.s acct.s adjtime.s async_daemon.s 	\
	bind.s chdir.s chmod.s chown.s chroot.s close.s 	\
	connect.s creat.s dup.s dup2.s execve.s exportfs.s	\
	fchmod.s fchown.s fcntl.s flock.s fstat.s fstatfs.s	\
	fsync.s ftruncate.s getdents.s getdirentries.s		\
	getdomainname.s getdtablesize.s getfh.s getgid.s	\
	getgroups.s gethostid.s gethostname.s getitimer.s	\
	getpagesize.s getpeername.s getpgrp.s getpid.s 		\
	getpriority.s getrlimit.s getrusage.s getsockname.s	\
	getsockopt.s getuid.s ioctl.s kill.s			\
	killpg.s link.s listen.s lseek.s lstat.s mkdir.s	\
	mknod.s mmap.s mount.s nfssvc.s open.s profil.s 	\
	quota.s quotactl.s read.s readlink.s readv.s reboot.s	\
	recv.s recvfrom.s recvmsg.s rename.s rmdir.s rpause.s	\
	select.s send.s sendmsg.s sendto.s setdomainname.s	\
	setgroups.s sethostid.s sethostname.s setitimer.s 	\
	setpgrp.s setpriority.s setquota.s setregid.s 		\
	setreuid.s setrlimit.s setsockopt.s settimeofday.s	\
	shutdown.s sigblock.s sigpause.s sigsetmask.s 		\
	sigstack.s socket.s socketpair.s stat.s statfs.s	\
	swapon.s symlink.s sync.s table.s truncate.s umask.s 	\
	unlink.s unmount.s utimes.s vhangup.s write.s 		\
	writev.s sigreturn.s _exit.s getegid.s geteuid.s 	\
	getppid.s execl.s execle.s execv.s pipe.s ptrace.s	\
	sigtramp.s syscall.s ustat.s wait.s wait3.s wait4.s 	\
	cerror.s fork.s

DERIVED=
GARBAGE=

SHLIB_NAME=_s
PROF_NAME=_p
STD_NAME=

SHLIB_COPTS= -O -g -i$(SHLIBIDIR)/shlib.h -DSHLIB
PROF_COPTS= -O -pg
STD_COPTS= -O -g

SHLIB_VPATH=..
PROF_VPATH=..
STD_VPATH=..

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

COPTS= ${$(TYPE)_COPTS}
LDOPTS= ${$(TYPE)_LDOPTS}
DIR= $(TYPE)_OBJS
NAME= ${$(TYPE)_NAME}
VPATH= ${$(TYPE)_VPATH}
LIBRARY= $(LIBNAME)${$(TYPE)_NAME}

.c.o:
	$(CC) -MD $(CFLAGS) $(COPTS) -c -o $@ $<

.s.o:
	@rm -f x.s
	$(CC) -MD -E $(CFLAGS) $(COPTS) -DASSEMBLER $< >x.s
	$(AS) x.s -o $@
	@rm -f x.s

all:	$(BUILDS)

SHLIB PROF STD:	$$@_OBJS ALWAYS
	cd $@_OBJS; $(MAKE) -f ../Makefile TYPE="$@" $(LIBNAME)$($@_NAME)

#
# Building the library
#
$(LIBRARY): $(OBJS) depend
	$(AR) cr $@ `lorder $(OBJS) | tsort`

#
# Standard targets
#
depend:
	md -d -u Makedep *.d

clean:	ALWAYS
	-rm -f *.o *.a TAGS tags Makedep $(DERIVED) $(GARBAGE)
	-rm -f SHLIB_OBJS/* PROF_OBJS/* STD_OBJS/*

DSTROOT:
	@if [ -n "$($@)" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

$(DSTDIRS) SHLIB_OBJS PROF_OBJS STD_OBJS:
	mkdirs $@

tags:	ALWAYS
	etags $(SRCS)
	ctags $(SRCS)

ALWAYS:

-include $(DIR)/Makedep
