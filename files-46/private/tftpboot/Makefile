#
# Makefile for the /private/tftpboot directory.
#

# Comment out this line if you want make output
.SILENT:

# The makefile above hands us THISDIR for informational purposes
THISDIR=.

#	The directory we are building
DIR = /private/tftpboot

#	The directory that must be created
DSTDIR = ${DSTROOT}${DIR}
DSTOWNER = root
DSTGROUP = wheel
DSTMODE = 775

all:

clean:
	echo "    Removing" *~ .[a-zA-Z]*~ \#* *.BAK
	rm -f *~ .[a-zA-Z]*~ \#* *.BAK

install: ${DSTDIR}
	echo "    Making ${DSTDIR}/private"
	mkdirs -o ${DSTOWNER} -g ${DSTGROUP} -m ${DSTMODE} ${DSTDIR}/private
	echo "    Linking ${DSTDIR}/private/tftpboot to /"
	ln -s / ${DSTDIR}/private/tftpboot

${DSTDIR}: DSTROOT ${DSTROOT}
	mkdirs -o ${DSTOWNER} -g ${DSTGROUP} -m ${DSTMODE} ${DSTDIR}

$(DSTROOT):
	echo 'Please make a directory for $@ and try again'; exit 1

DSTROOT:
	@if [ -n "${$@}" ]; then exit 0; else echo Must define $@; exit 1; fi
