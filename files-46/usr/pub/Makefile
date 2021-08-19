#
# Makefile for the /usr/pub directory
#

# Comment out this next line if you want to see all of the make output
.SILENT:

#	The directory we are building
DIR = /usr/pub

#	The directory that must be created
DSTDIR = ${DSTROOT}${DIR}
DSTOWNER = root
DSTGROUP = wheel
DSTMODE = 755

#	Ownership and group of the files and directories installed here
OWNER = root
GROUP = wheel

#	Files that we don't want people to edit
ROFILES =
ROMODE = 444

#	Files that are ok to edit
RWFILES	= ascii eqnchar greek
RWMODE = 644

#	Files that are ok to edit that are initially empty
EMPTYFILES =
EMPTYMODE = 664

#	It doesn't mean anything to make in this directory
all:

#	Clean out editor lint
clean:
	echo "    Removing" *~ .[a-zA-Z]*~ \#* *.BAK
	rm -f *~ .[a-zA-Z]*~ \#* *.BAK

install: ${DSTDIR}
	echo -n "    Read only files:"
	for i in `echo ${ROFILES}` ; \
	  do \
		echo -n " $$i" ; \
		install -c -o ${OWNER} -g ${GROUP} -m ${ROMODE} $$i ${DSTDIR}/$$i ; \
	  done
	echo "."
	echo -n "    Read/Write files:"
	for i in `echo ${RWFILES}` ; \
	  do \
		echo -n " $$i" ; \
		install -c -o ${OWNER} -g ${GROUP} -m ${RWMODE} $$i ${DSTDIR}/$$i ; \
	  done
	echo "."
	echo -n "    Empty files:"
	for i in `echo ${EMPTYFILES}` ; \
	  do \
		echo -n " $$i" ; \
		touch ${DSTDIR}/$$i ; \
		chown ${OWNER}.${GROUP} ${DSTDIR}/$$i ; \
		chmod ${EMPTYMODE} ${DSTDIR}/$$i ; \
	  done
	echo "."

${DSTDIR}: DSTROOT ${DSTROOT}
	echo "    Making ${DSTDIR}"
	mkdirs -o ${DSTOWNER} -g ${DSTGROUP} -m ${DSTMODE} ${DSTDIR}

$(DSTROOT):
	@echo 'Please make a directory for $@ and try again'; exit 1

DSTROOT:
	@if [ -n "${$@}" ]; then exit 0; else echo Must define $@; exit 1; fi
