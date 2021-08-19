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

DOCDIR = BTree
NAME = btree
UTILITY_DIR = utility
TEST_DIR = test

#  The default version of the library is the debug version.
.DEFAULT: debug

install:: optimized common_install install_notes optimized_install

installsrc:: installsrc_notes

#  **2**  -SOURCE FILES-
#  Here list the .m, .c, .psw, .pswm, .s, .l, .lm, .y, .ym, .ps and
#  .spec files.  You may delete any lines that you don't use.
#  OTHER_SRCS is used for files with other suffixes that you wish
#  to be treated like source (i.e., printed with the source, copied
#  with the source, etc.).  For files in OTHER_SRCS, you need to
#  specify the corresponding .o file in OTHER_SRCS_OFILES.
SFILES =
MFILES = BTree.m BTreeCursor.m BTreeFile.m \
	BTreePrivate.m BTreeStore.m BTreeStoreFile.m BTreeStoreMemory.m \
	BTreeFileOffset.m BTreeGlobals.m BTreeCompare.m BTreeSearch.m
#SPECSHEETS = BTree.wn BTreeCursor.wn BTreeFile.wn
#OTHER_SRCS = $(SPECSHEETS)

#  **3**  -COMMAND OPTIONS-
#  These are passed as arguments to the Objective-C preprocessor, the C
#  compiler, pswrap, the assembler, lex and yacc.  You may delete any
#  lines that you don't use.  All CFLAGS also get passed to Objective-C. 
#  DEBUGCFLAGS is substituted for CFLAGS when "debug" is made.
#  PROFCFLAGS is substituted for CFLAGS when "profile" is made.
#  Under CFLAGS, for each library that you use that supplies class
#  and message group files, you must use a -I$(OBJDIR)/XXX, where
#  XXX is the base name of the library.
OPTFLAGS = -O -fstrength-reduce 
CFLAGS = $(OPTFLAGS) -fomit-frame-pointer -g -I.
DEBUGCFLAGS = -g -I. -Wall -DVERIFY -Dinline=
PROFCFLAGS = $(OPTFLAGS) -pg -I. -Dinline=
SHLIBCFLAGS = $(OPTFLAGS) -fomit-frame-pointer -g -I. -I$(SHLIBIDIR)

#  **4**  -INCLUDE FILES-
#  Private include files are used by the library's code, but not needed
#  by the library's users.  Public include files are needed by others who
#  will use the library, and must be installed along with the library.
PRIVATE_INCFILES = BTreeFilePrivate.h BTreePrivate.h BTreeStore.h \
	BTreeStoreFile.h BTreeStoreMemory.h
PUBLIC_INCFILES = BTree.h BTreeCursor.h BTreeErrors.h BTreeFile.h

#this file included for standard functionality

include $(MAKE_DIR)/Makefile.lib
include Makefile.depends

#
#  any extra rules or dependencies can be added after this line
#

.m.s:; $(CC) $(CFLAGS) $(OBJCFLAGS) -S $*.m

sfiles: $(SFILES)

#install::; $(MAKE) "SPECFILES = $(SPECSHEETS)" inst_specs

#  Pass "make all", "make install", "make clean" or "make install_src"
#  to lower level directories.

clean all install installsrc:: 
	@if (test -d "./$(UTILITY_DIR)") \
		then echo make $@ for `pwd`/$(UTILITY_DIR); \
		( cd ./$(UTILITY_DIR) ; $(MAKE) SRCROOT=$(SRCROOT)/$(UTILITY_DIR) \
			DSTROOT=$(DSTROOT) $@ ; ); \
	fi

clean debug optimized profile::
	@if (test -d "./$(TEST_DIR)") \
		then echo make $@ for `pwd`/$(TEST_DIR); \
		( cd ./$(TEST_DIR) ; $(MAKE) SRCROOT=$(SRCROOT)/$(UTILITY_DIR) \
			DSTROOT=$(DSTROOT) $@ ; ); \
	fi

install::
	/bin/mv $(DSTROOT)/usr/lib $(DSTROOT)/usr/local ; \
	/bin/mv $(DSTROOT)/usr/include $(DSTROOT)/usr/local

#
# manual pages and release notes
#

LOCAL = $(DSTROOT)/usr/local
LOCALNOTES = $(LOCAL)/RelNotes/BTreeNotes
LOCALMAN = $(LOCAL)/ManPages/BTree

MANPAGES = ManPages
MANPAGESSRC = $(SRCROOT)/$(MANPAGES)

RELNOTES = RelNotes
RELNOTESSRC = $(SRCROOT)/$(RELNOTES)

$(LOCALMAN) $(LOCALNOTES) $(MANPAGESSRC) $(RELNOTESSRC):; mkdirs $@

install_man: $(LOCALMAN)
	@if (test -d "$(MANPAGES)") \
		then ( cd $(MANPAGES); tar cf - . ) | \
			( cd $(LOCALMAN); tar xvfp - ); \
	fi

installsrc_man: $(MANPAGESSRC)
	@if (test -d "$(MANPAGES)") \
		then ( cd $(MANPAGES); tar cf - . ) | \
			( cd $(MANPAGESSRC); tar xvfp - ); \
	fi

install_notes: $(LOCALNOTES)
	@if (test -d "$(RELNOTES)") \
		then ( cd $(RELNOTES); tar cf - . ) | \
			( cd $(LOCALNOTES); tar xvfp - ); \
	fi

installsrc_notes: $(RELNOTESSRC)
	@if (test -d "$(RELNOTES)") \
		then ( cd $(RELNOTES); tar cf - . ) | \
			( cd $(RELNOTESSRC); tar xvfp - ); \
	fi

