#  NeXT Internal Library Makefile
#  Copyright 1987, NeXT, Inc.
#  Responsibility: Trey Matteson
#

NAME = appkit

#directory where make gets its includes.  Do not edit.
MAKE_DIR = /usr/local/lib/Makefiles

#  **2**  -SOURCE FILES-
#  OTHER_SRCS is used for
#  files with other suffixes that you wish to be treated like source
#  (i.e., printed with the source, copied with the source, etc.).
#  For files in OTHER_SRCS, you need to specify the corresponding .o file
#  in OTHER_SRCS_OFILES.

PUBLIC_CLASSES = \
	ActionCell.m \
	Application.m \
	Bitmap.m \
	Box.m \
	Button.m \
	ButtonCell.m \
	Cell.m \
	ChoosePrinter.m \
	ClipView.m \
	Control.m \
	Cursor.m \
	Font.m \
	FontPanel.m \
	FontManager.m \
	Form.m \
	FormCell.m \
	Listener.m \
	Matrix.m \
	Menu.m \
	MenuCell.m \
	NXBrowser.m \
	NXBrowserCell.m \
	NXColorPanel.m \
	NXColorWell.m \
	NXImage.m \
	NXImageRep.m \
	NXJournaler.m \
	NXSplitView.m \
	OpenPanel.m \
	PageLayout.m \
	Panel.m \
	Pasteboard.m \
	PopUpList.m \
	PrintInfo.m \
	PrintPanel.m \
	Responder.m \
	SavePanel.m \
	ScrollView.m \
	Scroller.m \
	SelectionCell.m \
	Slider.m \
	SliderCell.m \
	Speaker.m \
	Text.m \
	TextField.m \
	TextFieldCell.m \
	View.m \
	Window.m

# These are classes whose header files we put in /usr/local for IB.
# ColumnCell is used by PrintManager.
LOCAL_CLASSES = \
	ColumnCell.m \
	CustomObject.m \
	CustomView.m \
	FrameView.m \
	MenuTemplate.m \
	NXXYShow.m \
	NameTable.m \
	NibData.m \
	WindowTemplate.m

PRIVATE_CLASSES = \
	Alert.m \
	TaggedCell.m \
	FocusState.m \
	FontFile.m \
	NXJournalListener.m \
	NXJournalSpeaker.m \
	NXColorCell.m \
	NXColorCustom.m \
	NXColorPalette.m \
	NXColorPicker.m \
	NXColorSlider.m \
	NXColorSwatch.m \
	NXColorWheel.m \
	NXLazyBrowserCell.m \
	NXRulerView.m \
	PaperView.m \
	PSMatrix.m \
	QueryCell.m \
	QueryText.m \
	NXFaxCoverView.m \
	NXFaxPanel.m \
	NXFaxPaperBox.m \
	NXFaxText.m

OTHER_MFILES = \
	appkit_globals.m \
	appCache.m \
	appServicesMenu.m \
	appWindows.m \
	color.m \
	epsUtil.m \
	errors.m \
	graphicArchiving.m \
	imageClient.m \
	objectDelayedPerform.m \
	printSupport.m \
	rect.m \
	systemUtils.m \
	textEdit.m \
	textEvent.m \
	textLayout.m \
	textPublic.m \
	textRTF.m \
	textReplace.m \
	textReplaceUtil.m \
	textScanDraw.m \
	textSelect.m \
	textSelectUtil.m \
	textSpell.m \
	textStream.m \
	textTabs.m \
	textUtil.m \
	tiff.m \
	tiffCompress.m \
	tiffDir.m \
	tiffIO.m \
	tiffJpeg.m \
	timer.m \
	viewDrag.m \
	wordTables.m

MFILES = $(PUBLIC_CLASSES) $(LOCAL_CLASSES) $(PRIVATE_CLASSES) $(OTHER_MFILES)

CFILES = 

SFILES = 

PSWFILES = \
	publicWraps.psw \
	textWraps.psw

PSWMFILES = \
	graphicOps.pswm \
	privateWraps.pswm \
	image.pswm  \
	imageWraps.pswm  \
	packagesWraps.pswm \
	wsMemoryWraps.pswm

