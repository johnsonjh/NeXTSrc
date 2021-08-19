#
# NeXT Makefile to build the meta-shlib libsys
#

# Where host shared library gets installed
LIBDIR = /lib

# Where target shared library gets installed
SHLIBDIR = /usr/shlib

# Where the bill of materials file gets installed
BOMDIR = /usr/local/BOMs

# The major version of this shared library
SHLIBVERS = B

# This uses the strip -S option
SHLIBIFLAGS = -sSd debug_objects_list

# Symbolic links to the directories which contain the projects that make up
# this meta-shlib
PROJDIRS = libc libm libsound libobjc libstreams libdb librld 

# The default directory name for the current sources (and objects).  This is
# only used by installsrc to create the symbolic links.  This is a best guess
# but might work if the current conventions are followed.  For each project
# that the developer is working on the link will need to be set by hand to
# point at their sources.
LIBC =		/sources/projects/libc/libc
LIBM =		/sources/projects/libm
LIBSOUND =	/sources/projects/sound
LIBOBJC =	/sources/projects/objc
LIBSTREAMS = 	/sources/projects/streams
LIBDB =		/sources/projects/libdb
LIBRLD =	/sources/projects/cc_proj/ld

HFILES = shlib.h
CFILES = pointers.c
OTHER_SRCS = Makefile spec_sys mdb.sed notes debug_objects_list	\
	overrides fixed_text_order scattered_text_order libsys.user_bom \
	fixed_data_order scattered_data_order docompare.csh

VERSFILE = vers

SRCS = $(CFILES) $(HFILES) $(OTHER_SRCS)
OFILES = $(CFILES:.c=.o) $(VERSFILE).o

# at the top so that its the default
develop_all: shlib_ofiles shlib

all: current_links shlib_ofiles shlib profile_ofiles profile test

install: DSTROOT force_version $(DSTROOT)$(LIBDIR) $(DSTROOT)$(SHLIBDIR) $(DSTROOT)$(BOMDIR) all installbom
	install -m 444 libsys_s.a $(DSTROOT)$(LIBDIR)/libsys_s.a
	install $(SHLIBIFLAGS) -m 555 libsys_s.$(SHLIBVERS).shlib \
		$(DSTROOT)$(SHLIBDIR)/libsys_s.$(SHLIBVERS).shlib 
	install -m 444 libsys_p.a $(DSTROOT)$(LIBDIR)/libsys_p.a
	mkdirs $(DSTROOT)/usr/local/shlib $(DSTROOT)/usr/local/lib
	ln -s ../src/libsys/libsys_s.$(SHLIBVERS).shlib \
		$(DSTROOT)/usr/local/shlib/libsys_s.$(SHLIBVERS).shlib
#	install -m 444 libsys_mdb_s.a $(DSTROOT)/usr/local/lib/libsys_mdb_s.a
#	install $(SHLIBIFLAGS) -m 555 libsys_mdb_s.$(SHLIBVERS).shlib \
#		$(DSTROOT)/usr/local/shlib/libsys_mdb_s.$(SHLIBVERS).shlib 

SRCROOT DSTROOT:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

$(DSTROOT)$(LIBDIR) $(DSTROOT)$(SHLIBDIR) $(DSTROOT)$(BOMDIR):
	mkdirs $@

shlib_ofiles: 
	SHLIBIDIR=`pwd`; \
	for i in `echo ${PROJDIRS}`; \
	  do \
		echo ================= make $@ for $$i =================; \
		(cd $$i; ${MAKE} SHLIBIDIR=$$SHLIBIDIR $@); \
	  done

shlib: $(OFILES) text_order data_order
	mkshlib $(MKSHLIBFLAGS) -s spec_sys 				\
		-t libsys_s.$(SHLIBVERS).shlib 				\
		-h libsys_s_INDR.a 					\
		-minor_version `vers_string -n`				\
		-sectorder __TEXT __text text_order			\
		-sectorder __DATA __data data_order			\
		-segaddr __LINKEDIT 05088000
	indr libc/SHLIB_OBJS/IND_LIST libsys_s_INDR.a libsys_s.a
	rm libsys_s_INDR.a
#	rm -f spec_sys_mdb
#	sed -f mdb.sed < spec_sys > spec_sys_mdb
#	mkshlib $(MKSHLIBFLAGS) -s spec_sys_mdb				\
#		-t libsys_mdb_s.$(SHLIBVERS).shlib 			\
#		-h libsys_mdb_s_INDR.a 					\
#		-minor_version `vers_string -n`				\
#		-segaddr __LINKEDIT 05088000
#	indr libc/SHLIB_OBJS/IND_LIST libsys_mdb_s_INDR.a libsys_mdb_s.a
#	rm libsys_mdb_s_INDR.a

profile_ofiles: 
	for i in `echo ${PROJDIRS}`; \
	  do \
		echo ================= make $@ for $$i =================; \
		(cd $$i; ${MAKE} $@); \
	  done

profile:
	rm -f libsys_p_INDR.a libsys_p.a
	for i in `echo ${PROJDIRS}`; \
	  do \
		ar cq libsys_p_INDR.a $$i/profile_obj/*.o; \
	  done
	indr libc/IND_LIST libsys_p_INDR.a libsys_p.a
	ranlib libsys_p.a

clean:
	rm -f libsys_s.a libsys_s.$(SHLIBVERS).shlib libsys_s_INDR.a $(OFILES)
	rm -f libsys_p_INDR.a libsys_p.a

installsrc: $(SRCROOT)
	mkdirs $(SRCROOT)
	tar cf - $(SRCS) | (cd $(SRCROOT); tar xf -)
	
test:	overrides compareshlib

compareshlib::
	@echo "Comparing old with new shlib version"
	./docompare.csh
	
overrides::
	echo "Checking overridable functions: shouldn't see any output"
	nm -ou libsys_s.a  > /tmp/libsys.nm
	for i in `cat overrides`; \
	do \
		echo -n $$i >> /tmp/greplist;	\
		echo -n "|" >> /tmp/greplist;	\
	done
	egrep -f /tmp/greplist /tmp/libsys.nm
	rm -f /tmp/greplist /tmp/libsys.nm

current_links:
	if [ -f PROJECT_FROZEN ]; then exit 1; fi
	rm -f libc libm libsound libobjc libstreams libdb librld 
	ln -s ../`ls -l ../libc | awk '{print $$10}'`/libc libc
	ln -s ../`ls -l ../libm | awk '{print $$10}'` libm
	ln -s ../`ls -l ../sound | awk '{print $$10}'` libsound
	ln -s ../`ls -l ../objc | awk '{print $$10}'` libobjc
	ln -s ../`ls -l ../streams | awk '{print $$10}'` libstreams
	ln -s ../`ls -l ../libdb | awk '{print $$10}'` libdb
	ln -s ../`ls -l ../cc | awk '{print $$10}'`/ld librld

installbom: $(DSTROOT) $(DSTROOT)$(BOMDIR)
	install -m 444 libsys.user_bom $(DSTROOT)$(BOMDIR)
	
# rules to make version files
$(VERSFILE).c:
	vers_string -l libsys > $@

force_version::
	-/bin/rm -f $(VERSFILE).c


text_order: fixed_text_order scattered_text_order
	cat fixed_text_order scattered_text_order > text_order

data_order: fixed_data_order scattered_data_order
	cat fixed_data_order scattered_data_order > data_order

