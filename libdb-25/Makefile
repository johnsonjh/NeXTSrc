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

#directory where make gets its includes.  Do not edit.
MAKE_DIR = /usr/local/lib/Makefiles

#  **1**  -LIBRARY NAME-
#  The base name of the library goes here. This should also be the same
#  name as the SGS project the library is built under.  For example, if
#  you enter "appkit" as the name, then the name of the SGS project
#  should be "appkit", and the library created will be "libappkit.a".

NAME = db
UTILITY_DIR = /utility

USERBOM = lib$(NAME).user_bom
BOMDIR = /usr/local/BOMs
DSTBOM = $(DSTROOT)/$(BOMDIR)

#  The default version of the library is the optimized version.
all:: optimized 

install:: optimized_install common_install install_notes install_bom

installsrc:: installsrc_notes

#  **2**  -SOURCE FILES-
#  Here list the .m, .c, .psw, .pswm, .s, .l, .lm, .y, .ym, .ps and
#  .spec files.  You may delete any lines that you don't use.
#  OTHER_SRCS is used for files with other suffixes that you wish
#  to be treated like source (i.e., printed with the source, copied
#  with the source, etc.).  For files in OTHER_SRCS, you need to
#  specify the corresponding .o file in OTHER_SRCS_OFILES.
CFILES = db.c dbInit.c dbError.c dbCat.c defaults.c \
	dbUnlink.c dbLock.c dbPrint.c dbData.c dbExists.c dbGlobals.c
OTHER_SRCS = $(USERBOM)

#  **3**  -COMMAND OPTIONS-
#  These are passed as arguments to the Objective-C preprocessor, the C
#  compiler, pswrap, the assembler, lex and yacc.  You may delete any
#  lines that you don't use.  All CFLAGS also get passed to Objective-C. 
#  DEBUGCFLAGS is substituted for CFLAGS when "debug" is made.
#  PROFCFLAGS is substituted for CFLAGS when "profile" is made.
#  Under CFLAGS, for each library that you use that supplies class
#  and message group files, you must use a -I$(OBJDIR)/XXX, where
#  XXX is the base name of the library.
CFLAGS = -O -I.
DEBUGCFLAGS = -g -I. -Wall
PROFCFLAGS = -O -pg -g -I.
SHLIBCFLAGS = -O -g -I. -I$(SHLIBIDIR)

#  **4**  -INCLUDE FILES-
#  Private include files are used by the library's code, but not needed
#  by the library's users.  Public include files are needed by others who
#  will use the library, and must be installed along with the library.
PRIVATE_INCFILES = internal.h
PUBLIC_INCFILES = db.h defaults.h

#this file included for standard functionality
include $(MAKE_DIR)/Makefile.lib

#  **7**  -ADDITIONAL INSTALLS-
#  If there are other actions that you need to do during an installation,
#  add them as actions under the below target, "additional_installs".
#  These actions will be carried out right after the library, include
#  files, class, and message group files have been installed.

#  any extra rules or dependencies can be added after this line
include Makefile.depends

installhdrs: inst_incfiles
# terrifying hackery to add defaults to libdb for 2.0
	@/bin/rm -f $(DSTROOT)/usr/include/defaults.h
	@/bin/mv $(INCDIR)/defaults.h $(DSTROOT)/usr/include/defaults.h

install_bom: $(DSTBOM)
	install -m 644 $(USERBOM) $(DSTBOM)

$(DSTBOM):; mkdirs -m 755 $@

# Yet another hack to add defaults to libdb for 2.0
install:: installhdrs

#  Release notes are installed to /usr/local/RelNotes.
NOTEDIR = $(DSTROOT)/usr/local/RelNotes

#  List of release notes for the library and subdirectory.
NOTEFILES = Lib$(NAME)Notes.rtf

#  Install release notes by copying to the target directory.
install_notes: $(NOTEDIR)
	- ( cd Notes; install -m 644 $(NOTEFILES) $(NOTEDIR) )

#  Create the target directory for the release notes.
$(NOTEDIR):; mkdirs $@

#  Notefiles are kept in ./Notes
SRCNOTES = $(SRCROOT)/Notes

#  Install notefiles to the notefile source directory.
installsrc_notes:: $(SRCNOTES)
	- ( cd Notes; cp $(NOTEFILES) $(SRCNOTES) )

#  Create the source directory for the notefiles.
$(SRCNOTES):; mkdirs $@

#  Pass "make all", "make install", "make clean" or "make install_src"
#  to lower level directories.
all install clean installsrc::
	@echo make $@ for `pwd`/utility
	( cd .$(UTILITY_DIR) ; $(MAKE) SRCROOT=$(SRCROOT)$(UTILITY_DIR) \
		DSTROOT=$(DSTROOT) $@ ; )

ALWAYS:;


