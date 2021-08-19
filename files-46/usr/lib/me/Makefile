#
# Makefile for the /usr/lib/me directory
#
# This makefile strips comments from the source files and installs
# them in the expected directories.
#

# Comment out this next line if you want to see all of the make output
.SILENT:

#	The makefile above hands us THISDIR for information purposes
THISDIR=.

#	The directory we are building
LIBDIR = /usr/lib
MEDIR = ${LIBDIR}/me
TMACDIR = ${LIBDIR}/tmac

#	The directory that must be created
DSTDIRS = ${DSTROOT}${MEDIR} ${DSTROOT}${TMACDIR}
DSTOWNER = root
DSTGROUP = wheel
DSTMODE = 755

#	Ownership and group of the files and directories installed here
OWNER = root
GROUP = wheel

#	ME source files
MEFILES = tmac.e acm.me chars.me deltext.me eqn.me \
	  float.me footnote.me index.me local.me null.me \
	  refer.me sh.me tbl.me thesis.me
MEMODE = 444

#	Other read only files
ROFILES	= revisions
ROMODE = 444

#	It doesn't mean anything to make in this directory
all:

#	Clean out editor lint
clean:
	echo "    Removing" *~ .[a-zA-Z]*~ \#* *.BAK
	rm -f *~ .[a-zA-Z]*~ \#* *.BAK
	rm -f _mac_tmp_

install: ${DSTDIRS}
	echo -n "    ME files:"
	for i in `echo ${MEFILES}` ; \
	  do \
		echo -n " $$i" ; \
		rm -f _mac_tmp_; \
		sed -e '/%beginstrip%/{' \
		    -e 'h' \
		    -e 's/.*/.\\" This version has had comments stripped; an unstripped version is available./p' \
		    -e 'g' \
		    -e '}' \
		    -e '/%beginstrip%/,$$s/[. 	][ 	]*\\".*//' \
		    -e '/^$$/d' \
		    -e '/\\n@/d' $$i > _mac_tmp_; \
		install -m ${MEMODE}  _mac_tmp_ $(DSTROOT)${MEDIR}/$$i; \
	  done
	echo "."
	echo -n "    Read Only files:"
	for i in `echo ${ROFILES}` ; \
	  do \
		echo -n " $$i" ; \
		install -c -o ${OWNER} -g ${GROUP} -m ${ROMODE} $$i ${DSTROOT}${MEDIR}/$$i ; \
	  done
	echo "."
	echo "    Moving ${DSTROOT}${MEDIR}/tmac.e to ${DSTROOT}${TMACDIR}/tmac.e"
	mv -f ${DSTROOT}${MEDIR}/tmac.e ${DSTROOT}${TMACDIR}/tmac.e
	
${DSTDIRS}: DSTROOT ${DSTROOT}
	echo "    Making ${DSTDIRS}"
	mkdirs -o ${DSTOWNER} -g ${DSTGROUP} -m ${DSTMODE} ${DSTDIRS}

$(DSTROOT):
	@echo 'Please make a directory for $@ and try again'; exit 1

DSTROOT:
	@if [ -n "${$@}" ]; then exit 0; else echo Must define $@; exit 1; fi