PSFILES = \
	printPackage.ps \
	nlpPrintPackage.ps

RELNOTES = AppkitNotes.wn

SPELLD = spelld

# These are the nib files for the public panels
NIBFILES = \
	AlertPanel.nib \
	FontPanel.nib \
	PageLayout.nib \
	PrintPanel.nib \
	SavePanel.nib \
	ChoosePrinter.nib \
	ColorCustomAlert.nib \
	ColorPicker.nib \
	ColorPickerAlert.nib \
	NXFaxPanel.nib \
	SpellChecker.nib

TIFFFILES = \
	Spectrum.tiff

OTHER_SRCS = Cover.rtf gdbinit msgwrap.c $(NIBFILES) \
		logErrorInc.c perfTimerInc.m \
		app.defs pbs.defs pbs.c ss.defs $(TIFFFILES) \
		afmFinder.c afmParser.c afmStrings.c afmTables.c \
		afmUtils.c fontinfo.c request.c cacheAFMData.c \
		Dictionary.m Dictionary.h SpellServer.m SpellServer.h \
		ssMain.m dict.h bindict linkUnreferencedClasses \
		imagemessage.h CharOrdering make_services.m \
		$(LOCAL_INCFILES) appkit.user_bom \
		derecord22m.asm dspstream.asm ioequ.asm

OTHER_SRCS_OFILES = pbsUser.o appServer.o ssUser.o

#need a rule for specs that is recursive on make so that we don't have 
#to do the dependiencies.
#SPECFILES = $(PUBLIC_CLASSES:.m=.spec)

#  **3**  -COMMAND OPTIONS-
#  These are passed as arguments to the Objective-C preprocessor, the C
#  compiler, pswrap, the assembler, lex and yacc.  You may delete any
#  lines that you don't use.  MINOR_VERS is passed in from the libNeXT project. 
#  DEBUGCFLAGS is used along with CFLAGS when "debug" is made.
#
COMMONCFLAGS = -I. -O -Wall $(EXTERNAL_CFLAGS) \
			-DMINOR_VERS_STRING='\"$(MINOR_VERS)\"'
CFLAGS = $(COMMONCFLAGS) 
DEBUGCFLAGS = $(COMMONCFLAGS) -g -DDEBUG
PROFCFLAGS = -pg $(COMMONCFLAGS)
SHLIBCFLAGS = $(COMMONCFLAGS) -g -I$(SHLIBIDIR)
OBJCFLAGS = 
LDFLAGS = 
PSWFLAGS = -a -r -S shlib.h
ASFLAGS = 
SPECFLAGS = -v 0.8 -R

#  **4**  -INCLUDE FILES-
#  Private include files are used by the library's code, but not needed
#  by the library's users.  Public include files are needed by others who
#  will use the library, and must be installed along with the library.
PRIVATE_INCFILES = $(PRIVATE_CLASSES:.m=.h) \
	afmprivate.h \
	appkitPrivate.h \
	bitmapPrivate.h	\
	colorPickerPrivate.h \
	cursorprivate.h \
	cursorRect.h \
	listenerprivate.h \
	npd.h \
	perfTimer.h \
	printSupport.h \
	rtfdata.h \
	rtfdefs.h \
	rtfstructs.h \
	spellserver.h \
	textprivate.h \
	Application_Private.h \
	Box_Private.h \
	ButtonCell_Private.h \
	Cell_Private.h \
	ClipView_Private.h \
	Control_Private.h \
	FontManager_Private.h \
	Font_Private.h \
	Listener_Private.h \
	Matrix_Private.h \
	Menu_Private.h \
	NXCursor_Private.h \
	NXImage_Private.h \
	NXImageRep_Private.h \
	NXJournaler_Private.h \
	Panel_Private.h \
	Pasteboard_Private.h \
	PrintInfo_Private.h \
	PrintPanel_Private.h \
	SavePanel_Private.h \
	SelectionCell_Private.h \
	View_Private.h \
	Window_Private.h

