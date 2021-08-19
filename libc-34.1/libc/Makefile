#
# Generic Makefile for libraries
# (c) NeXT, Inc.  1989
#
LIBNAME= libc
BUILDS= STD PROF
HOSTLOOKUP= hosttable
MACHINE= next
LIBDIR= /usr/local/lib/
SHLIBIDIR=

RANLIB= /bin/ranlib
AR= /bin/ar
ARFLAGS= x
RM= /bin/rm
LN= /bin/ln

IFLAGS= -r -m 444

COMPAT=	compat-4.1 compat-sys5
MACHINEDIRS= $(MACHINE)/gen $(MACHINE)/compat-4.1	\
	$(MACHINE)/net $(MACHINE)/stdio $(MACHINE)/sys	\
	$(MACHINE)/ansi
SUBDIR=	ansi des gen inet net ns stdio $(MACHINEDIRS)	\
	$(COMPAT) gnu rpc yp netinfo mach threads	\
	internat

DERIVED=
GARBAGE=

SHLIB_NAME=_s
PROF_NAME=_p
STD_NAME=

SHLIB_COPTS= 
PROF_COPTS= 
STD_COPTS= 

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

VPATH=..
DIR=.

.c.o:
	$(CC) -MD $(CFLAGS) $(COPTS) -c -o $@ $<
	md -d -u Makedep $*.d

all:	$(BUILDS)

SHLIB PROF STD: $$@_OBJS ALWAYS
	@echo '#################' make $@  '###################'
	@for i in ${SUBDIR}; 						    \
	do 								    \
		echo ================= make $@ for $$i =================;   \
		(cd $$i; ${MAKE}  "SHLIBIDIR=$$SHLIBIDIR"		    \
			"BUILDS=$(BUILDS)" $@ || exit 1);	 	    \
	done
	@echo '#################' unarchive $@  '###################'
	-rm -f $@_OBJS/*
	@for i in ${SUBDIR}; 						    \
	do 								    \
		echo ============== unarchive for $$i ==============;	    \
		cd $@_OBJS; 						    \
		$(AR) $(ARFLAGS) ../$$i/$@_OBJS/`basename $$i`lib$($@_NAME);\
		cd ..;							    \
	done
	@if [ "$@" = "STD" ];						    \
	then 								    \
		echo ============== make $@ library  ==============;	    \
		( cd $@_OBJS; $(MAKE) -f ../Makefile "COPTS=$($@_COPTS)"    \
		    "LDOPTS=$($@_LDOPTS)" LIBRARY=$(LIBNAME)$($@_NAME)      \
		    "DIR=$@_OBJS" $(LIBNAME)$($@_NAME) || exit 1);	    \
	fi
	@echo '#################' make $@ done '###################'

install: DSTROOT ${DSTROOT}${LIBDIR} all
	install ${IFLAGS} libc.a ${DSTROOT}${LIBDIR}libc_static.a
	(cd $(MACHINE)/csu; $(MAKE) $(MFLAGS) DEFS=$(DEFS) DSTROOT=$$DSTROOT $@);
#	(cd $(MACHINE)-csu; $(MAKE) $(MFLAGS) DEFS=$(DEFS) DSTROOT=$$DSTROOT $@);

clean depend tags: ALWAYS
	@echo '#################' make $@  '###################'
	@for i in ${SUBDIR}; \
	do \
		echo ================= make $@ for $$i =================; \
		(cd $$i; ${MAKE} $@ "BUILDS=$(BUILDS)" || exit 1); \
	done
	@echo ================= make $@ for libc =================;
	-rm -f *.o *.a TAGS tags *_vers.c Makedep $(DERIVED) $(GARBAGE)
	-rm -f SHLIB_OBJS/* 
	-rm -f PROF_OBJS/* 
	-rm -f STD_OBJS/*

shlib_ofiles: SHLIB_OBJS SHLIB $(LIBNAME)_vers.o
	-rm -f shlib_obj
	-$(LN) -s SHLIB_OBJS shlib_obj
	$(CP) $(LIBNAME)_vers.o SHLIB_OBJS
	$(CP) IND_LIST SHLIB_OBJS

profile_ofiles: PROF_OBJS $(LIBNAME)_vers.o
	-rm -f profile_obj
	-$(LN) -s PROF_OBJS profile_obj
	$(CP) $(LIBNAME)_vers.o PROF_OBJS
#
# Building the library
#
$(LIBRARY): $(OBJS) $(LIBRARY).tmp IND_LIST
	indr IND_LIST $(LIBRARY).tmp $(LIBRARY).a
	ranlib -s $(LIBRARY).a
	mv $(LIBRARY).a ..

$(LIBRARY).tmp: $(LIBRARY)_vers.o
	-cp ../save/gnusoftfloat.o .
	$(RM) -f $(LIBRARY).a $(LIBRARY).tmp
	$(AR) cr $@ `lorder *.o | tsort`
	$(RANLIB) -s $(LIBRARY).tmp

$(LIBNAME)_vers.c: $(DOVERS) $(OBJS)
	@rm -f $@
	vers_string -l $(LIBNAME) > $@

IND_LIST: ALWAYS
	rm -f ../IND_LIST ../SHLIB_OBJS/IND_LIST ../STD_OBJS/IND_LIST
	@echo '#################' make $(MACHINE)/CSU all '###################'
	(cd ../$(MACHINE)/csu; $(MAKE) $(MFLAGS) DEFS=$(DEFS) all);
	$(CC) -c $(DEFS) -I../../usr.include ../ANSI_TEST.c -o tmp-ansi0
	nm -g tmp-ansi0 | sed -n -e '/^[ 0-9a-f]*[TUDCB] \(_[^_][^ ]*\).*$$/s//\1/p' | sort -u > tmp-ansi1
	ld -r ../$(MACHINE)/csu/crt0.o tmp-ansi0 $(LIBNAME).tmp -o tmp-ansi2
	nm -g tmp-ansi2 | sed -n -e '/^[ 0-9a-f]*[TUDCB] \(_[^_][^ ]*\).*$$/s//\1/p' | sort -u > tmp-ansi3
	rm -f $@
	sort -m tmp-ansi1 tmp-ansi3 | uniq -u | sort -u > $@
	-cp IND_LIST ..
	rm -f tmp-ansi0 tmp-ansi1 tmp-ansi2 tmp-ansi3

#
# Standard targets
#
DSTROOT:
	@if [ -n "$($@)" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

$(DSTROOT)$(LIBDIR) $(DSTDIRS) SHLIB_OBJS PROF_OBJS STD_OBJS:
	mkdirs $@

tags:	ALWAYS
	etags $(SRCS)
	ctags $(SRCS)

ALWAYS depend:

-include $(DIR)/Makedep
