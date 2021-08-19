#  Packages Makefile
#  Copyright 1990, NeXT, Inc.
#  Responsibility: Trey Matteson
#


PSFILES = defs.ps activation.ps beep.ps contextData.ps covers.ps \
		debugging.ps events.ps resizing.ps sharedWins.ps \
		cursor.ps winList.ps
SRC = $(PSFILES) Makefile
PACKAGES = testPackages
SRCDIR = $(SRCROOT)/packages


all : $(PACKAGES)

install: all
	mkdirs $(DSTROOT)/usr/lib/NextStep
	install -q -c -m 444 $(PACKAGES) \
			$(DSTROOT)/usr/lib/NextStep/windowPackage.ps

clean :
	rm -f $(PACKAGES)

installsrc:
	mkdirs $(SRCDIR)
	chmod 755 $(SRCDIR)
	tar cf - $(SRC) | (cd $(SRCDIR); tar xf -)

$(PACKAGES): $(PSFILES)
	rm -f $(PACKAGES)
	cat $(PSFILES) > $(PACKAGES)

debug: $(PACKAGES)
	sed -e '/\/_NXDebug/s/false/true/' $(PACKAGES) > newpacks
	mv newpacks $(PACKAGES)