# These files are put in /usr/local
LOCAL_INCFILES = $(LOCAL_CLASSES:.m=.h) \
	nibprivate.h \
	pbtypes.h \
	tiffPrivate.h

PUBLIC_INCFILES = $(PUBLIC_CLASSES:.m=.h) \
	NXBitmapImageRep.h \
	NXEPSImageRep.h \
	NXCachedImageRep.h \
	NXCustomImageRep.h \
	NXCursor.h \
	appkit.h \
	afm.h \
	chunk.h \
	color.h \
	errors.h \
	graphics.h \
	nextstd.h \
	obsoleteBitmap.h \
	screens.h \
	timer.h \
	tiff.h

#  **6**  -BY_PRODUCTS GENERATED FROM BUILDS-
#  If your build procedure creates any files other than .o files and the
#  .c and .m files left over from pswrap, you should list them here so
#  they are removed when a make clean is done.
BY_PRODUCTS = msgwrap msgwrap.o \
	$(PBS_OFILES) pbs pbs.h pbsServer.c pbsUser.c \
	app.h appServer.c appUser.c make_services \
	cacheAFMData cacheAFMData.o afmParser2.o \
	$(SPELLD) ss.h ssServer.c ssUser.c $(SS_OFILES) \
	derecord22m.lod make_services.o \
	.protos_done $(PSWFILES:.psw=.h) $(PSWMFILES:.pswm=.h)

ofiles:: .protos_done

#this file included for standard functionality
include $(MAKE_DIR)/Makefile.lib

#  **7**  -ADDITIONAL INSTALLS-
#  If there are other actions that you need to do during an installation,
#  add them as actions under the below target, "additional_installs".
#  These actions will be carried out right after the library, include
#  files, class, and message group files have been installed.
#additional_installs:
#	put extra actions here with a tab in front of them
#	cp $(SPECFILES)  $(SRCDIR)/lib/$(NAME);

all: window_packages msgwrap pbs $(SPELLD) cacheAFMData make_services

install: publicWraps.h pbs.h common_install local_inc_install \
		derecord22m.lod msgwrap_install pbs_install ss_install \
		window_packages_install cover_install relnotes_install \
		bom_install cacheAFMData_install make_services_install
	install -m 644 gdbinit  $(LIBDIR)/.gdbinit
	mkdirs -m 755 $(PSLIBDIR)
	install $(IFLAGS) -m 444 $(NIBFILES) $(PSLIBDIR)
	install $(IFLAGS) -m 444 $(TIFFFILES) $(PSLIBDIR)
	install $(IFLAGS) -m 444 derecord22m.lod $(PSLIBDIR)
	install -m 444 publicWraps.h $(INCDIR)
	install -m 444 CharOrdering $(PSLIBDIR)/CharOrdering
	ln -s ../defaults.h $(INCDIR)/defaults.h

#  any extra rules or dependencies can be added after this line
#include Makefile.depends

View.m printSupport.m : printSupport.h

Pasteboard.m Font.m Application.m SavePanel.m: pbs.h

errors.m pbs.c: logErrorInc.c
timer.m pbs.c: perfTimerInc.m

# This block of rules builds the pasteboard server (pbs) and the corresponding
# module in the kit.  There are two mig interface files, pbs.defs and app.defs.
# The first is for messages from the app to the server, the second is for
# messages from server to app.  Each one generates $$Server, $$User and $$.h
# files.  In order to shlib-ify correctly, "shlib.h" must be included
# at the top of any files contributing to the appkit library.  The special
# macro SHLIB_MIG is defined in these files use the macros in "shlib.h"
# without arguments.  This is needed because mig does not properly prototype
# the functions it extern's.

pbsServer.c pbsUser.c pbs.h: pbs.defs
	mig pbs.defs
	echo \#ifdef SHLIB > pbsUser.tmp
	echo \#define SHLIB_MIG >> pbsUser.tmp
	echo \#import \"shlib.h\" >> pbsUser.tmp
	echo \#endif SHLIB >> pbsUser.tmp
	cat pbsUser.c >> pbsUser.tmp
	mv pbsUser.tmp pbsUser.c

