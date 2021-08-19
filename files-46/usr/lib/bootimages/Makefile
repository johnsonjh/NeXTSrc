#
# Makefile for the usr/etc/images directory
#

# Comment out this next line if you want to see all of the make output
.SILENT:

#	The directory we are building
DIR = /usr/lib/bootimages

#	The directory that must be created
DSTDIR = ${DSTROOT}${DIR}
DSTOWNER = root
DSTGROUP = wheel
DSTMODE = 775

IMAGEFILES = systemfiles.eps fsck.eps nfsmount.eps ifconfig.eps
IMAGEMODE = 644

all:

#	Clean out editor lint
clean:
	echo "    Removing" *~ .[a-zA-Z]*~ \#* *.BAK
	rm -f *~ .[a-zA-Z]*~ \#* *.BAK

install: ${DSTDIR}
	cd $(DSTDIR); \
	echo -n "    Image files:"
	for i in $(IMAGEFILES); \
	  do \
		echo -n " $$i" ; \
		install -c -o ${DSTOWNER} -g ${DSTGROUP} -m ${IMAGEMODE} $$i ${DSTDIR}/$$i ; \
	  done
	echo "."

${DSTDIR}: DSTROOT ${DSTROOT}
	echo "    Making ${DSTDIR}"
	mkdirs -o ${DSTOWNER} -g ${DSTGROUP} -m ${DSTMODE} ${DSTDIR}

$(DSTROOT):
	@echo 'Please make a directory for $@ and try again'; exit 1

DSTROOT:
	@if [ -n "${$@}" ]; then exit 0; else echo Must define $@; exit 1; fi
