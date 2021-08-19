#
# Makefile for the /private/spool directory
#

# Comment out this next line if you want to see all of the make output
.SILENT:

#	The directory we are building
DIR = /private/spool

#	The directory that must be created
DSTDIR = ${DSTROOT}${DIR}

#	It doesn't mean anything to make in this directory
all:

#	Clean out editor lint
clean:
	echo "    Removing" *~ .[a-zA-Z]*~ \#* *.BAK
	rm -f *~ .[a-zA-Z]*~ \#* *.BAK

install: ${DSTDIR}
	echo -n "    Printer directories: lpd
	mkdirs -o daemon -g daemon -m 770 ${DSTDIR}/lpd
	echo -n " appkit"
	mkdirs -o daemon -g daemon -m 777 ${DSTDIR}/appkit
	echo -n " faxes"
	mkdirs -o daemon -g daemon -m 777 ${DSTDIR}/NeXTFaxes
	echo "."
	echo -n "    Mail directories: mail"
	mkdirs -o root -g wheel -m 1777 ${DSTDIR}/mail
	echo -n " mqueue"
	mkdirs -o root -g wheel -m 770 ${DSTDIR}/mqueue
	echo -n " syslog"
	touch ${DSTDIR}/mqueue/syslog
	chown root.staff ${DSTDIR}/mqueue/syslog
	chmod 640 ${DSTDIR}/mqueue/syslog
	echo "."
	echo -n "    UUCP directories: uucp"
	mkdirs -o uucp -g wheel -m 777 uucp
	echo -n " uucppublic"
	mkdirs -o uucp -g wheel -m 777 uucppublic
	echo "."

${DSTDIR}: DSTROOT ${DSTROOT}
	echo "    Making ${DSTDIR}"
	mkdirs -o root -g wheel -m 755 ${DSTDIR}

$(DSTROOT):
	@echo 'Please make a directory for $@ and try again'; exit 1

DSTROOT:
	@if [ -n "${$@}" ]; then exit 0; else echo Must define $@; exit 1; fi