appServer.c appUser.c app.h: app.defs
	mig app.defs
	echo \#ifdef SHLIB > appServer.tmp
	echo \#define SHLIB_MIG >> appServer.tmp
	echo \#import \"shlib.h\" >> appServer.tmp
	echo \#endif SHLIB >> appServer.tmp
	cat appServer.c >> appServer.tmp
	mv appServer.tmp appServer.c

ssServer.c ssUser.c ss.h: ss.defs
	mig ss.defs
	echo \#ifdef SHLIB > ssUser.tmp
	echo \#define SHLIB_MIG >> ssUser.tmp
	echo \#import \"shlib.h\" >> ssUser.tmp
	echo \#endif SHLIB >> ssUser.tmp
	cat ssUser.c >> ssUser.tmp
	mv ssUser.tmp ssUser.c

Pasteboard.o: pbs.h app.h

textSpell.o: ss.h

SS_OFILES = ssServer.o Dictionary.o SpellServer.o ssMain.o

PBS_OFILES = pbsServer.o appUser.o pbs.o \
	     afmFinder.o afmStrings.o \
	     fontinfo.o afmParser.o request.o afmUtils.o

debug_pbs:
	$(MAKE) pbs CFLAGS="-DDEBUG $(CFLAGS)"

pbs.o: pbs.c pbs.h app.h
	VERS=`pwd | sed -e 's/.*-//'`;		\
	$(CC) -g -DPBS_VERS=\"$$VERS\" $(CFLAGS) -c pbs.c

afmFinder.o: afmFinder.c pbs.h
	$(CC) -g $(CFLAGS) -c afmFinder.c

afmParser.o: afmParser.c afmTables.c
	$(CC) -g $(CFLAGS) -c afmParser.c

afmUtils.o: afmUtils.c
	$(CC) -g $(CFLAGS) -c afmUtils.c

afmStrings.o: afmStrings.c
	$(CC) -g $(CFLAGS) -c afmStrings.c

fontinfo.o: fontinfo.c pbs.h
	$(CC) -g $(CFLAGS) -c fontinfo.c

request.o: request.c pbs.h
	$(CC) -g $(CFLAGS) -c request.c

pbsServer.o: pbsServer.c
	$(CC) -g $(CFLAGS) -c pbsServer.c

appUser.o: appUser.c
	$(CC) -g $(CFLAGS) -c appUser.c

pbs: $(PBS_OFILES)
	cc -g -o pbs $(PBS_OFILES) -lsys_s

pbs_install: pbs
	mkdirs $(DSTROOT)/usr/etc
	install -m 555 -s pbs $(DSTROOT)/usr/etc/pbs

Dictionary.o:
	$(CC) -g $(CFLAGS) -c Dictionary.m

SpellServer.o:
	$(CC) -g $(CFLAGS) -c SpellServer.m

ssMain.o: ssMain.m ss.h spellserver.h
	$(CC) -g $(CFLAGS) -c ssMain.m

ssServer.o: ssServer.c
	$(CC) -g $(CFLAGS) -c ssServer.c

$(SPELLD): $(SS_OFILES)
	cc -O -g -o $(SPELLD) $(SS_OFILES) -lsys_s

ss_install: $(SPELLD)
	mkdirs $(DSTROOT)/usr/etc
	install -m 4555 -s $(SPELLD) $(DSTROOT)/usr/etc/$(SPELLD)
	mkdirs $(DSTROOT)/usr/lib/NextStep
	install -m 444 bindict $(DSTROOT)/usr/lib/NextStep

MEDIR = $(DSTROOT)/me
TEMPLATEDIR = $(DSTROOT)/usr/template/user
ROOTSDIR = $(DSTROOT)

derecord22m.lod: dspstream.asm derecord22m.asm ioequ.asm
	asm56000 -a -b -L/dev/null derecord22m.asm

cover_install:
	mkdirs -m 755 $(ROOTSDIR)/NextLibrary/Fax
	install -m 444 Cover.rtf  $(ROOTSDIR)/NextLibrary/Fax/Cover.rtf

