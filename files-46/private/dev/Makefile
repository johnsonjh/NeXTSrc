#
# Makefile for the /private/dev directory
#

# Comment out this next line if you want to see all of the make output
.SILENT:

#	The directory we are building
DIR = /private/dev

#	The directory that must be created
DSTDIR = ${DSTROOT}${DIR}
DSTOWNER = root
DSTGROUP = wheel
DSTMODE = 755

#	Ownership and group of the files and directories installed here
OWNER = root
GROUP = staff

#	Files that are ok to edit
RWFILES	= MAKEDEV.local
RWMODE = 664

#	It doesn't mean anything to make in this directory
all:

#	Clean out editor lint
clean:
	echo "    Removing" *~ .[a-zA-Z]*~ \#* *.BAK
	rm -f *~ .[a-zA-Z]*~ \#* *.BAK

install: ${DSTDIR}
	echo "    Linking MAKEDEV to ../../usr/etc/MAKEDEV"
	ln -s ../../usr/etc/MAKEDEV ${DSTDIR}
	echo -n "    Read/Write files:"
	for i in `echo ${RWFILES}` ; \
	  do \
		echo -n " $$i"; \
		install -c -o ${OWNER} -g ${GROUP} -m ${RWMODE} $$i ${DSTDIR}/$$i ; \
	  done
	echo "."

${DSTDIR}: DSTROOT ${DSTROOT}
	echo "    Making ${DSTDIR}"
	mkdirs -o ${DSTOWNER} -g ${DSTGROUP} -m ${DSTMODE} ${DSTDIR}

$(DSTROOT):
	@echo 'Please make a directory for $@ and try again'; exit 1

DSTROOT:
	@if [ -n "${$@}" ]; then exit 0; else echo Must define $@; exit 1; fi
