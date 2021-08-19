#
# Makefile for the /private/vm directory
#

# Comment out this next line if you want to see all of the make output
.SILENT:

#	The directory we are building
DIR = /private/vm

#	The directory that must be created
DSTDIR = ${DSTROOT}${DIR}
DSTOWNER = root
DSTGROUP = wheel
DSTMODE = 755

#	Ownership and group of the files and directories installed here
OWNER = root
GROUP = wheel

#	Files that are ok to edit that are initially empty
SWAPFILES = swapfile
SWAPMODE = 1600

#	It doesn't mean anything to make in this directory
all:

#	Clean out editor lint
clean:
	echo "    Removing" *~ .[a-zA-Z]*~ \#* *.BAK
	rm -f *~ .[a-zA-Z]*~ \#* *.BAK

install: ${DSTDIR}
	echo -n "    Swap files:"
	for i in `echo ${SWAPFILES}` ; \
	  do \
		echo -n " $$i" ; \
		touch ${DSTDIR}/$$i ; \
		chown ${OWNER}.${GROUP} ${DSTDIR}/$$i ; \
		chmod ${SWAPMODE} ${DSTDIR}/$$i ; \
	  done
	echo "."

${DSTDIR}: DSTROOT ${DSTROOT}
	echo "    Making ${DSTDIR}"
	mkdirs -o ${DSTOWNER} -g ${DSTGROUP} -m ${DSTMODE} ${DSTDIR}

$(DSTROOT):
	@echo 'Please make a directory for $@ and try again'; exit 1

DSTROOT:
	@if [ -n "${$@}" ]; then exit 0; else echo Must define $@; exit 1; fi
