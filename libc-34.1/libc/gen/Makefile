#
# Generic Makefile for libraries
# (c) NeXT, Inc.  1989
#
LIBNAME= genlib
BUILDS= STD PROF

CDEFS=	
CFLAGS=	$(CDEFS) -I../../../usr.include
IFLAGS= -r -m 444
LIBS=

HFILES=	PortListen.h mallocProtocol.h
YFILES=
CFILES=	abort.c alarm.c aliasdb.c atoi.c atol.c cfree.c 	\
	closedir.c crypt.c ctime.c ctime_data.c ctype_.c 	\
	debug_malloc.c errlst.c errstrings.c execvp.c 		\
	exportent.c fgetpwent.c frexp.c fstab.c getenv.c 	\
	getgrent.c getlogin.c getpass.c getpwent.c getttyent.c 	\
	getttynam.c getusershell.c getwd.c initgroups.c 	\
	isatty.c malloc.c malloc_server.c mkstemp.c mktemp.c 	\
	mntent.c ndbm.c nlist.c opendir.c perror.c popen.c 	\
	port_listen.c psignal.c qsort.c random.c readdir.c	\
	realpath.c regex.c scandir.c seekdir.c setgid.c 	\
	setegid.c setrgid.c setuid.c seteuid.c setruid.c	\
	siginterrupt.c siglist.c signal.c sigstrings.c sleep.c	\
	strcasecmp.c swab.c syslog.c system.c telldir.c time.c	\
	timezone.c ttyname.c ttyslot.c ualarm.c usleep.c
SFILES=

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

all:	$(BUILDS)

SHLIB PROF STD:	$$@_OBJS ALWAYS
	cd $@_OBJS; $(MAKE) -f ../Makefile TYPE="$@" $(LIBNAME)$($@_NAME)

#
# Building the library
#
genlib: $(OBJS) depend
	-rm -f debug_malloc.o
	$(AR) cr $@ `lorder $(OBJS) | tsort`

genlib_p: $(OBJS) depend
	-rm -f debug_malloc.o
	$(AR) cr $@ `lorder $(OBJS) | tsort`

genlib_s: $(OBJS) depend
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
