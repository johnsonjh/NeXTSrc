# Makefile for GNU tar program.
# @(#)Makefile 1.30	87/11/11

# In addition to setting DEFS appropriately for your system, you might
# have to hand edit the #defines and #undefs in port.c and rtape_lib.c.
# For Ultrix 3.1, you will have to compile rtape_lib.c with -DUSG
#

## GNU version
#DEFS = -DBSD42
#LOCAL_SRC = 
#LOCAL_OBJ = 
#LDFLAGS =
#LIBS =  -lutils
#LINT = lint
#LINTFLAGS = -abchx
#DEF_AR_FILE = \"/dev/rmt8\"
#DEFBLOCKING = 20
#O = o

# Berserkeley version
DEFS = -DBSD42
LOCAL_SRC = getdate.y  rtape_lib.c
LOCAL_OBJ = getdate.$O rtape_lib.$O

LDFLAGS =
LIBS =
LINT = lint
LINTFLAGS = -abchx
DEF_AR_FILE = \"/dev/rmt8\"
DEFBLOCKING = 20
O = o

# USG version
# Add -DNDIR to DEFS if your system used ndir.h instead of dirent.h
# Add -DDIRECT to DEFS if your system uses 'struct direct' instead of
# 'struct dirent' (this is the case at least with one add-on ndir 
# library)
# Add -lndir to LIBS if your ndir routines aren't in libc.a
# Add -lPW to LIBS if you don't compile with gcc (to get alloca)
#DEFS = -DUSG #-DNDIR -DDIRECT
#LOCAL_SRC =  getdate.y rtape_lib.c
#LOCAL_OBJ =  getdate.$O rtape_lib.$O
#LDFLAGS =
#LIBS = #-lndir -lPW
#LINT = lint
#LINTFLAGS = -p
#DEF_AR_FILE = \"/dev/rmt8\"
#DEFBLOCKING = 20
#O = o

# UniSoft's Uniplus SVR2 with NFS
#DEFS = -DUSG -DUNIPLUS -DNFS -DSVR2
#LOCAL_SRC =  getdate.y rtape_lib.c
#LOCAL_OBJ =  getdate.$O rtape_lib.$O
#LDFLAGS =
#LIBS = -lndir
#LINT = lint
#LINTFLAGS = -bx
#DEF_AR_FILE = \"/dev/rmt8\"
#DEFBLOCKING = 20
#O = o

# MASSCOMP version
#CC = ucb cc
#DEFS = -DBSD42
#LOCAL_SRC =  getdate.y rtape_lib.c
#LOCAL_OBJ =  getdate.$O rtape_lib.$O
#LDFLAGS =
#LIBS = 
#LINT = lint
#LINTFLAGS = -bx
#DEF_AR_FILE = \"/dev/rmt0\"
#DEFBLOCKING = 20
#O = o

# (yuk) MS-DOS (Microsoft C 4.0) version
#MODEL = S
#DEFS = -DNONAMES -A$(MODEL) -DNO_REMOTE
#LOCAL_SRC =  getdate.y rtape_lib.c
#LOCAL_OBJ =  getdate.$O rtape_lib.$O
#LDFLAGS =
#LIBS = $(MODEL)dir.lib
#LINT =	$(CC)
#LINTFLAGS = -W3
#DEF_AR_FILE = \"tar.out\"
#DEFBLOCKING = 20
#O = obj

# V7 version
# Pick open3 emulation or nonexistence.  See open3.h, port.c.
##DEFS = -DV7 -DEMUL_OPEN3 -Dvoid=int
##DEFS = -DV7 -DNO_OPEN3 -Dvoid=int
#LOCAL_SRC =  getdate.y rtape_lib.c
#LOCAL_OBJ =  getdate.$O rtape_lib.$O
#LDFLAGS =
#LIBS = -lndir
#LINT = lint
#LINTFLAGS = -abchx
#DEF_AR_FILE = \"/dev/rmt8\"
#DEFBLOCKING = 20
#O = o

# Minix version
# No lint, so no lintflags.  Default file is stdin/out.  (Minix "tar"
# doesn't even take an "f" flag, it assumes argv[2] is the archive name!)
# Minix "make" doesn't expand macros right, so Minix users will have
# to expand CFLAGS, SRCS, O, etc by hand, or fix your make.  Not my problem!
# You'll also need to come up with ctime(), the directory
# library, and a fixed doprintf() that handles %*s.  Put this stuff in
# the "SUBSRC/SUBOBJ" macro below if you didn't put it in your C library.
# Note that Minix "cc" produces ".s" files, not .o's, so O = s has been set.
#
# Pick open3 emulation or nonexistence.  See open3.h, port.c.
##DEFS = -DV7 -DMINIX -DEMUL_OPEN3
##DEFS = -DV7 -DMINIX -DNO_OPEN3
#LOCAL_SRC =  getdate.y rtape_lib.c
#LOCAL_OBJ =  getdate.$O rtape_lib.$O
#LDFLAGS =
#LIBS =
#DEF_AR_FILE = \"-\"
#DEFBLOCKING = 8	/* No good reason for this, change at will */
#O = s

