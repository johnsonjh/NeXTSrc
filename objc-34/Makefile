#
#  NeXT Internal Library Makefile
#  Copyright 1987, NeXT, Inc.
#
#  This Makefile is used for maintaining and installing NeXT software
#  libraries.
#
#  Fill in values for the various variables at the places marked by
#  **number**.  Any rules or dependencies of your own can be added
#  at the end of the file.
#

CC=cc

#directory where make gets its includes.  Do not edit.
MAKE_DIR = /usr/local/lib/Makefiles

#  **1**  -LIBRARY NAME-
#  The base name of the library goes here. This should also be the same
#  name as the SGS project the library is built under.  For example, if
#  you enter "appkit" as the name, then the name of the SGS project
#  should be "appkit", and the library created will be "libappkit.a".
NAME = objc

#  **2**  -SOURCE FILES-
#  Here list the .m, .c, .psw, .pswm, .s, .l, .lm, .y, .ym, .ps and
#  .spec files.  You may delete any lines that you don't use.
#  OTHER_SRCS is used for files with other suffixes that you wish
#  to be treated like source (i.e., printed with the source, copied
#  with the source, etc.).  For files in OTHER_SRCS, you need to
#  specify the corresponding .o file in OTHER_SRCS_OFILES.

MRTIME = Object.m 
MNEWLIB = HashTable.m List.m Storage.m StreamTable.m NXStringTable.m

MFUNC = objc-runtime.m objc-class.m hashtable.m typedstream.m \
	objc-globaltext.m objc-globaldata.m objc-utils.m objc-sel.m objc-load.m

MFILES = $(MRTIME) $(MNEWLIB) $(MFUNC)
CFILES = except.c 
SFILES = objc-msg.s 
OTHER_SRCS = objcedit.c objcopt.c ObjC.rtf
OTHER_SRCS_OFILES =

RELNOTES = ObjC.rtf

#  **3**  -COMMAND OPTIONS-
#  These are passed as arguments to the Objective-C preprocessor, the C
#  compiler, pswrap, the assembler, lex, yacc and speculate.  You may
#  delete lines that you don't use.  All CFLAGS also get passed to
#  Objective-C.
#  DEBUGCFLAGS is substituted for CFLAGS when "debug" is made.
#  PROFCFLAGS is substituted for CFLAGS when "profile" is made.
#  Under OBJCFLAGS, for each library that you use that supplies class
#  and message group files, you must use a -I$(OBJDIR)/XXX, where
#  XXX is the base name of the library.
COMMONFLAGS = -Wimplicit 
OBJCFLAGS = 
CFLAGS = $(COMMONFLAGS) 
SHLIBCFLAGS = $(COMMONFLAGS) -g -O -I$(SHLIBIDIR)
DEBUGCFLAGS = $(COMMONFLAGS) -g
PROFCFLAGS = $(COMMONFLAGS) -pg -O
PSWFLAGS = 
ASFLAGS = 
LFLAGS = 
YFLAGS = 
SPECFLAGS = 

LOCBINDIR = /usr/local/bin

#  **4**  -INCLUDE FILES-
#  Private include files are used by the library's code, but not needed
#  by the library's users.  Public include files are needed by others who
#  will use the library, and must be installed along with the library.

PRIVATE_INCFILES = typedstreamprivate.h objc-private.h objc-dispatch.h
PUBLIC_INCFILES = $(MRTIME:.m=.h) $(MNEWLIB:.m=.h) \
	objc-runtime.h objc-class.h objc.h objc-load.h \
	vectors.h hashtable.h typedstream.h error.h

#  **5**  -CLASS AND MESSAGE GROUP FILES-
#  If you are generating (not just using) any Objective-C classes
#  list their class files here so their C_ files will be installed for
#  others.  Also list the message group file, which should be of the form
#  P_SomeKit (if you have a message group file from generating classes).
CLFILES = 
MGROUP_FILES =

#  **6**  -BY_PRODUCTS GENERATED FROM BUILDS-
#  If your build procedure creates any files other than .o files and the
#  .c and .m files left over from pswrap, you should list them here so
#  they are removed when a make clean is done.
BY_PRODUCTS = 

#this file included for standard functionality
include $(MAKE_DIR)/Makefile.lib

#  **7**  -ADDITIONAL INSTALLS-
#  If there are other actions that you need to do during an installation,
#  add them as actions under the below target, "additional_installs".
#  These actions will be carried out right after the library, include
#  files, class, and message group files have been installed.

all: profile_ofiles

# choose profile optimize and/or debug
install: profile_ofiles common_install relnotes_install objcopt_install

installhdrs: inst_incfiles

#	put extra actions here with a tab in front of them

# You may comment in this line to get dependencies for the include files
# you use.  To generate the dependencies, do a "make depend".
include Makefile.depends

#  any extra rules or dependencies can be added after this line

relnotes_install:
	if [ -f $(RELNOTES) ]; \
	then \
	    mkdirs $(DSTROOT)/usr/local/RelNotes ; \
	    install -m 444 $(RELNOTES) $(DSTROOT)/usr/local/RelNotes ; \
	fi

objcopt:
	$(CC) -c -DFREEZE objc-sel.m 
	$(CC) -c objcedit.c objcopt.c
	$(CC) -o objcopt objc-sel.o objcedit.o objcopt.o
	-/bin/rm -f objc-sel.o objcedit.o objcopt.o	

${DSTROOT}${LOCBINDIR}:
	mkdirs $@

objcopt_install: objcopt ${DSTROOT}${LOCBINDIR}
	install -m 555 objcopt ${DSTROOT}${LOCBINDIR}/objcopt
