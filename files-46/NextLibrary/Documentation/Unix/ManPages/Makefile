#
# Makefile for the /NextLibrary/Documentation/Unix/ManPages directory
#

# Comment out this next line if you want to see all of the make output
.SILENT:

#	The directory we are building
DIR = /NextLibrary/Documentation/Unix/ManPages

#	The directory that must be created
DSTDIR = ${DSTROOT}${DIR}
DSTOWNER = root
DSTGROUP = staff
DSTMODE = 775

#	Ownership and group of the files and directories installed here
OWNER = root
GROUP = staff

#	Empty subdirectories
EMPTYDIRS = cat1 cat2 cat3 cat4 cat5 cat6 cat7 cat8 cat9 catl catn
EMPTYMODE = 777

#	It doesn't mean anything to make in this directory
all:

#	Clean out editor lint
clean:
	echo "    Removing" *~ .[a-zA-Z]*~ \#* *.BAK
	rm -f *~ .[a-zA-Z]*~ \#* *.BAK

install: ${DSTDIR}
	echo -n "    Empty subdirectories:"
	for i in ` echo ${EMPTYDIRS}` ; \
	  do \
		echo -n " $$i" ; \
		mkdirs -o ${OWNER} -g ${GROUP} -m ${EMPTYMODE} ${DSTDIR}/$$i ;\
	  done
	echo "."

${DSTDIR}: DSTROOT ${DSTROOT}
	echo "    Making ${DSTDIR}"
	mkdirs -o ${DSTOWNER} -g ${DSTGROUP} -m ${DSTMODE} ${DSTDIR}

$(DSTROOT):
	@echo 'Please make a directory for $@ and try again'; exit 1

DSTROOT:
	@if [ -n "${$@}" ]; then exit 0; else echo Must define $@; exit 1; fi
