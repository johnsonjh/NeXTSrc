#
# Top level makefile for the FILES project.  This establishes the basic 
# files necessary for a NeXT filesystem.
#

# Comment out this line if you want make output
.SILENT:

# The makefile above hands us THISDIR for informational purposes
THISDIR=.

#	The directory we are building
DIR = /
BOMDIR = /usr/local/BOMs
USERBOM = files.user_bom

#	The directory that must be created
DSTDIR = ${DSTROOT}${DIR}
BOMDSTDIR = ${DSTROOT}${BOMDIR}
DSTOWNER = root
DSTGROUP = wheel
DSTMODE = 755

#	Subdirectories with their own makefiles
SUBDIRS = private usr NextLibrary NextDeveloper

#	Empty directories to be created
EMPTYDIRS = Net lib

#	Dirs in /private that should be linked to
PRIVATEDIR= private
PRIVATELINKS= dev etc tmp

#	Super-User files
SUFILES= cshrc login profile hidden
SUMODE= 644

all:
	for i in ${SUBDIRS} ; \
	  do \
		echo ============= make $@ for ${THISDIR}/$$i ============= ; \
		(cd $$i; ${MAKE} $@ THISDIR=${THISDIR}/$$i) || exit $? ; \
	  done

clean:
	echo "    Removing" *~ .[a-zA-Z]*~ \#* *.BAK
	rm -f *~ .[a-zA-Z]*~ \#* *.BAK
	for i in ${SUBDIRS} ; \
	  do \
		echo ============= make $@ for ${THISDIR}/$$i ============= ; \
		(cd $$i; ${MAKE} $@ THISDIR=${THISDIR}/$$i) || exit $? ; \
	  done

install: ${DSTDIR}
	echo -n "    Creating special links:"
	for i in ${PRIVATELINKS} ; \
	  do \
		echo -n " $$i" ; \
		ln -s ${PRIVATEDIR}/$$i ${DSTROOT}/$$i ; \
	  done
	for i in ${EMPTYDIRS} ; \
	  do \
		echo -n " $$i" ; \
		mkdir ${DSTROOT}/$$i ; \
	  done
	echo -n " /mach"
	ln -s \$$BOOTFILE ${DSTROOT}/mach
	echo "."

	echo -n "    Super-User files:"
	for i in ${SUFILES} ; \
	  do \
		echo -n " .$$i" ; \
		install -c -o ${DSTOWNER} -g ${DSTGROUP} -m ${SUMODE} root_$$i ${DSTDIR}/.$$i ; \
	  done
	echo -n " .NeXT" ; \
	mkdirs -o ${DSTOWNER} -g ${DSTGROUP} -m ${DSTMODE} ${DSTDIR}/.NeXT
	echo "."

	for i in ${SUBDIRS} ; \
	  do \
		echo ============= make $@ for ${THISDIR}/$$i ============= ; \
		(cd $$i; ${MAKE} $@ THISDIR=${THISDIR}/$$i) || exit $? ; \
	  done
	install ${USERBOM} ${BOMDSTDIR}

#	Make a snapshot of this source directory.
installsrc: SRCROOT ${SRCROOT}
	tar cf - . | (cd ${SRCROOT}; tar xfBp -)
	
${DSTDIR} ${BOMDSTDIR}: DSTROOT ${DSTROOT}
	mkdirs -o ${DSTOWNER} -g ${DSTGROUP} -m ${DSTMODE} ${DSTDIR}
	mkdirs -o ${DSTOWNER} -g ${DSTGROUP} -m ${DSTMODE} ${BOMDSTDIR}

$(DSTROOT) $(SRCROOT):
	echo 'Please make a directory for $@ and try again'; exit 1

DSTROOT SRCROOT:
	@if [ -n "${$@}" ]; then exit 0; else echo Must define $@; exit 1; fi
