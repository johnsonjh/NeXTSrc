#  NeXT Internal Library Makefile
#  Copyright 1987, NeXT, Inc.
#

NAME = DL

#directory where make gets its includes.  Do not edit.
MAKE_DIR = /usr/local/lib/Makefiles

#  **2**  -SOURCE FILES-
#  OTHER_SRCS is used for
#  files with other suffixes that you wish to be treated like source
#  (i.e., printed with the source, copied with the source, etc.).
#  For files in OTHER_SRCS, you need to specify the corresponding .o file
#  in OTHER_SRCS_OFILES.
PUBLIC_CLASSES = \
	About.m\
	BezelView.m\
	Contents.m\
	Find.m\
	RtfFile.m\
	LRSwitch.m\
	NroffText.m\
	PSCell.m\
	QueryField.m\
	SplitView.m\
	SummaryMatrix.m\
	TXText.m

PRIVATE_CLASSES = 

OTHER_MFILES = \
	aux.m\
	bitmap.m\
	confirm.m\
	drawLine.m

MFILES = $(PUBLIC_CLASSES) $(PRIVATE_CLASSES) $(OTHER_MFILES)

CFILES = \
	TIFFlib.c\
	ancillary.c\
	query.c\
	findFile.c

PSWFILES = \
	image.psw \
	textps.psw

PSWMFILES = 
PSFILES =

OTHER_SRCS = SGS_ENV
OTHER_SRCS_OFILES = 

#  **3**  -COMMAND OPTIONS-
#  These are passed as arguments to the Objective-C preprocessor, the C
#  compiler, pswrap, the assembler, lex and yacc.  You may delete any
#  lines that you don't use.  All CFLAGS also get passed to Objective-C. 
#  DEBUGCFLAGS is used along with CFLAGS when "debug" is made.
#  Under CFLAGS, for each library that you use that supplies class
#  and message group files, you must list a -I$(OBJDIR)/XXX, where
#  XXX is the base name of the library.
COMMONCFLAGS = -I. -Wimplicit
# cheat and have the regular library have symbols
CFLAGS = -O -g $(COMMONCFLAGS)
DEBUGCFLAGS = -g $(COMMONCFLAGS) 
PROFCFLAGS = -pg -O $(COMMONCFLAGS)
SHLIBCFLAGS = -O -g $(COMMONCFLAGS) -I$(SHLIBIDIR)
PSWFLAGS = -a -r

#  **4**  -INCLUDE FILES-
#  Private include files are used by the library's code, but not needed
#  by the library's users.  Public include files are needed by others who
#  will use the library, and must be installed along with the library.
PRIVATE_INCFILES = $(PRIVATE_CLASSES:.m=.h)\
	textGlobalDefines.h\
	textprivate.h

PUBLIC_INCFILES = $(PUBLIC_CLASSES:.m=.h) \
	ancillary.h\
	aux.h\
	bitmap.h\
	confirm.h\
	define.h\
	filetypes.h\
	findFile.h\
	frames.h\
	query.h\
	super.h

#  **6**  -BY_PRODUCTS GENERATED FROM BUILDS-
#  If your build procedure creates any files other than .o files and the
#  .c and .m files left over from pswrap, you should list them here so
#  they are removed when a make clean is done.
BY_PRODUCTS = $(PSWFILES:.psw=.h)


all: optimized
ofiles:: .protos_done

#this file included for standard functionality
include $(MAKE_DIR)/Makefile.lib
LIBDIR = $(DSTROOT)/usr/local/lib
INCDIR = $(DSTROOT)/usr/local/include/$(NAME)

#  **7**  -ADDITIONAL INSTALLS-
#  If there are other actions that you need to do during an installation,
#  add them as actions under the below target, "additional_installs".
#  These actions will be carried out right after the library, include
#  files, class, and message group files have been installed.
#additional_installs:
#	put extra actions here with a tab in front of them
${LIBDIR}:
	mkdirs -m 755 $@

install: ${LIBDIR} common_install optimized_install 

libinstall: ${LIBDIR} optimized_install 

#  any extra rules or dependencies can be added after this line
include Makefile.depends

.protos_done: $(PSWFILES:.psw=.h) $(PSWMFILES:.pswm=.h)
	touch .protos_done

## RULES ##
.psw.o:
	$(PSWRAP) $(PSWFLAGS) -h $*.h -o $*.c $*.psw
	$(CC) $(CFLAGS) -c $*.c -o $(OFILE_DIR)/$*.o
	-rm $*.c
	
.psw.c:
	$(PSWRAP) $(PSWFLAGS) -h $*.h -o $*.c $*.psw
	
.psw.h:
	$(PSWRAP) $(PSWFLAGS) -h $*.h -o $*.c $*.psw