# Xenix version
#DEFS = -DUSG -DXENIX
#LOCAL_SRC =  getdate.y rtape_lib.c
#LOCAL_OBJ =  getdate.$O rtape_lib.$O
#LDFLAGS = 
#LIBS = -lx
#LINT = lint
#LINTFLAGS = -p
#DEF_AR_FILE = \"/dev/rmt8\"
#DEFBLOCKING = 20
#O = o

# SGI 4D version
#DEFS = -DUSG -I/usr/include/bsd
#LOCAL_SRC = getdate.y rtape_lib.c
#LOCAL_OBJ =  getdate.$O rtape_lib.$O
#LDFLAGS = 
#LIBS =
#LINT = lint
#LINTFLAGS = -p
#DEF_AR_FILE = \"/dev/tape\"
#DEFBLOCKING = 20
#O = o

#CC = gcc
#TARGET_ARCH = 

CFLAGS = $(COPTS) $(ALLDEFS)
ALLDEFS = $(DEFS) \
	-DDEF_AR_FILE=$(DEF_AR_FILE) \
	-DDEFBLOCKING=$(DEFBLOCKING)
# next line for Debugging
COPTS = -g
# next line for Production
#COPTS = -O

# Add things here like readdir that aren't in your standard libraries.
# (E.g. MSDOS needs msd_dir.c, msd_dir.obj)
SUBSRC=
SUBOBJ=	

# Destination directory and installation program for make install
DESTDIR = /usr/local
INSTALL = cp
RM = rm -f

SRC1 =	tar.c create.c extract.c buffer.c getoldopt.c update.c
SRC2 =  version.c list.c names.c diffarch.c port.c wildmat.c getopt.c getopt1.c
SRC3 =  $(LOCAL_SRC) $(SUBSRC)
SRCS =	$(SRC1) $(SRC2) $(SRC3)
OBJ1 =	tar.$O create.$O extract.$O buffer.$O getoldopt.$O list.$O update.$O
OBJ2 =	version.$O names.$O diffarch.$O port.$O wildmat.$O getopt.$O getopt1.$O
OBJ3 =  $(LOCAL_OBJ) $(SUBOBJ)
OBJS =	$(OBJ1) $(OBJ2) $(OBJ3)
# AUX =	README PORTING Makefile TODO tar.h port.h open3.h \
#	msd_dir.h msd_dir.c
AUX =   README COPYING ChangeLog Makefile tar.texinfo tar.h port.h open3.h \
	rmt.h msd_dir.h msd_dir.c rtape_server.c rtape_lib.c getdate.y getopt.h

all:	tar rmt

tar:	$(OBJS)
	$(CC) $(LDFLAGS) -o tar $(COPTS) $(OBJS) $(LIBS)

rmt:	rtape_server.c
	$(CC) $(CFLAGS) -o rmt rtape_server.c

# command is too long for Messy-Dos (128 char line length limit) so
# this kludge is used...
#	@echo $(OBJ1) + > command
#	@echo $(OBJ2) >> command
#	link @command, $@,,$(LIBS) /NOI;
#	@$(RM) command

install: all
	$(RM) $(DESTDIR)/bin/tar
	$(INSTALL) tar   $(DESTDIR)/bin/tar
	$(INSTALL) tar.texinfo $(DESTDIR)/man/tar.texinfo
	$(INSTALL) rmt /etc/rmt

lint:	$(SRCS)
	$(LINT) $(LINTFLAGS) $(ALLDEFS) $(SRCS)

clean:
	$(RM) errs $(OBJS) tar rmt

tar.shar.Z: $(SRCS) $(AUX)
	shar $(SRCS) $(AUX) | compress > tar.shar.Z

dist: tar.tar tar.tar.Z

tar.tar: $(SRCS) $(AUX)
	-rm -rf tar-dist
	mkdir tar-dist
	-ln $(AUX) $(SRCS) tar-dist
	/bin/tar cf tar.tar tar-dist

tar.tar.Z: tar.tar
	compress < tar.tar > tar.tar.Z
#	/bin/tar cf - $(AUX) $(SRCS) | compress -v >tar.tar.Z

tar.zoo: $(SRCS) $(AUX)
	zoo a tar $(AUX) $(SRCS)

$(OBJS): tar.h port.h