# make and install msgwrap utility

msgwrap: msgwrap.c
	cc -O -g -Wall -o msgwrap msgwrap.c -lsys_s

MSGWRAP_DIR = $(DSTROOT)/usr/bin

msgwrap_install: msgwrap
	mkdirs -m 755 $(MSGWRAP_DIR)
	install -m 555 -s msgwrap $(MSGWRAP_DIR)



.protos_done: $(PSWFILES:.psw=.h) $(PSWMFILES:.pswm=.h)
	rm -f .protos_done
	touch .protos_done

## RULES ##
.psw.o:
	$(PSWRAP) $(PSWFLAGS) -h $*.h -o $*.c $*.psw
	$(CC) $(CFLAGS) -c $*.c -o $(OFILE_DIR)/$*.o
	
.psw.c:
	$(PSWRAP) $(PSWFLAGS) -h $*.h -o $*.c $*.psw
	
.psw.h:
	$(PSWRAP) $(PSWFLAGS) -h $*.h -o $*.c $*.psw

.pswm.o:
	$(PSWRAP) $(PSWFLAGS) -h $*.h -o $*.m $*.pswm
	$(CC) $(CFLAGS) $(OBJCFLAGS) -c $*.m -o $(OFILE_DIR)/$*.o

.pswm.c:
	$(PSWRAP) $(PSWFLAGS) -h $*.h -o $*.m $*.pswm
	
.pswm.h:
	$(PSWRAP) $(PSWFLAGS) -h $*.h -o $*.m $*.pswm

# force this variable to be defined
shlib_ofiles:: SHLIBIDIR

SHLIBIDIR:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

# recursive makes for packages
clean installsrc::
	cd packages; $(MAKE) $@

window_packages:
	cd packages; $(MAKE) all

window_packages_install:
	cd packages; $(MAKE) install


bom_install:
	if [ -f appkit.user_bom ]; \
	then \
	    mkdirs ${DSTROOT}/usr/local/BOMs ; \
	    install -m 444 appkit.user_bom ${DSTROOT}/usr/local/BOMs ; \
	fi

relnotes_install:
	if [ -f $(RELNOTES) ]; \
	then \
	    mkdirs $(DSTROOT)/usr/local/RelNotes ; \
	    install -m 444 $(RELNOTES) $(DSTROOT)/usr/local/RelNotes ; \
	fi

local_inc_install: pbs.h
	mkdirs $(DSTROOT)/usr/local/include/appkit
	install $(IFLAGS) -m 444 pbs.h $(LOCAL_INCFILES) $(DSTROOT)/usr/local/include/appkit


afmParser2.c:
	ln -s afmParser.c afmParser2.c

afmParser2.o: afmParser2.c
	$(CC) -g -DCACHE_DATA_BUILD $(CFLAGS) -c afmParser2.c

cacheAFMData_OFILES = afmStrings.o afmParser2.o afmUtils.o cacheAFMData.o

cacheAFMData.o: cacheAFMData.c
	$(CC) -g $(CFLAGS) -c cacheAFMData.c

cacheAFMData_p:
	$(MAKE) cacheAFMData "CFLAGS = $(CFLAGS) -pg" "LDFLAGS = $(LDFLAGS) -pg"

cacheAFMData: $(cacheAFMData_OFILES)
	cc -g -o $(LDFLAGS) cacheAFMData $(cacheAFMData_OFILES) -lsys_s

cacheAFMData_install: cacheAFMData
	mkdirs $(DSTROOT)/usr/bin
	install -m 755 -s cacheAFMData $(DSTROOT)/usr/bin/cacheAFMData

make_services.o: make_services.m
	$(CC) -g $(CFLAGS) -c make_services.m

make_services: make_services.o afmStrings.o
	cc -g $(LDFLAGS) -o make_services make_services.o afmStrings.o -lsys_s

make_services_install: make_services
	mkdirs $(DSTROOT)/usr/bin
	install -m 755 -s make_services $(DSTROOT)/usr/bin/make_services
