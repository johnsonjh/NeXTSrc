SUBDIRS = usr.include libc libcs
HDRDIRS = usr.include

all clean depend tags:
	 @for i in ${SUBDIRS}; \
	 do \
		echo ================= make $@ for $$i =================; \
		(cd $$i; ${MAKE} $@ || exit 1); \
	 done

install: DSTROOT
	@CWD=`pwd`; cd ${DSTROOT}; DSTROOT=`pwd`; cd $$CWD; \
	 for i in ${SUBDIRS}; \
	 do \
		echo ================= make $@ for $$i =================; \
		(cd $$i; ${MAKE} DSTROOT=$$DSTROOT $@ || exit 1); \
	 done

installhdrs: DSTROOT
	@CWD=`pwd`; cd ${DSTROOT}; DSTROOT=`pwd`; cd $$CWD; \
	 for i in ${HDRDIRS}; \
	 do \
		echo ================= make $@ for $$i =================; \
		(cd $$i; ${MAKE} DSTROOT=$$DSTROOT install || exit 1); \
	 done

installsrc:	SRCROOT
	tar cf - . | (cd ${SRCROOT}; tar xfBp -)

SRCROOT DSTROOT:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi
