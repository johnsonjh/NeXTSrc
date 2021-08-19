#
# Makefile for the /private/etc/yp directory
#

# Comment out this next line if you want to see all of the make output
.SILENT:

#	The directory we are building
DIR = /private/etc/yp

#	The directory that must be created
DSTDIR = ${DSTROOT}${DIR}
DSTOWNER = root
DSTGROUP = wheel
DSTMODE = 775

YP_RODIR = ../../../usr/etc/yp
YPFILES= makedbm revnetgroup stdhosts yppoll yppush ypset \
	ypxfr
all:

#	Clean out editor lint
clean:
	echo "    Removing" *~ .[a-zA-Z]*~ \#* *.BAK
	rm -f *~ .[a-zA-Z]*~ \#* *.BAK

install: ${DSTDIR}
	cd $(DSTDIR); \
	for i in $(YPFILES); do \
		if [ ! -f $$i ]; then \
			ln -s $(YP_RODIR)/$$i $$i; \
		fi; \
	done

${DSTDIR}: DSTROOT ${DSTROOT}
	echo "    Making ${DSTDIR}"
	mkdirs -o ${DSTOWNER} -g ${DSTGROUP} -m ${DSTMODE} ${DSTDIR}

$(DSTROOT):
	@echo 'Please make a directory for $@ and try again'; exit 1

DSTROOT:
	@if [ -n "${$@}" ]; then exit 0; else echo Must define $@; exit 1; fi
